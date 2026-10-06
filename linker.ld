OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

KERNEL_BASE = 0x100000;

SECTIONS
{
    . = KERNEL_BASE;

    .multiboot2 ALIGN(8) : {
        KEEP(*(.multiboot2))
    }

    .text ALIGN(4K) : {
        *(.text.boot)
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data .data.*)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss .bss.*)
        _stack_bottom = .;
        . += 0x40000;
        _stack_top = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}
