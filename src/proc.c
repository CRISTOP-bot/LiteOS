/* LiteOS: tabla de procesos, scheduler round-robin preventivo,
 * fork/exec/exit/wait y montaje de la pila inicial de usuario. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "kernel.h"
extern void mm_free_pml4(u64 pml4);
extern void proc_child_entry(void);

static struct proc procs[NPROC];
struct proc *current;
int need_resched;
static int next_pid = 1;
struct trap_frame *first_user_frame;

/* ---- Descriptores de archivo ---- */

int proc_fd_alloc(struct file *f)
{
    for (int i = 0; i < PROC_FDS; i++) {
        if (!current->fds[i]) {
            current->fds[i] = f;
            return i;
        }
    }
    return -EMFILE;
}

void proc_fd_close(int fd)
{
    if (fd < 0 || fd >= PROC_FDS || !current->fds[fd])
        return;
    struct file *f = current->fds[fd];
    current->fds[fd] = NULL;
    if (f->vn->type == V_PIPE) {
        if ((f->flags & O_ACCMODE) == O_RDONLY)
            pipe_adjust(f, -1, 0);
        if ((f->flags & O_ACCMODE) == O_WRONLY)
            pipe_adjust(f, 0, -1);
    }
    if (--f->ref == 0) {
        vrelease(f->vn);
        kfree(f);
    }
}

/* ---- Ciclo de vida ---- */

static struct proc *proc_alloc(void)
{
    for (int i = 1; i < NPROC; i++) {
        struct proc *p = &procs[i];
        if (p->state != PS_FREE)
            continue;
        memset(p, 0, sizeof(*p));
        p->pid = next_pid++;
        p->state = PS_RUNNABLE;
        p->parent = current;
        p->cwd = current ? current->cwd : vfs_root();
        if (p->cwd)
            vref(p->cwd);

        /* Pila del kernel: 2 paginas contiguas */
        uint64_t base = 0;
        for (int tries = 0; tries < 64; tries++) {
            uint64_t p1 = pmm_alloc();
            uint64_t p2 = pmm_alloc();
            if (p1 && p2 && p2 == p1 + 4096) {
                base = p1;
                break;
            }
            if (p1)
                pmm_free(p1);
            if (p2)
                pmm_free(p2);
        }
        if (!base) {
            if (p->cwd) vrelease(p->cwd);
            p->state = PS_FREE;
            return NULL;
        }
        p->kstack = base + KSTACK_SZ;
        snprintf(p->name, sizeof(p->name), "proc%d", p->pid);
        return p;
    }
    return NULL;
}

static void proc_discard(struct proc *p)
{
    for (int i = 0; i < PROC_FDS; i++) {
        struct file *f = p->fds[i];
        if (!f) continue;
        if (f->vn->type == V_PIPE)
            pipe_adjust(f, (f->flags & O_ACCMODE) == O_RDONLY ? -1 : 0,
                           (f->flags & O_ACCMODE) == O_WRONLY ? -1 : 0);
        if (--f->ref == 0) {
            vrelease(f->vn);
            kfree(f);
        }
    }
    if (p->mm) mm_destroy(p->mm);
    if (p->cwd) vrelease(p->cwd);
    if (p->kstack) {
        pmm_free(p->kstack - KSTACK_SZ);
        pmm_free(p->kstack - 4096);
    }
    memset(p, 0, sizeof(*p));
}

void proc_init(void)
{
    memset(procs, 0, sizeof(procs));
    current = NULL;
    need_resched = 0;

    /* Proceso idle (slot 0): usa la pila de arranque del
     * enlazador; su contexto se salva en la primera
     * conmutacion y kernel_main se convierte en su bucle. */
    struct proc *idle = &procs[0];
    idle->pid = 0;
    idle->state = PS_RUNNING;
    strcpy(idle->name, "idle");
    idle->kstack = (uint64_t)&_stack_top;
    idle->cwd = vfs_root();
    vref(idle->cwd);

    current = idle;
}

int proc_start_init(const char *path)
{
    struct vnode *image = vfs_lookup(path, vfs_root());
    if (!image || image->type != V_REG) {
        if (image) vrelease(image);
        return -ENOENT;
    }
    struct proc *p = proc_alloc();
    if (!p) { vrelease(image); return -ENOMEM; }
    p->mm = mm_create();
    u64 entry = 0;
    int rc = p->mm ? elf_load(image->data, image->size, p->mm, &entry) : -ENOMEM;
    vrelease(image);
    char *argv[] = {(char *)path, NULL};
    u64 sp = rc ? 0 : setup_user_stack(p->mm, argv, NULL);
    if (!sp) rc = rc ? rc : -ENOMEM;
    if (rc) {
        proc_discard(p);
        return rc;
    }
    struct vnode *console = vfs_lookup("/dev/console", vfs_root());
    if (!console) { proc_discard(p); return -ENOENT; }
    for (int i = 0; i < 3; i++) {
        struct file *f = kmalloc(sizeof(*f));
        if (!f) {
            vrelease(console);
            proc_discard(p);
            return -ENOMEM;
        }
        memset(f, 0, sizeof(*f));
        f->vn = vref(console);
        f->ref = 1;
        f->flags = i ? O_WRONLY : O_RDONLY;
        p->fds[i] = f;
    }
    vrelease(console);
    struct trap_frame *tf = (struct trap_frame *)(p->kstack - sizeof(*tf));
    memset(tf, 0, sizeof(*tf));
    tf->rip = entry;
    tf->cs = GDT_UCODE;
    tf->ss = GDT_UDATA;
    tf->rflags = 0x202;
    tf->rsp = sp;
    u64 *context = (u64 *)tf;
    *--context = (u64)proc_child_entry;
    for (int i = 0; i < 6; i++) *--context = 0;
    p->ksp = (u64)context;
    return p->pid;
}

/* ---- Scheduler ---- */

void proc_wakeup(void *wchan)
{
    for (int i = 0; i < NPROC; i++) {
        if (procs[i].state == PS_SLEEPING && procs[i].wchan == wchan) {
            procs[i].state = PS_RUNNABLE;
            procs[i].wchan = NULL;
        }
    }
}

void schedule(void)
{
    if (!current)
        return;
    need_resched = 0;

    int ci = (int)(current - procs);
    struct proc *next = NULL;

    /* Round-robin: primer ejecutable despues de current */
    for (int i = 1; i < NPROC && !next; i++) {
        struct proc *p = &procs[(ci + i) % NPROC];
        if (p->state == PS_RUNNABLE)
            next = p;
    }

    if (!next) {
        if (current->state == PS_RUNNING)
            return;
        next = &procs[0];              /* idle */
        next->state = PS_RUNNABLE;
    }

    if (next == current)
        return;

    if (current->state == PS_RUNNING)
        current->state = PS_RUNNABLE;
    next->state = PS_RUNNING;

    cr3_load(next->mm ? next->mm->pml4 : kernel_pml4);
    tss_set_rsp0(next->kstack);
    switch_to(next);
}

struct proc *proc_self(void)
{
    return current;
}

/* ---- fork ---- */

int proc_fork(struct trap_frame *tf)
{
    struct proc *p = proc_alloc();
    if (!p)
        return -EAGAIN;

    /* Espacio de direcciones: copia profunda de paginas de usuario */
    uint64_t new_pml4 = mm_clone(current->mm->pml4);
    if (!new_pml4) {
        proc_discard(p);
        return -ENOMEM;
    }
    p->mm = kmalloc(sizeof(struct mm));
    if (!p->mm) {
        mm_free_pml4(new_pml4);
        proc_discard(p);
        return -ENOMEM;
    }
    p->mm->pml4 = new_pml4;
    p->mm->brk = current->mm->brk;
    p->mm->mmap_top = current->mm->mmap_top;

    /* Los descriptores comparten offset y lifetime, como dup2(). */
    for (int i = 0; i < PROC_FDS; i++) {
        struct file *f = current->fds[i];
        if (!f) continue;
        p->fds[i] = f;
        f->ref++;
        if (f->vn->type == V_PIPE)
            pipe_adjust(f, (f->flags & O_ACCMODE) == O_RDONLY,
                           (f->flags & O_ACCMODE) == O_WRONLY);
    }

    /* El hijo reanuda directamente en el epílogo del trap, no en
     * el stack C del padre (que puede cambiar tras wait/exec). */
    struct trap_frame *child_tf = (struct trap_frame *)(p->kstack - sizeof(*tf));
    *child_tf = *tf;
    child_tf->rax = 0;
    u64 *sp = (u64 *)child_tf;
    *--sp = (u64)proc_child_entry;
    for (int i = 0; i < 6; i++) *--sp = 0;
    p->ksp = (u64)sp;
    return p->pid;
}

/* ---- exit / wait / kill ---- */

static void proc_close_fds(struct proc *p)
{
    for (int i = 0; i < PROC_FDS; i++) {
        struct file *f = p->fds[i];
        if (!f)
            continue;
        p->fds[i] = NULL;
        if (f->vn->type == V_PIPE) {
            if ((f->flags & O_ACCMODE) == O_RDONLY)
                pipe_adjust(f, -1, 0);
            if ((f->flags & O_ACCMODE) == O_WRONLY)
                pipe_adjust(f, 0, -1);
        }
        if (--f->ref == 0) {
            vrelease(f->vn);
            kfree(f);
        }
    }
}

void proc_exit(int code)
{
    proc_close_fds(current);
    if (current->mm) {
        mm_destroy(current->mm);
        current->mm = NULL;
    }
    if (current->cwd) {
        vrelease(current->cwd);
        current->cwd = NULL;
    }

    current->state = PS_ZOMBIE;
    current->exit_code = code;

    if (current->parent)
        proc_wakeup(current->parent);

    schedule();
    for (;;)
        __asm__ volatile ("hlt");      /* no se alcanza */
}

int proc_wait(int pid, int *status)
{
    for (;;) {
        struct proc *child = NULL;
        int has_children = 0;

        for (int i = 1; i < NPROC; i++) {
            struct proc *p = &procs[i];
            if (p->state == PS_FREE || p->parent != current ||
                (pid >= 0 && p->pid != pid))
                continue;
            has_children = 1;
            if (p->state == PS_ZOMBIE) {
                child = p;
                break;
            }
        }

        if (child) {
            int pidd = child->pid;
            int code = child->exit_code;
            uint64_t base = child->kstack - KSTACK_SZ;
            pmm_free(base);
            pmm_free(base + 4096);
            memset(child, 0, sizeof(*child));
            if (status)
                *status = (code & 0xFF) << 8;   /* WIFEXITED */
            return pidd;
        }

        if (!has_children)
            return -ECHILD;

        current->state = PS_SLEEPING;
        current->wchan = current;
        schedule();
    }
}

int proc_kill(int pid, int sig)
{
    if (pid <= 0)
        return -EINVAL;
    if (sig != SIGKILL && sig != SIGTERM && sig != SIGINT)
        return -EINVAL;

    for (int i = 1; i < NPROC; i++) {
        struct proc *p = &procs[i];
        if (p->pid == pid && p->state != PS_FREE && p->state != PS_ZOMBIE) {
            p->killed = 1;
            p->kill_sig = sig;
            if (p->state == PS_SLEEPING) {
                p->state = PS_RUNNABLE;
                p->wchan = NULL;
            }
            return 0;
        }
    }
    return -ESRCH;
}

/* ---- Pila inicial de usuario ----
 * Layout (ver docs/abi.md):
 *   [argc][argv[0..n-1]][NULL][envp[0..n-1]][NULL][cadenas...]
 * con rsp 16-byte alineado al entrar a _start. */

u64 setup_user_stack(struct mm *mm, char *const argv[], char *const envp[])
{
    const u64 top = USER_STACK_TOP;

    for (u64 a = top - 8 * 4096; a < top; a += 4096) {
        uint64_t frame = pmm_alloc();
        if (!frame)
            return 0;
        memset((void *)frame, 0, 4096);
        if (map_page(mm->pml4, a, frame, PAGE_USER) < 0) {
            pmm_free(frame);
            return 0;
        }
    }

    u64 argv_addr[32], env_addr[32], words[67];
    int argc = 0, envc = 0;
    u64 sp = top;
    while (argv && argv[argc]) {
        if (argc == 32) return 0;
        usize len = strlen(argv[argc]) + 1;
        if (len > 4096 || sp - (top - 8 * 4096) < len + 1024) return 0;
        sp -= len;
        if (mm_copy_to(mm, sp, argv[argc], len) < 0) return 0;
        argv_addr[argc++] = sp;
    }
    while (envp && envp[envc]) {
        if (envc == 32) return 0;
        usize len = strlen(envp[envc]) + 1;
        if (len > 4096 || sp - (top - 8 * 4096) < len + 1024) return 0;
        sp -= len;
        if (mm_copy_to(mm, sp, envp[envc], len) < 0) return 0;
        env_addr[envc++] = sp;
    }
    int k = 0;
    words[k++] = (u64)argc;
    for (int i = 0; i < argc; i++) words[k++] = argv_addr[i];
    words[k++] = 0;
    for (int i = 0; i < envc; i++) words[k++] = env_addr[i];
    words[k++] = 0;
    sp = (sp - (u64)k * 8) & ~15ULL;
    if (mm_copy_to(mm, sp, words, (usize)k * 8) < 0) return 0;
    return sp;
}
