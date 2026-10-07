/* LiteOS: driver de teclado PS/2 (set 1, layout US).
 * Convierte scan codes a ASCII y alimenta la linea del TTY. */
#include <stdint.h>
#include "kernel.h"

static const char keymap[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = 0x7F,                          /* backspace */
    [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\r',                          /* enter */
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2A] = 0,                             /* shift izquierdo */
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.', [0x35] = '/',
    [0x36] = 0,                             /* shift derecho */
    [0x39] = ' ',
};

static int shift, caps;

void keyboard_init(void)
{
    shift = 0;
    caps = 0;
}

void keyboard_irq(void)
{
    uint8_t sc = inb(0x60);

    if (sc & 0x80) {                        /* liberacion de tecla */
        uint8_t code = sc & 0x7F;
        if (code == 0x2A || code == 0x36)
            shift = 0;
        return;
    }

    if (sc == 0x2A || sc == 0x36) {
        shift = 1;
        return;
    }
    if (sc == 0x3A) {                       /* bloq mayus */
        caps ^= 1;
        return;
    }
    if (sc == 0xE0 || sc == 0xE1)           /* prefijo extendido: ignorar */
        return;
    if (sc >= 128)
        return;

    char c = keymap[sc];
    if (!c)
        return;

    if (c >= 'a' && c <= 'z') {
        if (shift ^ caps)
            c = (char)(c - 'a' + 'A');
    } else if (shift) {
        switch (c) {
        case '1': c = '!'; break;
        case '2': c = '@'; break;
        case '3': c = '#'; break;
        case '4': c = '$'; break;
        case '5': c = '%'; break;
        case '6': c = '^'; break;
        case '7': c = '&'; break;
        case '8': c = '*'; break;
        case '9': c = '('; break;
        case '0': c = ')'; break;
        case '-': c = '_'; break;
        case '=': c = '+'; break;
        case '[': c = '{'; break;
        case ']': c = '}'; break;
        case ';': c = ':'; break;
        case '\'': c = '"'; break;
        case '`': c = '~'; break;
        case '\\': c = '|'; break;
        case ',': c = '<'; break;
        case '.': c = '>'; break;
        case '/': c = '?'; break;
        default: break;
        }
    }

    tty_input(c);
}
