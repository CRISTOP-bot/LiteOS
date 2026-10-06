#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" :: "a"(val), "d"((uint16_t)port));
}

void serial_init(void)
{
    outb(0x3F9, 0x00); /* deshabilitar IRQs */
    outb(0x3FB, 0x80); /* divisor */
    outb(0x3F8, 0x03); /* 38400 baud? -> 115200/3 */
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03); /* 8N1 */
    outb(0x3FA, 0xC7); /* FIFO on */
    outb(0x3FC, 0x0B);
}

static int serial_tx_ready(void)
{
    uint8_t s;
    uint16_t port = 0x3FD;
    __asm__ volatile ("inb %1, %0" : "=a"(s) : "d"(port));
    return s & 0x20;
}

void serial_putc(char c)
{
    while (!serial_tx_ready())
        ;
    if (c == '\n') {
        while (!serial_tx_ready())
            ;
        outb(0x3F8, '\r');
    }
    while (!serial_tx_ready())
        ;
    outb(0x3F8, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s)
        serial_putc(*s++);
}

void serial_hex(uint64_t v)
{
    static const char digits[] = "0123456789abcdef";
    char buf[17];
    for (int i = 15; i >= 0; i--)
        buf[i] = digits[(v >> ((15 - i) * 4)) & 0xF];
    buf[16] = '\0';
    serial_puts("0x");
    serial_puts(buf);
}
