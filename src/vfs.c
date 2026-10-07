/* LiteOS: VFS con sistema de archivos en memoria (ramfs).
 * Un unico VFS con nodos: directorios (ramfs), archivos
 * regulares (ramfs), dispositivos (dev) y pipes. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "kernel.h"

static struct vnode vfs_root_node;
static struct vnode *root = &vfs_root_node;

struct vnode *vfs_root(void)
{
    return root;
}

struct vnode *vref(struct vnode *vn)
{
    vn->ref++;
    return vn;
}

void vrelease(struct vnode *vn)
{
    if (--vn->ref > 0 || vn == root)
        return;
    if (!vn->dead)
        return;
    /* Sin refs y desvinculado: liberar recursos */
    if (vn->type == V_REG && vn->data)
        kfree(vn->data);
    if (vn->type == V_PIPE && vn->pipe)
        kfree(vn->pipe);
    kfree(vn);
}

static struct vnode *vfs_find_child(struct vnode *dir, const char *name)
{
    if (dir->type != V_DIR)
        return NULL;
    for (struct vnode *c = dir->child; c; c = c->next) {
        if (c->dead)
            continue;
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}

/* Resuelve una ruta absoluta o relativa a cwd. */
struct vnode *vfs_lookup(const char *path, struct vnode *cwd)
{
    if (!path || !*path)
        return NULL;

    struct vnode *v = (*path == '/') ? root : (cwd ? cwd : root);
    vref(v);

    while (*path) {
        while (*path == '/')
            path++;
        if (!*path)
            break;

        char comp[NAME_MAX + 1];
        usize i = 0;
        while (*path && *path != '/' && i < NAME_MAX)
            comp[i++] = *path++;
        comp[i] = '\0';

        struct vnode *next = NULL;
        if (strcmp(comp, ".") == 0) {
            next = v;
            vref(next);
        } else if (strcmp(comp, "..") == 0) {
            next = v->parent ? v->parent : v;
            vref(next);
        } else {
            next = vfs_find_child(v, comp);
            if (next)
                vref(next);
        }
        vrelease(v);
        v = next;
        if (!v)
            return NULL;
    }
    return v;
}

struct vnode *vfs_create(struct vnode *dir, const char *name, u32 type)
{
    if (dir->type != V_DIR || !name || !*name)
        return NULL;
    if (vfs_find_child(dir, name))
        return NULL;

    struct vnode *vn = kmalloc(sizeof(*vn));
    if (!vn)
        return NULL;
    memset(vn, 0, sizeof(*vn));
    vn->type = type;
    vn->ref = 1;
    strncpy(vn->name, name, NAME_MAX);
    vn->parent = dir;
    vn->next = dir->child;
    dir->child = vn;

    if (type == V_REG) {
        vn->ops = &ramfs_file_ops;
    } else if (type == V_DIR) {
        vn->ops = &ramfs_dir_ops;
    }
    return vn;
}

/* Desvincula un hijo del directorio. Si es directorio,
 * debe estar vacio. */
int vfs_remove(struct vnode *dir, const char *name, int is_dir)
{
    if (dir->type != V_DIR)
        return -ENOTDIR;
    struct vnode **link = &dir->child;
    while (*link) {
        struct vnode *vn = *link;
        if (!vn->dead && strcmp(vn->name, name) == 0) {
            if (is_dir && vn->type != V_DIR)
                return -ENOTDIR;
            if (!is_dir && vn->type == V_DIR)
                return -EISDIR;
            if (vn->type == V_DIR && vn->child)
                return -ENOTEMPTY;
            *link = vn->next;
            vn->parent = NULL;
            vn->dead = 1;
            vrelease(vn);
            return 0;
        }
        link = &vn->next;
    }
    return -ENOENT;
}

struct vnode *vfs_mkdir(struct vnode *dir, const char *name)
{
    return vfs_create(dir, name, V_DIR);
}

/* ---- Operaciones ramfs ---- */

ssize_t ramfs_file_read(struct vnode *vn, u64 off, void *buf, usize n)
{
    if (off >= vn->size)
        return 0;
    if (off + n > vn->size)
        n = (usize)(vn->size - off);
    memcpy(buf, vn->data + off, n);
    return (ssize_t)n;
}

ssize_t ramfs_file_write(struct vnode *vn, u64 off, const void *buf, usize n)
{
    if (off + n > vn->size) {
        u64 new_size = off + n;
        u8 *nd = kmalloc((usize)new_size);
        if (!nd)
            return -ENOMEM;
        memset(nd, 0, (usize)new_size);
        if (vn->data) {
            memcpy(nd, vn->data, (usize)vn->size);
            kfree(vn->data);
        }
        vn->data = nd;
        vn->size = new_size;
    }
    memcpy(vn->data + off, buf, n);
    return (ssize_t)n;
}

static ssize_t ramfs_dir_read(struct vnode *vn, u64 off, void *buf, usize n)
{
    (void)vn; (void)off; (void)buf; (void)n;
    return -EISDIR;
}

static ssize_t ramfs_dir_write(struct vnode *vn, u64 off, const void *buf, usize n)
{
    (void)vn; (void)off; (void)buf; (void)n;
    return -EISDIR;
}

const struct vops ramfs_file_ops = {
    .read  = ramfs_file_read,
    .write = ramfs_file_write,
};

const struct vops ramfs_dir_ops = {
    .read  = ramfs_dir_read,
    .write = ramfs_dir_write,
};

void vfs_init(void)
{
    memset(root, 0, sizeof(*root));
    root->type = V_DIR;
    root->ref = 1;
    strcpy(root->name, "/");
    root->ops = &ramfs_dir_ops;
}

/* ---- initramfs: cpio "newc" ---- */

static u32 cphex(const char *s, int digits)
{
    u32 v = 0;
    for (int i = 0; i < digits; i++) {
        char c = s[i];
        v <<= 4;
        v |= (u32)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
    }
    return v;
}

void initramfs_load(const u8 *img, u64 size)
{
    u64 off = 0;
    u64 files = 0, bytes = 0;

    while (off + 110 <= size) {
        const char *h = (const char *)(img + off);
        if (memcmp(h, "070701", 6) != 0)
            break;

        u32 filesize = cphex(h + 6 + 8 * 6, 8);    /* filesize */
        u32 namesize = cphex(h + 6 + 8 * 11, 8);   /* namesize */
        u32 mode     = cphex(h + 6 + 8 * 1, 8);    /* mode */

        u64 name_off = off + 110;
        char name[NAME_MAX + 1];
        if (!namesize || name_off + namesize > size)
            break;
        u64 data_off = (name_off + namesize + 3) & ~(u64)3;
        u64 next_off = (data_off + filesize + 3) & ~(u64)3;
        if (next_off > size || next_off < data_off)
            break;
        if (namesize > NAME_MAX)
            goto next;
        memcpy(name, img + name_off, namesize);
        name[namesize - 1] = '\0';

        if (strcmp(name, "TRAILER!!!") == 0)
            break;

        /* Crea la ruta completa en el VFS */
        char full[NAME_MAX * 2 + 2];
        if (snprintf(full, sizeof(full), "/%s", name) >= (int)sizeof(full))
            goto next;
        if (strcmp(full, "/dev") == 0)
            goto next;               /* /dev lo crea dev_init */

        /* Directorio padre */
        char *slash = 0;
        for (char *q = full; *q; q++) if (*q == '/') slash = q;
        if (!slash)
            goto next;
        *slash = '\0';
        struct vnode *dir = vfs_lookup(slash == full ? "/" : full, NULL);
        if (!dir || dir->type != V_DIR) {
            if (dir)
                vrelease(dir);
            goto next;
        }

        *slash = '/';
        const char *base = slash + 1;
        u32 type = S_ISDIR(mode) ? V_DIR : V_REG;
        struct vnode *vn = vfs_create(dir, base, type);
        if (vn && type == V_REG && filesize > 0) {
            vn->data = kmalloc((usize)filesize);
            if (vn->data) {
                memcpy(vn->data, img + data_off, (usize)filesize);
                vn->size = filesize;
                bytes += filesize;
            }
        }
        if (vn)
            vrelease(vn);
        vrelease(dir);
        files++;

next:
        off = next_off;
    }

    kvprintf("initramfs: %u archivos, %u bytes\n",
             (uint32_t)files, (uint32_t)bytes);
}
