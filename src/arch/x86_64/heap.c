#include <stdint.h>
#include <string.h>

#define HEAP_SIZE (1024 * 1024)

typedef struct block {
    uint64_t size;   /* bytes de payload (sin cabecera) */
    uint64_t free;   /* 1 = libre */
    struct block *next;
} block_t;

static uint8_t heap[HEAP_SIZE];
static block_t *heap_head;

void heap_init(void)
{
    heap_head = (block_t *)heap;
    heap_head->size = HEAP_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = 0;
}

void *kmalloc(uint64_t size)
{
    size = (size + 7) & ~7u;
    block_t *b = heap_head;
    while (b) {
        if (b->free && b->size >= size) {
            if (b->size >= size + sizeof(block_t) + 8) {
                block_t *split = (block_t *)((uint8_t *)b + sizeof(block_t) + size);
                split->size = b->size - size - sizeof(block_t);
                split->free = 1;
                split->next = b->next;
                b->next = split;
                b->size = size;
            }
            b->free = 0;
            return (void *)((uint8_t *)b + sizeof(block_t));
        }
        b = b->next;
    }
    return 0;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    block_t *b = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    b->free = 1;
    /* fusionar con siguiente si esta libre */
    if (b->next && b->next->free) {
        b->size += sizeof(block_t) + b->next->size;
        b->next = b->next->next;
    }
}
