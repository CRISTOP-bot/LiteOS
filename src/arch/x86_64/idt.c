extern void qemu_exit_pub(unsigned char);
#include <stdint.h>

void serial_puts(const char *s);
void serial_hex(uint64_t v);


void exc_c(uint64_t vector)
{
    serial_puts("EXCEPTION vector=");
    serial_hex(vector);
    serial_puts(" -> halting\n");
    qemu_exit_pub(0x20);
    for (;;)
        __asm__ volatile ("hlt");
}

typedef struct {
    uint16_t off0; uint16_t sel; uint8_t ist; uint8_t attr;
    uint16_t off1; uint32_t off2; uint32_t zero;
} __attribute__((packed)) idt_gate;

__attribute__((aligned(16))) static idt_gate idt[256];

typedef struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idt_ptr;

void isr_common(void);

static void idt_set(int n, void (*fn)(void))
{
    uint64_t a = (uint64_t)fn;
    idt[n].off0 = a & 0xFFFF;
    idt[n].sel  = 0x08;
    idt[n].ist  = 0;
    idt[n].attr = 0x8E; /* interrupt gate, present, ring0 */
    idt[n].off1 = (a >> 16) & 0xFFFF;
    idt[n].off2 = a >> 32;
    idt[n].zero = 0;
}

void idt_init(void)
{
    extern void idt_stub_0(void);
    extern void idt_stub_1(void);
    extern void idt_stub_2(void);
    extern void idt_stub_3(void);
    extern void idt_stub_6(void);
    extern void idt_stub_8(void);
    extern void idt_stub_10(void);
    extern void idt_stub_11(void);
    extern void idt_stub_12(void);
    extern void idt_stub_13(void);
    extern void idt_stub_14(void);
    extern void idt_irq_0(void);
    extern void idt_irq_1(void);
    extern void idt_irq_2(void);
    extern void idt_irq_3(void);
    extern void idt_irq_4(void);
    extern void idt_irq_5(void);
    extern void idt_irq_6(void);
    extern void idt_irq_7(void);
    extern void idt_irq_8(void);
    extern void idt_irq_9(void);
    extern void idt_irq_10(void);
    extern void idt_irq_11(void);
    extern void idt_irq_12(void);
    extern void idt_irq_13(void);
    extern void idt_irq_14(void);
    extern void idt_irq_15(void);

    void (*idt_irq_tab[16])(void) = {
        idt_irq_0, idt_irq_1, idt_irq_2, idt_irq_3,
        idt_irq_4, idt_irq_5, idt_irq_6, idt_irq_7,
        idt_irq_8, idt_irq_9, idt_irq_10, idt_irq_11,
        idt_irq_12, idt_irq_13, idt_irq_14, idt_irq_15,
    };

    idt_set(0, idt_stub_0);
    idt_set(1, idt_stub_1);
    idt_set(2, idt_stub_2);
    idt_set(3, idt_stub_3);
    idt_set(6, idt_stub_6);
    idt_set(8, idt_stub_8);
    idt_set(10, idt_stub_10);
    idt_set(11, idt_stub_11);
    idt_set(12, idt_stub_12);
    idt_set(13, idt_stub_13);
    idt_set(14, idt_stub_14);

    for (int i = 0; i < 16; i++)
        idt_set(32 + i, idt_irq_tab[i]);

    idt_ptr p = { sizeof(idt) - 1, (uint64_t)&idt };
    __asm__ volatile ("lidt %0" :: "m"(p));
}
