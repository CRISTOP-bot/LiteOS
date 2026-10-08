/* LiteOS: puerto serie 16550A (debug) con kvprintf. */
#include <stdint.h>
#include <stdarg.h>
#include "kernel.h"

void serial_init(void)
{
    outb(0x3F9, 0x00);   /* IRQs deshabilitadas */
    outb(0x3FB, 0x80);   /* divisor latch */
    outb(0x3F8, 0x03);   /* 38400 baud */
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);   /* 8N1 */
    outb(0x3FA, 0xC7);   /* FIFO on */
    outb(0x3FC, 0x0B);
}

static int serial_tx_ready(void)
{
    return inb(0x3FD) & 0x20;
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

static void serial_dec(u64 v)
{
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    do {
        buf[--i] = (char)('0' + (v % 10));
        v /= 10;
    } while (v);
    serial_puts(&buf[i]);
}

void kvprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            serial_putc(*p);
            continue;
        }
        p++;
        switch (*p) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            serial_puts(s ? s : "(null)");
            break;
        }
        case 'd':
            serial_dec((u64)va_arg(ap, int));
            break;
        case 'u':
            serial_dec(va_arg(ap, u64));
            break;
        case 'x':
        case 'p':
            serial_hex(va_arg(ap, u64));
            break;
        case 'c':
            serial_putc((char)va_arg(ap, int));
            break;
        case '%':
            serial_putc('%');
            break;
        default:
            serial_putc('%');
            serial_putc(*p);
            break;
        }
    }
    va_end(ap);
}
