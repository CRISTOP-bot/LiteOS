/* LiteOS: IDT de 256 entradas.
 * 0..31   excepciones de CPU
 * 32..47  IRQs (PIC remapeado)
 * 128     int 0x80 (syscalls)
 */
#include <stdint.h>
#include "kernel.h"

typedef struct {
    uint16_t off0;
    uint16_t sel;
    uint8_t  ist;
    uint8_t  attr;
    uint16_t off1;
    uint32_t off2;
    uint32_t zero;
} __attribute__((packed)) idt_gate;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr;

__attribute__((aligned(16))) static idt_gate idt[256];

extern void *idt_stub_table[];
extern void idt_int80(void);

static void idt_set(int n, void (*fn)(void), int dpl)
{
    uint64_t a = (uint64_t)fn;
    idt[n].off0  = a & 0xFFFF;
    idt[n].sel   = GDT_KCODE;
    idt[n].ist   = 0;
    idt[n].attr  = 0x80 | 0x0E | (dpl << 5);  /* presente, interrupt gate */
    idt[n].off1  = (a >> 16) & 0xFFFF;
    idt[n].off2  = a >> 32;
    idt[n].zero  = 0;
}

void idt_init(void)
{
    for (int i = 0; i < 48; i++)
        idt_set(i, (void (*)(void))idt_stub_table[i], 0);

    /* Syscalls: int 0x80 invocable desde ring 3 */
    idt_set(0x80, idt_int80, 3);

    idt_ptr p = { sizeof(idt) - 1, (uint64_t)&idt };
    __asm__ volatile ("lidt %0" :: "m"(p));
}
