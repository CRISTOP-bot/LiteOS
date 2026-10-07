/* LiteOS: remapeo del PIC 8259.
 * IRQ0-7 -> int 32-39, IRQ8-15 -> int 40-47. */
#include <stdint.h>
#include "kernel.h"

static inline void io_wait(void)
{
    outb(0x80, 0);
}

void pic_init(void)
{
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();   /* maestro: base 32 */
    outb(0xA1, 0x28); io_wait();   /* esclavo: base 40 */
    outb(0x21, 0x04); io_wait();   /* esclavo en IRQ2 */
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    outb(0x21, a1 & ~0x03);   /* IRQ0 (timer) + IRQ1 (teclado) */
    outb(0xA1, a2);           /* esclavo enmascarado */
}

void pic_eoi(int irq)
{
    if (irq >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
