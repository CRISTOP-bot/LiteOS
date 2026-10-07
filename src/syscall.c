#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "kernel.h"

extern struct proc *current;
#define UIO_CHUNK 1024
#define PATH_LIMIT 256
#define ARG_LIMIT 16
#define ARG_LEN 256

/* Physical identity map is supervisor-only; user pointers are never dereferenced. */
int mm_copy_to(struct mm *mm, u64 dst, const void *src, usize n)
{
    if (!mm || (n && dst + n - 1 < dst)) return -EFAULT;
    const u8 *s = src;
    while (n) {
        u64 addr = vm_query(mm->pml4, dst, NULL);
        if (!addr) return -EFAULT;
        usize take = 4096 - (dst & 4095);
        if (take > n) take = n;
        memcpy((void *)addr, s, take);
        dst += take;
        s += take;
        n -= take;
    }
    return 0;
}

int user_copy_from(void *dst, const void *src, usize n)
{
    u64 a = (u64)src;
    if (!current || !current->mm || (n && a + n - 1 < a)) return -EFAULT;
    u8 *d = dst;
    while (n) {
        u64 phys = vm_query(current->mm->pml4, a, NULL);
        if (!phys) return -EFAULT;
        usize take = 4096 - (a & 4095);
        if (take > n) take = n;
        memcpy(d, (void *)phys, take);
        a += take;
        d += take;
        n -= take;
    }
    return 0;
}

int user_copy_to(void *dst, const void *src, usize n)
{
    return mm_copy_to(current ? current->mm : NULL, (u64)dst, src, n);
}

static int copy_string(char *out, const char *src, usize cap)
{
    if (!src) return -EFAULT;
    for (usize i = 0; i < cap; i++) {
        if (user_copy_from(&out[i], src + i, 1) < 0) return -EFAULT;
        if (!out[i]) return 0;
    }
    return -ENAMETOOLONG;
}

static struct file *get_file(int fd)
{
    if (fd < 0 || fd >= PROC_FDS) return NULL;
    return current->fds[fd];
}

static struct vnode *path_parent(char *path, const char **name)
{
    char *slash = NULL;
    for (char *c = path; *c; c++) if (*c == '/') slash = c;
    if (!slash) {
        *name = path;
        return vref(current->cwd);
    }
    *name = slash + 1;
    if (!**name) return NULL;
    if (slash == path) return vfs_lookup("/", current->cwd);
    *slash = 0;
    return vfs_lookup(path, current->cwd);
}

static long sys_open_file(const char *user_path, int flags)
{
    char path[PATH_LIMIT];
    int err = copy_string(path, user_path, sizeof(path));
    if (err) return err;
    struct vnode *vn = vfs_lookup(path, current->cwd);
    if (!vn && (flags & O_CREAT)) {
        const char *name;
        struct vnode *parent = path_parent(path, &name);
        if (!parent) return -ENOENT;
        if (!*name || strlen(name) > NAME_MAX ||
            !strcmp(name, ".") || !strcmp(name, "..")) {
            vrelease(parent);
            return -EINVAL;
        }
        vn = vfs_create(parent, name, V_REG);
        vrelease(parent);
    }
    if (!vn) return -ENOENT;
    if (vn->type == V_DIR && (flags & O_ACCMODE) != O_RDONLY) {
        vrelease(vn);
        return -EISDIR;
    }
    if (vn->type == V_REG && (flags & O_TRUNC) &&
        (flags & O_ACCMODE) != O_RDONLY) {
        kfree(vn->data);
        vn->data = NULL;
        vn->size = 0;
    }
    struct file *f = kmalloc(sizeof(*f));
    if (!f) { vrelease(vn); return -ENOMEM; }
    memset(f, 0, sizeof(*f));
    f->vn = vn;
    f->flags = flags;
    if (flags & O_APPEND) f->off = vn->size;
    f->ref = 1;
    int fd = proc_fd_alloc(f);
    if (fd < 0) { vrelease(vn); kfree(f); }
    return fd;
}

static long sys_io(int fd, void *buf, usize len, int write)
{
    struct file *f = get_file(fd);
    if (!f) return -EBADF;
    if (write && (f->flags & O_ACCMODE) == O_RDONLY) return -EBADF;
    if (!write && (f->flags & O_ACCMODE) == O_WRONLY) return -EBADF;
    if (!f->vn->ops || (write && !f->vn->ops->write) ||
        (!write && !f->vn->ops->read))
        return -EINVAL;
    u8 tmp[UIO_CHUNK];
    usize done = 0;
    while (done < len) {
        usize chunk = len - done < sizeof(tmp) ? len - done : sizeof(tmp);
        if (write && user_copy_from(tmp, (u8 *)buf + done, chunk) < 0)
            return done ? (long)done : -EFAULT;
        ssize_t r = write ? f->vn->ops->write(f->vn, f->off, tmp, chunk)
                          : f->vn->ops->read(f->vn, f->off, tmp, chunk);
        if (r < 0) return done ? (long)done : r;
        if (!write && user_copy_to((u8 *)buf + done, tmp, (usize)r) < 0)
            return done ? (long)done : -EFAULT;
        f->off += (u64)r;
        done += (usize)r;
        if ((usize)r != chunk || !r) break;
    }
    return (long)done;
}

static long sys_change_node(const char *user_path, int is_mkdir)
{
    char path[PATH_LIMIT];
    int rc = copy_string(path, user_path, sizeof(path));
    if (rc) return rc;
    const char *name;
    struct vnode *parent = path_parent(path, &name);
    if (!parent) return -ENOENT;
    if (parent->type != V_DIR) { vrelease(parent); return -ENOTDIR; }
    if (!*name || strlen(name) > NAME_MAX ||
        !strcmp(name, ".") || !strcmp(name, "..")) {
        vrelease(parent);
        return -EINVAL;
    }
    if (is_mkdir) {
        struct vnode *vn = vfs_mkdir(parent, name);
        rc = vn ? 0 : -EEXIST;
        if (vn) vrelease(vn);
    } else {
        rc = vfs_remove(parent, name, 0);
    }
    vrelease(parent);
    return rc;
}

static int copy_args(char *args[ARG_LIMIT + 1], u64 addr)
{
    for (int i = 0; i <= ARG_LIMIT; i++) args[i] = NULL;
    if (!addr) return 0;
    int i = 0;
    for (; i < ARG_LIMIT; i++) {
        u64 ptr;
        if (user_copy_from(&ptr, (void *)(addr + (u64)i * 8), 8) < 0)
            goto err;
        if (!ptr) return 0;
        args[i] = kmalloc(ARG_LEN);
        if (!args[i]) goto err;
        int rc = copy_string(args[i], (const char *)ptr, ARG_LEN);
        if (rc) goto err;
    }
    return -E2BIG;
err:
    for (int j = 0; j < i; j++) kfree(args[j]);
    return -ENOMEM;
}

static void free_args(char *args[ARG_LIMIT + 1])
{
    for (int i = 0; i < ARG_LIMIT; i++) kfree(args[i]);
}

static long sys_exec(struct trap_frame *tf, const char *user_path,
                     u64 argv_ptr, u64 envp_ptr)
{
    char path[PATH_LIMIT];
    int rc = copy_string(path, user_path, sizeof(path));
    if (rc) return rc;
    struct vnode *vn = vfs_lookup(path, current->cwd);
    if (!vn) return -ENOENT;
    if (vn->type != V_REG) { vrelease(vn); return -EACCES; }
    char *argv[ARG_LIMIT + 1], *envp[ARG_LIMIT + 1];
    rc = copy_args(argv, argv_ptr);
    if (!rc) rc = copy_args(envp, envp_ptr);
    else memset(envp, 0, sizeof(envp));
    if (rc) goto done;
    struct mm *mm = mm_create();
    if (!mm) { rc = -ENOMEM; goto done; }
    u64 entry = 0;
    rc = elf_load(vn->data, vn->size, mm, &entry);
    if (rc) { mm_destroy(mm); goto done; }
    u64 sp = setup_user_stack(mm, argv, envp);
    if (!sp) { mm_destroy(mm); rc = -ENOMEM; goto done; }
    struct mm *old = current->mm;
    current->mm = mm;
    cr3_load(mm->pml4);
    mm_destroy(old);
    tf->rip = entry;
    tf->rsp = sp;
    tf->rax = 0;
    rc = 0;
done:
    free_args(argv);
    free_args(envp);
    vrelease(vn);
    return rc;
}

static long sys_getcwd_path(char *buf, usize size)
{
    char path[PATH_LIMIT];
    usize pos = sizeof(path);
    path[--pos] = 0;
    struct vnode *dir = current->cwd;
    while (dir && dir != vfs_root()) {
        usize len = strlen(dir->name);
        if (len + 1 > pos) return -ERANGE;
        pos -= len;
        memcpy(path + pos, dir->name, len);
        path[--pos] = '/';
        dir = dir->parent;
    }
    if (pos == sizeof(path) - 1) path[--pos] = '/';
    usize len = sizeof(path) - pos;
    if (len > size) return -ERANGE;
    if (user_copy_to(buf, path + pos, len) < 0) return -EFAULT;
    return (long)len;
}

struct dirent64 {
    u64 ino;
    s64 off;
    u16 reclen;
    u8 type;
    char name[];
} __attribute__((packed));

static long sys_getdents(int fd, void *out, usize size)
{
    struct file *f = get_file(fd);
    if (!f) return -EBADF;
    if (f->vn->type != V_DIR) return -ENOTDIR;
    if (size > 4096) size = 4096;
    u8 bytes[4096];
    usize used = 0;
    u64 index = 0;
    for (struct vnode *v = f->vn->child; v; v = v->next, index++) {
        if (index < f->off || v->dead) continue;
        usize reclen = (offsetof(struct dirent64, name) + strlen(v->name) + 1 + 7) & ~7UL;
        if (reclen > size - used) break;
        struct dirent64 *d = (struct dirent64 *)(bytes + used);
        memset(d, 0, reclen);
        d->ino = (u64)v;
        d->off = (s64)(index + 1);
        d->reclen = (u16)reclen;
        d->type = v->type == V_DIR ? 4 : v->type == V_DEV ? 2 : 8;
        strcpy(d->name, v->name);
        used += reclen;
        f->off = index + 1;
    }
    if (user_copy_to(out, bytes, used) < 0) return -EFAULT;
    return (long)used;
}

void syscall_dispatch(struct trap_frame *tf)
{
    long r = -ENOSYS;
    switch (tf->rax) {
    case 0: r = sys_io((int)tf->rdi, (void *)tf->rsi, tf->rdx, 0); break;
    case 1: r = sys_io((int)tf->rdi, (void *)tf->rsi, tf->rdx, 1); break;
    case 2: r = sys_open_file((const char *)tf->rdi, (int)tf->rsi); break;
    case 3:
        if (get_file((int)tf->rdi)) {
            proc_fd_close((int)tf->rdi);
            r = 0;
        } else {
            r = -EBADF;
        }
        break;
    case 22: {
        int fds[2];
        r = pipe_create_fds(fds);
        if (!r && user_copy_to((void *)tf->rdi, fds, sizeof(fds)) < 0) {
            proc_fd_close(fds[0]); proc_fd_close(fds[1]); r = -EFAULT;
        }
        break;
    }
    case 24: schedule(); r = 0; break;
    case 33: {
        int old = (int)tf->rdi, newfd = (int)tf->rsi;
        struct file *f = get_file(old);
        if (!f || newfd < 0 || newfd >= PROC_FDS) { r = -EBADF; break; }
        if (old != newfd) {
            proc_fd_close(newfd);
            current->fds[newfd] = f;
            f->ref++;
            if (f->vn->type == V_PIPE)
                pipe_adjust(f, (f->flags & O_ACCMODE) == O_RDONLY,
                               (f->flags & O_ACCMODE) == O_WRONLY);
        }
        r = newfd;
        break;
    }
    case 39: r = current->pid; break;
    case 57: r = proc_fork(tf); break;
    case 59: r = sys_exec(tf, (const char *)tf->rdi, tf->rsi, tf->rdx); break;
    case 60: proc_exit((int)tf->rdi); break;
    case 61: {
        int status = 0;
        r = proc_wait((int)tf->rdi, &status);
        if (r > 0 && tf->rsi && user_copy_to((void *)tf->rsi, &status, sizeof(status)) < 0)
            r = -EFAULT;
        break;
    }
    case 79: r = sys_getcwd_path((char *)tf->rdi, tf->rsi); break;
    case 80: {
        char path[PATH_LIMIT];
        r = copy_string(path, (const char *)tf->rdi, sizeof(path));
        if (r) break;
        struct vnode *vn = vfs_lookup(path, current->cwd);
        if (!vn) { r = -ENOENT; break; }
        if (vn->type != V_DIR) { vrelease(vn); r = -ENOTDIR; break; }
        vrelease(current->cwd);
        current->cwd = vn;
        r = 0;
        break;
    }
    case 83: r = sys_change_node((const char *)tf->rdi, 1); break;
    case 87: r = sys_change_node((const char *)tf->rdi, 0); break;
    case 217: r = sys_getdents((int)tf->rdi, (void *)tf->rsi, tf->rdx); break;
    default: break;
    }
    tf->rax = (u64)r;
}
