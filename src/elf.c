#include <errno.h>
#include <string.h>
#include "kernel.h"

/* Load only bounded, static x86-64 ET_EXEC images; no dynamic linker. */
int elf_load(const u8 *img, u64 size, struct mm *mm, u64 *entry)
{
    if (!img || !mm || !entry || size < sizeof(Elf64_Ehdr)) return -ENOEXEC;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)img;
    if (*(const u32 *)eh->e_ident != ELF_MAGIC ||
        eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_ident[EI_DATA] != ELSDATA2LSB ||
        eh->e_type != ET_EXEC || eh->e_machine != 62 ||
        eh->e_phentsize != sizeof(Elf64_Phdr) || eh->e_phnum > 64 ||
        eh->e_phoff > size ||
        eh->e_phnum > (size - eh->e_phoff) / sizeof(Elf64_Phdr))
        return -ENOEXEC;
    if (eh->e_entry < USER_TEXT_BASE || eh->e_entry >= USER_BRK_BASE)
        return -ENOEXEC;
    const Elf64_Phdr *ph = (const Elf64_Phdr *)(img + eh->e_phoff);
    int loads = 0;
    for (u16 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        u64 v = ph[i].p_vaddr, len = ph[i].p_memsz;
        if (ph[i].p_filesz > len || ph[i].p_offset > size ||
            ph[i].p_filesz > size - ph[i].p_offset ||
            v < USER_TEXT_BASE || v >= USER_BRK_BASE ||
            len > USER_BRK_BASE - v) return -ENOEXEC;
        if (!len) continue;
        loads++;
        for (u64 a = v & ~4095ULL; a < v + len; a += 4096) {
            if (vm_query(mm->pml4, a, NULL)) continue;
            u64 page = pmm_alloc();
            if (!page) return -ENOMEM;
            memset((void *)page, 0, 4096);
            if (map_page(mm->pml4, a, page, PAGE_USER) < 0) {
                pmm_free(page);
                return -ENOMEM;
            }
        }
        if (mm_copy_to(mm, v, img + ph[i].p_offset,
                       (usize)ph[i].p_filesz) < 0) return -ENOEXEC;
    }
    if (!loads || !vm_query(mm->pml4, eh->e_entry, NULL)) return -ENOEXEC;
    *entry = eh->e_entry;
    return 0;
}
