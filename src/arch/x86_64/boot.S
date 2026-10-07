/* LiteOS x86_64 bootstrap: GRUB (multiboot2) -> long mode -> kernel_main.
 *
 * GRUB carga este ELF64 y salta aqui en modo protegido de 32 bits.
 * Construimos un mapa de identidad de 8 GiB con paginas de 2 MiB
 * (solo supervisor) y entramos en modo long. El kernel se enlaza
 * en 1 MiB, por lo que las direcciones virtuales == fisicas.
 */

#define MULTIBOOT2_HEADER_MAGIC   0xe85250d6
#define MULTIBOOT2_ARCH_I386      0
#define MULTIBOOT2_MAGIC          0x36d76289

.section .multiboot2,"a",@progbits
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

.section .text.boot
.code32
.global _start
_start:
    cli
    movl %ebx, multiboot_ptr
    movl %eax, multiboot_magic

    cmp $MULTIBOOT2_MAGIC, %eax
    jne .hang32

    /* Limpiar pml4_table + pdpt_table + pd_tables (10 paginas) */
    lea pml4_table, %edi
    mov $(10 * 1024), %ecx          /* 10 paginas = 10240 dwords */
    xor %eax, %eax
1:  movl %eax, (%edi)
    add $4, %edi
    loop 1b

    /* pml4[0] -> pdpt */
    mov $pdpt_table, %eax
    or  $0x3, %eax
    mov %eax, pml4_table

    /* pdpt[0..7] -> pd_tables[0..7]: 8 GiB de identidad */
    mov $pd_tables, %esi
    xor %ecx, %ecx
2:  mov %esi, %edx
    or  $0x3, %edx
    mov %edx, pdpt_table(,%ecx,8)
    add $4096, %esi
    inc %ecx
    cmp $8, %ecx
    jb 2b

    /* 8 * 512 entradas de 2 MiB con flags P|RW|PS */
    lea pd_tables, %edi
    mov $0x87, %eax
    mov $(8 * 512), %ecx
3:  mov %eax, (%edi)
    add $0x200000, %eax
    add $8, %edi
    loop 3b

    /* CR3, PAE, EFER.LME, CR0.PG */
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

    /* GDT de transicion y salto lejano a 64 bits */
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
    mov multiboot_magic, %rsi
    call kernel_main
.hang64:
    cli
    hlt
    jmp .hang64

.section .rodata
.align 8
gdt64:
    .quad 0
    .quad 0x00af9a000000ffff      /* codigo 64 bits */
    .quad 0x00af92000000ffff      /* datos */
gdt64_ptr:
    .word . - gdt64 - 1
    .quad gdt64

.section .data
.global multiboot_ptr, multiboot_magic
multiboot_ptr:    .quad 0
multiboot_magic:  .quad 0

.section .bss
.align 4096
.global pml4_table, pdpt_table, pd_tables
pml4_table: .skip 4096
pdpt_table: .skip 4096
pd_tables:  .skip 8 * 4096
