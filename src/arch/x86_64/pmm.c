#include <stdint.h>

void serial_puts(const char *s);
void serial_hex(uint64_t v);

extern char _stack_top;

#define PMM_REGION_END (16u * 1024 * 1024)
#define PAGE_SIZE      4096u
#define PMM_PAGES      (PMM_REGION_END / PAGE_SIZE)

static uint8_t pmm_bitmap[PMM_PAGES / 8];
static uint64_t pmm_free_count;

static void pmm_set(uint64_t page)
{
    pmm_bitmap[page / 8] |= (uint8_t)(1u << (page % 8));
}

static void pmm_clear(uint64_t page)
{
    pmm_bitmap[page / 8] &= (uint8_t)~(1u << (page % 8));
}

static int pmm_used(uint64_t page)
{
    return pmm_bitmap[page / 8] & (1u << (page % 8));
}

void pmm_init(uint64_t mbi)
{
    for (uint64_t i = 0; i < PMM_PAGES / 8; i++)
        pmm_bitmap[i] = 0xFF; /* todo usado hasta demostrar lo contrario */

    uint64_t end = (uint64_t)&_stack_top;
    uint64_t stack_page = (end + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Recorrer tags multiboot2 buscando el memory map (type 6) */
    uint32_t total = *(uint32_t *)mbi;
    uint64_t off = 8;
    while (off + 8 <= total) {
        uint32_t type = *(uint32_t *)(mbi + off);
        uint32_t size = *(uint32_t *)(mbi + off + 4);
        if (size < 8)
            break;
        if (type == 6) {
            uint32_t entry_size = *(uint32_t *)(mbi + off + 8);
            uint32_t entry_ver = *(uint32_t *)(mbi + off + 12);
            (void)entry_ver;
            uint64_t p = mbi + off + 16;
            uint64_t pend = mbi + off + size;
            while (p + entry_size <= pend) {
                uint64_t base = *(uint64_t *)p;
                uint64_t len = *(uint64_t *)(p + 8);
                uint32_t etype = *(uint32_t *)(p + 16);
                if (etype == 1 && len > 0) {
                    uint64_t lo = base < (1u << 21) ? (1u << 21) : base;
                    uint64_t hi = base + len;
                    if (hi > PMM_REGION_END)
                        hi = PMM_REGION_END;
                    lo = (lo + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
                    hi = hi & ~(uint64_t)(PAGE_SIZE - 1);
                    for (uint64_t a = lo; a + PAGE_SIZE <= hi; a += PAGE_SIZE) {
                        uint64_t pg = a / PAGE_SIZE;
                        if (pg >= stack_page && pg < PMM_PAGES)
                            pmm_clear(pg);
                    }
                }
                p += entry_size;
            }
        }
        off += (size + 7) & ~7u;
        if (type == 0)
            break;
    }

    /* Seguridad: nunca liberar paginas del kernel/pila */
    for (uint64_t pg = 0; pg < stack_page; pg++)
        pmm_set(pg);

    pmm_free_count = 0;
    for (uint64_t pg = 0; pg < PMM_PAGES; pg++)
        if (!pmm_used(pg))
            pmm_free_count++;

    serial_puts("pmm: free pages=");
    serial_hex(pmm_free_count);
    serial_puts(" (of ");
    serial_hex(PMM_PAGES);
    serial_puts(")\n");
}

void *pmm_alloc(void)
{
    for (uint64_t pg = 0; pg < PMM_PAGES; pg++) {
        if (!pmm_used(pg)) {
            pmm_set(pg);
            pmm_free_count--;
            return (void *)(pg * PAGE_SIZE);
        }
    }
    return 0;
}

void pmm_free(void *addr)
{
    uint64_t pg = (uint64_t)addr / PAGE_SIZE;
    if (pg < PMM_PAGES && pmm_used(pg)) {
        pmm_clear(pg);
        pmm_free_count++;
    }
}
