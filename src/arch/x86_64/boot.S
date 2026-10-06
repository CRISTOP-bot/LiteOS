/* LiteOS x86_64 bootstrap: GRUB (multiboot2) -> long mode -> kernel_main */

#define MULTIBOOT2_HEADER_MAGIC   0xe85250d6
#define MULTIBOOT2_ARCH_I386      0
#define MULTIBOOT2_MAGIC          0x36d76289
#define MULTIBOOT2_END_TAG        0

.section .multiboot2
.align 8
multiboot2_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT2_ARCH_I386
    .long header_end - multiboot2_header
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCH_I386 + (header_end - multiboot2_header))
    .short 0
    .short 0
    .long 8
header_end:

.extern kernel_main
.extern pml4_table
.extern pdpt_table
.extern pd_table

.section .text.boot
.code32
.global _start
_start:
    cli
    movl %ebx, multiboot_ptr
    movl %eax, multiboot_magic

    /* Validar magic */
    cmp $MULTIBOOT2_MAGIC, %eax
    jne .hang32

    /* Inicializar tablas de paginas (identidad, primeras 16 MiB) */
    mov $pml4_table, %eax
    xor %ecx, %ecx
    mov $4096*3, %ecx
    xor %edx, %edx
    mov $0x1000, %edi
.zero:
    movl $0, (%eax)
    add $4, %eax
    loop .zero

    /* pml4[0] -> pdpt */
    mov $pdpt_table, %eax
    or  $0x3, %eax
    mov %eax, pml4_table
    /* pdpt[0] -> pd */
    mov $pd_table, %eax
    or  $0x3, %eax
    mov %eax, pdpt_table
    /* pd[0] = 0-2MiB, pd[1] = 2-4MiB, ... hasta 16 MiB */
    mov $pd_table, %edi
    mov $0x83, %eax
    mov $8, %ecx
.fill_pd:
    mov %eax, (%edi)
    add $0x200000, %eax
    add $8, %edi
    loop .fill_pd

    /* Cargar CR3, activar PAE, EFER.LME, CR0.PG */
    mov $pml4_table, %eax
    mov %eax, %cr3
    mov %cr4, %eax
    or  $0x20, %eax
    mov %eax, %cr4
    mov $0xC0000080, %ecx
    rdmsr
    or  $0x100, %eax
    wrmsr
    mov %cr0, %eax
    or  $0x80000001, %eax
    mov %eax, %cr0

    /* GDT de 64 bits temporal y far jump */
    .byte 0x66
    lgdt (gdt64_ptr)
    ljmp $0x08, $long_mode

.hang32:
    hlt
    jmp .hang32

.code64
long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $_stack_top, %rsp
    xor %rbp, %rbp
    mov multiboot_ptr, %rdi
    mov multiboot_magic, %esi
    call kernel_main

.hang64:
    cli
    hlt
    jmp .hang64

.section .rodata
.align 8
gdt64:
    .quad 0
    .quad 0x00af9a000000ffff   /* code 64 */
    .quad 0x00af92000000ffff   /* data 64 */
gdt64_ptr:
    .word . - gdt64 - 1
    .quad gdt64

.section .data
.global multiboot_ptr, multiboot_magic
multiboot_ptr:    .quad 0
multiboot_magic:  .quad 0

.section .bss
.align 4096
.global pml4_table, pdpt_table, pd_table
pml4_table: .skip 4096
pdpt_table: .skip 4096
pd_table:   .skip 4096
