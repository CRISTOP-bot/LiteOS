#include <stdint.h>
#include <string.h>
#include "kernel.h"


void kernel_main(uint64_t mbi, uint64_t magic)
{
    serial_init();
    serial_puts("LiteOS kernel: serial ok\n");
    serial_puts("multiboot2 magic=");
    serial_hex(magic);
    serial_puts(" mbi=");
    serial_hex(mbi);
    serial_puts("\n");
    if (magic != 0x36d76289)
        serial_puts("WARN: magic incorrecto (no multiboot2?)\n");

    idt_init();
    serial_puts("idt cargada\n");

    pmm_init(mbi);

    heap_init();
    void *p = kmalloc(64);
    if (p) {
        memset(p, 0xA5, 64);
        serial_puts("heap ok: kmalloc(64) -> ");
        serial_hex((uint64_t)p);
        serial_puts("\n");
        kfree(p);
    } else {
        serial_puts("heap FAIL\n");
    }

    gdt_init();
    vga_init();
    vfs_init();
    initramfs_load(_binary_initramfs_cpio_start,
                   (u64)(_binary_initramfs_cpio_end - _binary_initramfs_cpio_start));
    u32 total = *(u32 *)mbi;
    for (u64 off = 8; off + 16 <= total;) {
        u32 tag = *(u32 *)(mbi + off);
        u32 size = *(u32 *)(mbi + off + 4);
        if (size < 8) break;
        if (tag == 3 && size >= 16) {
            u64 start = *(u32 *)(mbi + off + 8);
            u64 end = *(u32 *)(mbi + off + 12);
            if (end > start) initramfs_load((const u8 *)start, end - start);
        }
        if (tag == 0) break;
        off += (size + 7) & ~7u;
    }
    rtc_init();
    tty_init();
    dev_init();
    keyboard_init();
    proc_init();
    int pid = proc_start_init("/bin/sh");
    if (pid < 0) {
        serial_puts("FATAL: /bin/sh no disponible, error=");
        serial_hex((u64)(long)pid);
        serial_puts("\n");
        qemu_exit(0x24);
        for (;;) __asm__ volatile ("hlt");
    }
    pic_init();
    timer_init();
    serial_puts("BOOT OK\n");
    __asm__ volatile ("sti");
    schedule();
    for (;;)
        __asm__ volatile ("hlt");
}
