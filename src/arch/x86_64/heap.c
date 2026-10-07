/* LiteOS: heap del kernel (kmalloc/kfree) sobre paginas
 * del gestor de memoria fisica. Las paginas de identidad
 * (boot.S) hacen que direccion fisica == virtual, asi que
 * cada bloque es directamente direccionable. */
#include <stdint.h>
#include <string.h>
#include "kernel.h"

#define HEAP_CHUNK_PAGES  16          /* 64 KiB por crecimiento */

typedef struct kblock {
    uint64_t size;                    /* payload en bytes */
    uint64_t free;
    struct kblock *next;
} kblock_t;

#define KHDR sizeof(kblock_t)

static kblock_t *heap_head;

static void heap_grow(void)
{
    uint64_t frames[HEAP_CHUNK_PAGES];
    int got = 0;

    for (int i = 0; i < HEAP_CHUNK_PAGES; i++) {
        uint64_t page = pmm_alloc();
        if (!page) {
            for (int j = 0; j < got; j++)
                pmm_free(frames[j]);
            serial_puts("FATAL: kmalloc sin memoria fisica\n");
            qemu_exit(0x22);
            for (;;) __asm__ volatile ("hlt");
        }
        frames[got++] = page;
    }

    for (int i = 0; i < got; i++) {
        kblock_t *b = (kblock_t *)frames[i];
        b->size = 4096 - KHDR;
        b->free = 1;
        b->next = heap_head;
        heap_head = b;
    }
}

void heap_init(void)
{
    heap_head = NULL;
    heap_grow();
}

void *kmalloc(usize size)
{
    if (size == 0)
        return NULL;
    if (size > (usize)-1 - KHDR - 4095) return NULL;
    size = (size + 15u) & ~15u;
    if (size > 4096 - KHDR) {
        usize pages = (size + KHDR + 4095) / 4096;
        u64 frame = pmm_alloc_pages(pages);
        if (!frame) return NULL;
        kblock_t *large = (kblock_t *)frame;
        large->size = pages;
        large->free = 2;
        large->next = NULL;
        return (u8 *)large + KHDR;
    }

    kblock_t *best = NULL, *prev = NULL, *p = heap_head, *prev_best = NULL;
    while (p) {
        if (p->free && p->size >= size) {
            if (!best || p->size < best->size) {
                best = p;
                prev_best = prev;
            }
        }
        prev = p;
        p = p->next;
    }

    if (!best) {
        heap_grow();
        return kmalloc(size);
    }

    if (best->size >= size + KHDR + 16) {
        kblock_t *split = (kblock_t *)((uint8_t *)best + KHDR + size);
        split->size = best->size - size - KHDR;
        split->free = 1;
        split->next = best->next;
        best->size = size;
        best->next = split;
    }
    best->free = 0;
    if (prev_best) {
        prev_best->next = best;
    } else {
        heap_head = best;
    }
    return (void *)((uint8_t *)best + KHDR);
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    kblock_t *b = (kblock_t *)((uint8_t *)ptr - KHDR);
    if (b->free == 2) {
        for (usize i = 0; i < b->size; i++)
            pmm_free((u64)b + i * 4096);
        return;
    }
    b->free = 1;
    if (b->next && b->next->free == 1 &&
        (u8 *)b + KHDR + b->size == (u8 *)b->next) {
        b->size += KHDR + b->next->size;
        b->next = b->next->next;
    }
}
