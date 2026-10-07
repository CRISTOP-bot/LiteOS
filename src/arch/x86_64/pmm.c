/* LiteOS: gestor de memoria fisica (bitmap) sobre el memory map
 * de multiboot2. Cubre hasta 8 GiB (ver boot.S). */
#include <stdint.h>
#include <string.h>
#include "kernel.h"

#define PAGE_SZ      4096ULL
#define PMM_MAX_MEM  (8ULL * 1024 * 1024 * 1024)
#define PMM_PAGES    (PMM_MAX_MEM / PAGE_SZ)

static uint8_t pmm_bitmap[PMM_PAGES / 8];
static uint64_t pmm_free_pages;

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
    return (pmm_bitmap[page / 8] >> (page % 8)) & 1u;
}

void pmm_init(uint64_t mbi)
{
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

    uint64_t kernel_end_page =
        ((uint64_t)&_stack_top + PAGE_SZ - 1) / PAGE_SZ;

    /* MBI y módulos son memoria ocupada hasta copiar el initramfs. */
    uint32_t total = *(uint32_t *)mbi;
    uint64_t reserved_end = mbi + total;
    for (uint64_t tag = 8; tag + 8 <= total;) {
        uint32_t type = *(uint32_t *)(mbi + tag);
        uint32_t sz = *(uint32_t *)(mbi + tag + 4);
        if (sz < 8) break;
        if (type == 3 && sz >= 16) {
            uint64_t end = *(uint32_t *)(mbi + tag + 12);
            if (end > reserved_end) reserved_end = end;
        }
        if (type == 0) break;
        tag += (sz + 7) & ~7u;
    }
    if ((reserved_end + PAGE_SZ - 1) / PAGE_SZ > kernel_end_page)
        kernel_end_page = (reserved_end + PAGE_SZ - 1) / PAGE_SZ;

    /* Recorrer los tags multiboot2 buscando el memory map (tipo 6) */
    uint64_t off = 8;
    uint64_t usable_bytes = 0;

    while (off + 8 <= total) {
        uint32_t type  = *(uint32_t *)(mbi + off);
        uint32_t size  = *(uint32_t *)(mbi + off + 4);
        if (size < 8)
            break;
        if (type == 6) {
            uint32_t entry_size = *(uint32_t *)(mbi + off + 8);
            uint64_t p = mbi + off + 16;
            uint64_t pend = mbi + off + size;
            while (p + entry_size <= pend) {
                uint64_t base = *(uint64_t *)p;
                uint64_t len  = *(uint64_t *)(p + 8);
                uint32_t etype = *(uint32_t *)(p + 16);
                if (etype == 1 && len > 0) {   /* disponible */
                    uint64_t lo = base < kernel_end_page * PAGE_SZ
                                  ? kernel_end_page * PAGE_SZ : base;
                    uint64_t hi = base + len;
                    if (hi > PMM_MAX_MEM)
                        hi = PMM_MAX_MEM;
                    lo = (lo + PAGE_SZ - 1) & ~(PAGE_SZ - 1);
                    hi &= ~(PAGE_SZ - 1);
                    for (uint64_t a = lo; a + PAGE_SZ <= hi; a += PAGE_SZ) {
                        uint64_t pg = a / PAGE_SZ;
                        if (pmm_used(pg)) {
                            pmm_clear(pg);
                            pmm_free_pages++;
                            usable_bytes += PAGE_SZ;
                        }
                    }
                }
                p += entry_size;
            }
        }
        off += (size + 7) & ~7u;
        if (type == 0)
            break;
    }

    if (pmm_free_pages == 0) {
        serial_puts("FATAL: pmm sin memoria usable (memory map multiboot2)\n");
        qemu_exit(0x21);
        for (;;) __asm__ volatile ("hlt");
    }

    serial_puts("pmm: ");
    serial_hex(usable_bytes);
    serial_puts(" bytes utilizables, ");
    serial_hex(pmm_free_pages);
    serial_puts(" paginas\n");
}

uint64_t pmm_alloc_pages(usize n)
{
    if (!n || n > pmm_free_pages) return 0;
    for (uint64_t pg = 1; pg <= PMM_PAGES - n;) {
        usize found = 0;
        while (found < n && !pmm_used(pg + found)) found++;
        if (found == n) {
            for (usize i = 0; i < n; i++) pmm_set(pg + i);
            pmm_free_pages -= n;
            return pg * PAGE_SZ;
        }
        pg += found + 1;
    }
    return 0;
}

uint64_t pmm_alloc(void)
{
    for (uint64_t pg = 0; pg < PMM_PAGES; pg++) {
        if (!pmm_used(pg)) {
            pmm_set(pg);
            pmm_free_pages--;
            return pg * PAGE_SZ;
        }
    }
    return 0;
}

void pmm_free(uint64_t page)
{
    if ((page & (PAGE_SZ - 1)) != 0 || page >= PMM_MAX_MEM)
        return;
    uint64_t pg = page / PAGE_SZ;
    if (pmm_used(pg)) {
        pmm_clear(pg);
        pmm_free_pages++;
    }
}

uint64_t pmm_free_count(void)
{
    return pmm_free_pages;
}
