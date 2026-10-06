#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" :: "a"(val), "d"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "d"(port));
    return v;
}

static inline void io_wait(void)
{
    outb(0x80, 0);
}

/* Remapea PIC 8259: IRQ0-7 -> int 32-39, IRQ8-15 -> int 40-47 */
void pic_init(void)
{
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    outb(0x21, 0xFE); /* solo IRQ0 (timer) habilitado */
    outb(0xA1, 0xFF); /* todo enmascarado en el esclavo */
    (void)a1; (void)a2;
}

void pic_eoi_master(void)
{
    outb(0x20, 0x20);
}

void pic_eoi_slave(void)
{
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}
