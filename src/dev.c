/* LiteOS: nodos de dispositivo (/dev) y operaciones asociadas. */
#include <string.h>
#include <errno.h>
#include "kernel.h"

enum {
    DEV_CONSOLE = 0,
    DEV_TTY,
    DEV_NULL,
    DEV_ZERO,
    DEV_RANDOM,
    DEV_URANDOM
};

static ssize_t dev_read(struct vnode *vn, u64 off, void *buf, usize n)
{
    (void)off;
    switch (vn->dev) {
    case DEV_CONSOLE:
    case DEV_TTY:
        return tty_read(off, buf, n);
    case DEV_ZERO:
        memset(buf, 0, n);
        return (ssize_t)n;
    case DEV_RANDOM:
    case DEV_URANDOM:
        /* Never expose the old RTC-seeded xorshift as random data. */
        return -ENOSYS;
    case DEV_NULL:
    default:
        return 0;
    }
}

static ssize_t dev_write(struct vnode *vn, u64 off, const void *buf, usize n)
{
    (void)off;
    switch (vn->dev) {
    case DEV_CONSOLE:
    case DEV_TTY:
        return tty_write(off, buf, n);
    case DEV_NULL:
    case DEV_ZERO:
    case DEV_RANDOM:
    case DEV_URANDOM:
    default:
        return (ssize_t)n;            /* descartar */
    }
}

static const struct vops dev_ops = {
    .read  = dev_read,
    .write = dev_write,
};

__attribute__((unused)) static struct vnode *dev_make(const char *name, u32 dev)
{
    struct vnode *vn = vfs_create(vfs_root(), name, V_DEV);
    if (!vn)
        return NULL;
    vn->dev = dev;
    vn->ops = &dev_ops;
    return vn;
}

void dev_init(void)
{
    struct vnode *dev = vfs_create(vfs_root(), "dev", V_DIR);
    if (dev)
        vrelease(dev);

    /* /dev ya existe (del initramfs): crear los nodos bajo el
     * directorio real del VFS */
    struct vnode *dir = vfs_lookup("/dev", NULL);
    if (!dir) {
        serial_puts("FATAL: /dev no existe en el VFS\n");
        qemu_exit(0x23);
        for (;;) __asm__ volatile ("hlt");
    }

    struct vnode *vn;
    vn = vfs_create(dir, "console", V_DEV);
    if (vn) { vn->dev = DEV_CONSOLE; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "tty", V_DEV);
    if (vn) { vn->dev = DEV_TTY; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "tty0", V_DEV);
    if (vn) { vn->dev = DEV_TTY; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "null", V_DEV);
    if (vn) { vn->dev = DEV_NULL; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "zero", V_DEV);
    if (vn) { vn->dev = DEV_ZERO; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "random", V_DEV);
    if (vn) { vn->dev = DEV_RANDOM; vn->ops = &dev_ops; vrelease(vn); }
    vn = vfs_create(dir, "urandom", V_DEV);
    if (vn) { vn->dev = DEV_URANDOM; vn->ops = &dev_ops; vrelease(vn); }

    vrelease(dir);
}
