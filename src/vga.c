/* LiteOS: consola VGA en modo texto (80x25). */
#include <stdint.h>
#include "kernel.h"

#define VGA_MEM   ((volatile uint16_t *)0xB8000)
#define VGA_W     80
#define VGA_H     25
#define VGA_COLOR 0x07                  /* gris claro sobre negro */

static int vga_row, vga_col;

static void vga_cursor(void)
{
    uint16_t pos = (uint16_t)(vga_row * VGA_W + vga_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void)
{
    if (vga_row < VGA_H)
        return;
    for (int i = 0; i < (VGA_H - 1) * VGA_W; i++)
        VGA_MEM[i] = VGA_MEM[i + VGA_W];
    for (int i = (VGA_H - 1) * VGA_W; i < VGA_H * VGA_W; i++)
        VGA_MEM[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    vga_row = VGA_H - 1;
}

void vga_putc(char c)
{
    switch (c) {
    case '\n':
        vga_row++;
        vga_col = 0;
        break;
    case '\r':
        vga_col = 0;
        break;
    case '\b':
        if (vga_col > 0) {
            vga_col--;
            VGA_MEM[vga_row * VGA_W + vga_col] =
                (uint16_t)(' ' | (VGA_COLOR << 8));
        }
        break;
    case '\t':
        vga_col = (vga_col + 8) & ~7;
        break;
    default:
        VGA_MEM[vga_row * VGA_W + vga_col] =
            (uint16_t)((uint8_t)c | (VGA_COLOR << 8));
        vga_col++;
        break;
    }
    if (vga_col >= VGA_W) {
        vga_col = 0;
        vga_row++;
    }
    vga_scroll();
    vga_cursor();
}

void vga_puts(const char *s)
{
    while (*s)
        vga_putc(*s++);
}

void vga_init(void)
{
    vga_row = 0;
    vga_col = 0;
    for (int i = 0; i < VGA_H * VGA_W; i++)
        VGA_MEM[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    vga_cursor();
}
