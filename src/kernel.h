/* LiteOS: declaraciones centrales del kernel. */
#ifndef LITEOS_KERNEL_H
#define LITEOS_KERNEL_H

#include "types.h"

/* ---- Estructuras ELF64 (subset) ---- */
typedef struct {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} Elf64_Phdr;

#define ELF_MAGIC      0x464c457f
#define ET_EXEC        2
#define PT_LOAD        1
#define EI_CLASS       4
#define ELFCLASS64     2
#define EI_DATA        5
#define ELSDATA2LSB    1

/* ---- Frame de trampa unificado (ver isrs.S) ---- */
struct trap_frame {
    u64 rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11;
    u64 r15, r14, r13, r12, rbx, rbp;
    u64 vector, err;
    u64 rip, cs, rflags, rsp, ss;
};

/* ---- VFS ---- */
#define V_REG   1
#define V_DIR   2
#define V_DEV   3
#define V_PIPE  4

#define NAME_MAX 63

struct vnode;
struct pipe;

typedef ssize_t (*vop_read)(struct vnode *vn, u64 off, void *buf, usize n);
typedef ssize_t (*vop_write)(struct vnode *vn, u64 off, const void *buf, usize n);

struct vops {
    vop_read  read;
    vop_write write;
};

struct vnode {
    u32 type;
    char name[NAME_MAX + 1];
    struct vnode *parent;
    struct vnode *child;   /* primer hijo */
    struct vnode *next;    /* siguiente hermano */
    int ref;
    int dead;              /* desvinculado pero con refs abiertas */
    u64 size;
    u8 *data;              /* V_REG */
    u32 dev;               /* V_DEV */
    struct pipe *pipe;     /* V_PIPE */
    const struct vops *ops;
};

struct file {
    struct vnode *vn;
    u64 off;
    int ref;
    int flags;
};

/* ---- Procesos ---- */
#define NPROC      64
#define PROC_FDS   64
#define KSTACK_SZ  8192

enum proc_state {
    PS_FREE = 0,
    PS_RUNNABLE,
    PS_RUNNING,
    PS_SLEEPING,
    PS_ZOMBIE
};

struct mm;

struct proc {
    u64 ksp;                       /* offset 0: salvo por switch_to (asm) */
    int pid;
    int state;
    char name[32];
    struct mm *mm;
    struct file *fds[PROC_FDS];
    struct vnode *cwd;
    struct proc *parent;
    int exit_code;
    void *wchan;
    int killed;
    int kill_sig;
    u64 kstack;                    /* tope de la pila del kernel */
};

struct mm {
    u64 pml4;
    u64 brk;
    u64 mmap_top;
};

/* ---- Señales (subset) ---- */
#define SIGHUP    1
#define SIGINT    2
#define SIGKILL   9
#define SIGTERM   15
#define SIGSEGV   11

/* ---- E/S ---- */
void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_hex(u64 v);
void kvprintf(const char *fmt, ...);

void vga_init(void);
void vga_putc(char c);
void vga_puts(const char *s);

/* ---- CPU / arch ---- */
void gdt_init(void);
void tss_set_rsp0(u64 rsp0);
void idt_init(void);
void pic_init(void);
void pic_eoi(int irq);
void timer_init(void);
void timer_tick(void);
void tsc_calibrate(void);
u64  tsc_hz(void);
u64  uptime_ns(void);
void keyboard_init(void);
void keyboard_irq(void);
void rtc_init(void);
u64  rtc_boot_epoch(void);

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %0, %1" :: "a"(val), "d"(port));
}

static inline u8 inb(u16 port)
{
    u8 v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "d"(port));
    return v;
}

static inline void irq_disable(void)
{
    __asm__ volatile ("cli");
}

static inline void irq_enable(void)
{
    __asm__ volatile ("sti");
}

static inline u64 cr2_read(void)
{
    u64 v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

static inline void cr3_load(u64 pml4)
{
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pml4));
}

static inline u64 rdtsc(void)
{
    u32 lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

/* ---- Memoria fisica y heap ---- */
void pmm_init(u64 mbi);
u64  pmm_alloc(void);
u64  pmm_alloc_pages(usize n);
void pmm_free(u64 page);
u64  pmm_free_count(void);

void heap_init(void);
void *kmalloc(usize n);
void kfree(void *ptr);

/* ---- Paginacion ---- */
extern u64 kernel_pml4;              /* == pml4_table (identidad) */
int  map_page(u64 pml4, u64 vaddr, u64 paddr, u64 flags);
int  unmap_page(u64 pml4, u64 vaddr);
u64  vm_query(u64 pml4, u64 vaddr, u64 *flags);
struct mm *mm_create(void);
void mm_destroy(struct mm *mm);
u64  mm_clone(u64 src_pml4);

/* ---- VFS ---- */
void vfs_init(void);
struct vnode *vfs_root(void);
struct vnode *vfs_lookup(const char *path, struct vnode *cwd);
struct vnode *vfs_create(struct vnode *dir, const char *name, u32 type);
int vfs_remove(struct vnode *dir, const char *name, int is_dir);
struct vnode *vfs_mkdir(struct vnode *dir, const char *name);
struct vnode *vref(struct vnode *vn);
void vrelease(struct vnode *vn);
void initramfs_load(const u8 *img, u64 size);
void dev_init(void);

extern const struct vops ramfs_file_ops;
extern const struct vops ramfs_dir_ops;

/* ---- TTY ---- */
void tty_init(void);
void tty_input(char c);
ssize_t tty_read(u64 off, void *buf, usize n);
ssize_t tty_write(u64 off, const void *buf, usize n);

/* ---- Pipes ---- */
int  pipe_create_fds(int fds[2]);
void pipe_adjust(struct file *f, int delta_readers, int delta_writers);

/* ---- Procesos / scheduler ---- */
void proc_init(void);
void schedule(void);
struct proc *proc_self(void);
int  proc_fork(struct trap_frame *tf);
int  proc_start_init(const char *path);
void proc_exit(int code);
int  proc_wait(int pid, int *status);
int  proc_kill(int pid, int sig);
void proc_wakeup(void *wchan);
int  proc_fd_alloc(struct file *f);
void proc_fd_close(int fd);
u64  setup_user_stack(struct mm *mm, char *const argv[], char *const envp[]);
void switch_to(struct proc *next);

/* ---- Syscalls ---- */
void trap_handler(struct trap_frame *f);
long sys_debug_exit(int code);

/* ---- ELF ---- */
int elf_load(const u8 *img, u64 size, struct mm *mm, u64 *entry);
int user_copy_from(void *dst, const void *src, usize n);
int user_copy_to(void *dst, const void *src, usize n);
int mm_copy_to(struct mm *mm, u64 dst, const void *src, usize n);

/* ---- Simbolos del enlazador ---- */
extern u8 _stack_top;
extern u8 _binary_initramfs_cpio_start[];
extern u8 _binary_initramfs_cpio_end[];

/* ---- Contexto de la primera entrada a usuario ---- */
extern struct trap_frame *first_user_frame;

/* ---- qemu isa-debug-exit ---- */
void qemu_exit(u8 code);

#endif
