#include <stdint.h>

void serial_init(void);
void serial_puts(const char *s);
void serial_hex(uint64_t v);

static inline void qemu_exit(uint8_t code)
{
    __asm__ volatile ("outb %0, %1" :: "a"(code), "d"((uint16_t)0xf4));
}

extern void idt_init(void);

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

    serial_puts("BOOT OK\n");
    qemu_exit(0x10); /* exit qemu: (0x10<<1)|1 = 33 */
    for (;;)
        __asm__ volatile ("hlt");
}
