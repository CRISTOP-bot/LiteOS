/* LiteOS: TTY canonico con edicion de linea.
 * La entrada del teclado se eco en VGA+serie y se almacena
 * en un búfer circular; read() devuelve lineas completas. */
#include <stdint.h>
#include <string.h>
#include "kernel.h"
extern struct proc *current;

#define TTY_BUF 4096

static struct {
    uint8_t in[TTY_BUF];
    usize head;                  /* proximo byte en salir */
    usize count;                 /* bytes almacenados */
} tty;

void tty_init(void)
{
    tty.head = 0;
    tty.count = 0;
}

static void tty_echo(char c)
{
    vga_putc(c);
    serial_putc(c);
}

/* Alimenta la linea del TTY desde el driver de teclado. */
void tty_input(char c)
{
    if (c == '\r')
        c = '\n';

    if (c == 0x7F) {             /* retroceso */
        if (tty.count > 0) {
            tty.count--;
            tty_echo('\b');
            tty_echo(' ');
            tty_echo('\b');
        }
        return;
    }

    if (tty.count < TTY_BUF)
        tty.in[(tty.head + tty.count) % TTY_BUF] = (uint8_t)c;
    else
        return;                  /* búfer lleno: descartar */

    tty.count++;
    tty_echo(c);

    if (c == '\n')
        proc_wakeup(&tty);      /* despertar a los lectores */
}

/* Devuelve una linea completa (con '\n') o hasta n bytes. */
ssize_t tty_read(u64 off, void *buf, usize n)
{
    (void)off;
    if (n == 0)
        return 0;

    for (;;) {
        usize i = 0;
        while (i < tty.count &&
               tty.in[(tty.head + i) % TTY_BUF] != '\n')
            i++;
        if (i < tty.count)        /* hay linea completa */
            break;
        if (tty.count == TTY_BUF) /* búfer lleno sin newline */
            break;
        current->state = PS_SLEEPING;
        current->wchan = &tty;
        schedule();
    }

    usize take = 0;
    while (take < tty.count && take < n) {
        ((uint8_t *)buf)[take] =
            tty.in[(tty.head + take) % TTY_BUF];
        if (((uint8_t *)buf)[take] == '\n') {
            take++;
            break;
        }
        take++;
    }
    tty.head = (tty.head + take) % TTY_BUF;
    tty.count -= take;
    return (ssize_t)take;
}

ssize_t tty_write(u64 off, const void *buf, usize n)
{
    (void)off;
    const uint8_t *p = buf;
    for (usize i = 0; i < n; i++) {
        vga_putc((char)p[i]);
        serial_putc((char)p[i]);
    }
    return (ssize_t)n;
}
