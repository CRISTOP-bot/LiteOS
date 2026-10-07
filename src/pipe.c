/* LiteOS: pipes del kernel (buffer circular con bloqueo).
 * Los nodos de pipe son anonimos (dead=1): se liberan
 * automaticamente cuando el ultimo descriptor los suelta. */
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "kernel.h"
extern struct proc *current;

#define PIPE_BUF 2048

struct pipe {
    uint8_t buf[PIPE_BUF];
    usize head;
    usize count;
    int readers;
    int writers;
};

static ssize_t pipe_read(struct vnode *vn, u64 off, void *buf, usize n)
{
    (void)off;
    struct pipe *p = vn->pipe;
    if (n == 0)
        return 0;

    for (;;) {
        if (p->count > 0)
            break;
        if (p->writers == 0)
            return 0;                    /* EOF: escritores cerrados */
        current->state = PS_SLEEPING;
        current->wchan = p;
        schedule();
    }

    usize take = p->count < n ? p->count : n;
    for (usize i = 0; i < take; i++)
        ((uint8_t *)buf)[i] = p->buf[(p->head + i) % PIPE_BUF];
    p->head = (p->head + take) % PIPE_BUF;
    p->count -= take;

    proc_wakeup(p);                     /* espacio libre para escritores */
    return (ssize_t)take;
}

static ssize_t pipe_write(struct vnode *vn, u64 off, const void *buf, usize n)
{
    (void)off;
    struct pipe *p = vn->pipe;
    if (p->readers == 0)
        return -EPIPE;

    usize done = 0;
    while (done < n) {
        while (p->count == PIPE_BUF) {
            current->state = PS_SLEEPING;
            current->wchan = p;
            schedule();
            if (p->readers == 0)
                return done > 0 ? (ssize_t)done : -EPIPE;
        }
        usize space = PIPE_BUF - p->count;
        usize chunk = (n - done < space) ? n - done : space;
        for (usize i = 0; i < chunk; i++)
            p->buf[(p->head + p->count + i) % PIPE_BUF] =
                ((const uint8_t *)buf)[done + i];
        p->count += chunk;
        done += chunk;
        proc_wakeup(p);                 /* datos disponibles para lectores */
    }
    return (ssize_t)done;
}

const struct vops pipe_ops = {
    .read  = pipe_read,
    .write = pipe_write,
};

/* Ajusta contadores de extremos abiertos al cerrar un file. */
void pipe_adjust(struct file *f, int delta_readers, int delta_writers)
{
    if (!f || !f->vn || f->vn->type != V_PIPE)
        return;
    struct pipe *p = f->vn->pipe;
    if (delta_readers)
        p->readers += delta_readers;
    if (delta_writers)
        p->writers += delta_writers;
    proc_wakeup(p);
}

/* Crea un par de descriptores: fds[0] lectura, fds[1] escritura. */
int pipe_create_fds(int fds[2])
{
    struct pipe *p = kmalloc(sizeof(*p));
    if (!p)
        return -ENOMEM;
    memset(p, 0, sizeof(*p));

    struct vnode *vn = kmalloc(sizeof(*vn));
    if (!vn) {
        kfree(p);
        return -ENOMEM;
    }
    memset(vn, 0, sizeof(*vn));
    vn->type = V_PIPE;
    vn->ref = 1;
    vn->dead = 1;                    /* anonimo: libre al soltar refs */
    vn->pipe = p;
    vn->ops = &pipe_ops;
    p->readers = 1;
    p->writers = 1;

    struct file *fr = kmalloc(sizeof(*fr));
    struct file *fw = kmalloc(sizeof(*fw));
    if (!fr || !fw) {
        kfree(fr);
        kfree(fw);
        vrelease(vn);
        return -ENOMEM;
    }
    memset(fr, 0, sizeof(*fr));
    memset(fw, 0, sizeof(*fw));
    fr->vn = vref(vn);
    fr->ref = 1;
    fr->flags = O_RDONLY;
    fw->vn = vref(vn);
    fw->ref = 1;
    fw->flags = O_WRONLY;

    fds[0] = proc_fd_alloc(fr);
    if (fds[0] < 0) {
        vrelease(fr->vn);
        kfree(fr);
        vrelease(fw->vn);
        kfree(fw);
        vrelease(vn);
        return fds[0];
    }
    fds[1] = proc_fd_alloc(fw);
    if (fds[1] < 0) {
        proc_fd_close(fds[0]);
        vrelease(fw->vn);
        kfree(fw);
        vrelease(vn);
        return -ENOMEM;
    }
    vrelease(vn);                    /* refs de los files lo mantienen vivo */
    return 0;
}
