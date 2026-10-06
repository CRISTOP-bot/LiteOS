#include <stdint.h>
#include <string.h>

void serial_init(void);
extern void qemu_exit_pub(unsigned char);
void serial_puts(const char *s);
void serial_hex(uint64_t v);


extern void idt_init(void);
extern void pmm_init(uint64_t mbi);
extern void heap_init(void);
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);
extern void pic_init(void);
extern void timer_init(void);

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

    pic_init();
    timer_init();
    __asm__ volatile ("sti");

    serial_puts("BOOT OK\n");
    for (;;)
        __asm__ volatile ("hlt");
}
