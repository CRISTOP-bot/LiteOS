/* LiteOS: paginacion de 4 niveles.
 *
 * El kernel vive en identidad baja (PML4[0], paginas de 2 MiB
 * solo supervisor, creadas por boot.S). El espacio de usuario
 * ocupa PML4[2..255] con paginas de 4 KiB y flags de usuario;
 * cada proceso tiene su propio PML4 y copia la entrada 0 del
 * kernel para compartir el mapa de identidad (sin exponerlo a
 * ring 3: las entradas supervisor no son accesibles desde usuario).
 */
#include <stdint.h>
#include <string.h>
#include "kernel.h"

extern uint64_t pml4_table;
uint64_t kernel_pml4 = (uint64_t)&pml4_table;

static int is_user_vaddr(uint64_t vaddr)
{
    return vaddr >= USER_TEXT_BASE && vaddr < USER_STACK_TOP + 4096;
}

/* Caminata de 4 niveles asignando tablas intermedias segun
 * haga falta. Solo se usa para direcciones de usuario. */
int map_page(uint64_t pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    if (!is_user_vaddr(vaddr))
        return -1;

    uint64_t *pml4e = &((uint64_t *)pml4)[(vaddr >> 39) & 511];
    if (!(*pml4e & PAGE_P)) {
        uint64_t pdpt = pmm_alloc();
        if (!pdpt)
            return -1;
        memset((void *)pdpt, 0, 4096);
        *pml4e = pdpt | PAGE_USER;
    }
    uint64_t *pdpt = (uint64_t *)(*pml4e & ~0xFFFULL);
    uint64_t *pdpe = (uint64_t *)(pdpt + ((vaddr >> 30) & 511));
    if (!(*pdpe & PAGE_P)) {
        uint64_t pd = pmm_alloc();
        if (!pd)
            return -1;
        memset((void *)pd, 0, 4096);
        *pdpe = pd | PAGE_USER;
    }
    uint64_t *pd = (uint64_t *)(*pdpe & ~0xFFFULL);
    uint64_t *pde = (uint64_t *)(pd + ((vaddr >> 21) & 511));
    if (*pde & PAGE_PS)                 /* colision con pagina gigante */
        return -1;
    if (!(*pde & PAGE_P)) {
        uint64_t pt = pmm_alloc();
        if (!pt)
            return -1;
        memset((void *)pt, 0, 4096);
        *pde = pt | PAGE_USER;
    }
    uint64_t *pt = (uint64_t *)(*pde & ~0xFFFULL);
    uint64_t *pte = (uint64_t *)(pt + ((vaddr >> 12) & 511));
    *pte = (paddr & ~0xFFFULL) | (flags & 0xFFF);
    __asm__ volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
    return 0;
}

uint64_t vm_query(uint64_t pml4, uint64_t vaddr, uint64_t *flags)
{
    if (!is_user_vaddr(vaddr)) return 0;
    uint64_t e = ((uint64_t *)pml4)[(vaddr >> 39) & 511];
    if (!(e & PAGE_P) || !(e & PAGE_U)) return 0;
    e = ((uint64_t *)(e & ~0xFFFULL))[(vaddr >> 30) & 511];
    if (!(e & PAGE_P) || !(e & PAGE_U) || (e & PAGE_PS)) return 0;
    e = ((uint64_t *)(e & ~0xFFFULL))[(vaddr >> 21) & 511];
    if (!(e & PAGE_P) || !(e & PAGE_U) || (e & PAGE_PS)) return 0;
    e = ((uint64_t *)(e & ~0xFFFULL))[(vaddr >> 12) & 511];
    if (!(e & PAGE_P) || !(e & PAGE_U)) return 0;
    if (flags) *flags = e & 0xFFF;
    return (e & ~0xFFFULL) | (vaddr & 0xFFF);
}

int unmap_page(uint64_t pml4, uint64_t vaddr)
{
    uint64_t pml4e = ((uint64_t *)pml4)[(vaddr >> 39) & 511];
    if (!(pml4e & PAGE_P))
        return -1;
    uint64_t *pdpt = (uint64_t *)(pml4e & ~0xFFFULL);
    uint64_t pdpe = pdpt[(vaddr >> 30) & 511];
    if (!(pdpe & PAGE_P))
        return -1;
    uint64_t *pd = (uint64_t *)(pdpe & ~0xFFFULL);
    uint64_t pde = pd[(vaddr >> 21) & 511];
    if (!(pde & PAGE_P) || (pde & PAGE_PS))
        return -1;
    uint64_t *pt = (uint64_t *)(pde & ~0xFFFULL);
    uint64_t pte = pt[(vaddr >> 12) & 511];
    pt[(vaddr >> 12) & 511] = 0;
    __asm__ volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
    return (pte & PAGE_P) ? 0 : -1;
}

struct mm *mm_create(void)
{
    struct mm *mm = kmalloc(sizeof(*mm));
    if (!mm)
        return NULL;
    uint64_t pml4 = pmm_alloc();
    if (!pml4) {
        kfree(mm);
        return NULL;
    }
    memset((void *)pml4, 0, 4096);
    ((uint64_t *)pml4)[0] = ((uint64_t *)kernel_pml4)[0];
    mm->pml4 = pml4;
    mm->brk = 0;
    mm->mmap_top = 0;
    return mm;
}

/* Libera paginas de usuario y tablas intermedias de un PML4. */
static void mm_free_tables(uint64_t pml4_addr)
{
    uint64_t *pml4 = (uint64_t *)pml4_addr;
    for (int pi = 2; pi < 256; pi++) {          /* solo usuario */
        uint64_t pdpt_addr = pml4[pi];
        if (!(pdpt_addr & PAGE_P))
            continue;
        uint64_t *pdpt = (uint64_t *)(pdpt_addr & ~0xFFFULL);
        for (int pdi = 0; pdi < 512; pdi++) {
            uint64_t pd_addr = pdpt[pdi];
            if (!(pd_addr & PAGE_P))
                continue;
            uint64_t *pd = (uint64_t *)(pd_addr & ~0xFFFULL);
            for (int pti = 0; pti < 512; pti++) {
                uint64_t pt_addr = pd[pti];
                if (!(pt_addr & PAGE_P))
                    continue;
                uint64_t *pt = (uint64_t *)(pt_addr & ~0xFFFULL);
                for (int i = 0; i < 512; i++) {
                    if (pt[i] & PAGE_P)
                        pmm_free(pt[i] & ~0xFFFULL);
                }
                pmm_free(pt_addr & ~0xFFFULL);
            }
            pmm_free(pd_addr & ~0xFFFULL);
        }
        pmm_free(pdpt_addr & ~0xFFFULL);
    }
}

void mm_destroy(struct mm *mm)
{
    if (!mm)
        return;
    mm_free_tables(mm->pml4);
    pmm_free(mm->pml4);
    kfree(mm);
}

void mm_free_pml4(uint64_t pml4)
{
    mm_free_tables(pml4);
    pmm_free(pml4);
}

/* Copia profunda del espacio de usuario (fork): asigna nuevos
 * marcos para cada pagina de usuario y copia el contenido. */
uint64_t mm_clone(uint64_t src_pml4)
{
    uint64_t dst = pmm_alloc();
    if (!dst)
        return 0;
    memset((void *)dst, 0, 4096);
    ((uint64_t *)dst)[0] = ((uint64_t *)src_pml4)[0];

    uint64_t *spml4 = (uint64_t *)src_pml4;
    uint64_t *dpml4 = (uint64_t *)dst;

    for (int pi = 2; pi < 256; pi++) {
        uint64_t spdpt_addr = spml4[pi];
        if (!(spdpt_addr & PAGE_P))
            continue;
        uint64_t *spdpt = (uint64_t *)(spdpt_addr & ~0xFFFULL);
        uint64_t dpdpt_addr = pmm_alloc();
        if (!dpdpt_addr)
            goto fail;
        memset((void *)dpdpt_addr, 0, 4096);
        dpml4[pi] = dpdpt_addr | PAGE_USER;
        uint64_t *dpdpt = (uint64_t *)dpdpt_addr;

        for (int pdi = 0; pdi < 512; pdi++) {
            uint64_t spd_addr = spdpt[pdi];
            if (!(spd_addr & PAGE_P))
                continue;
            uint64_t *spd = (uint64_t *)(spd_addr & ~0xFFFULL);
            uint64_t dpd_addr = pmm_alloc();
            if (!dpd_addr)
                goto fail;
            memset((void *)dpd_addr, 0, 4096);
            dpdpt[pdi] = dpd_addr | PAGE_USER;
            uint64_t *dpd = (uint64_t *)dpd_addr;

            for (int pti = 0; pti < 512; pti++) {
                uint64_t spt_addr = spd[pti];
                if (!(spt_addr & PAGE_P))
                    continue;
                uint64_t *spt = (uint64_t *)(spt_addr & ~0xFFFULL);
                uint64_t dpt_addr = pmm_alloc();
                if (!dpt_addr)
                    goto fail;
                memset((void *)dpt_addr, 0, 4096);
                dpd[pti] = dpt_addr | PAGE_USER;
                uint64_t *dpt = (uint64_t *)dpt_addr;

                for (int i = 0; i < 512; i++) {
                    if (!(spt[i] & PAGE_P))
                        continue;
                    uint64_t frame = pmm_alloc();
                    if (!frame)
                        goto fail;
                    memcpy((void *)frame, (void *)(spt[i] & ~0xFFFULL), 4096);
                    dpt[i] = frame | (spt[i] & 0xFFF);
                }
            }
        }
    }
    return dst;

fail:
    mm_free_tables(dst);
    pmm_free(dst);
    return 0;
}
