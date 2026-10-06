#include <stdint.h>

void serial_puts(const char *s);
void serial_hex(uint64_t v);
void qemu_exit_pub(uint8_t code);
void pic_eoi_master(void);

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" :: "a"(val), "d"(port));
}

static uint64_t ticks;

void timer_init(void)
{
    uint16_t divisor = (uint16_t)(1193182 / 100); /* 100 Hz */
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
    ticks = 0;
}

void irq_c(uint64_t vector)
{
    if (vector == 32) {
        ticks++;
        if (ticks <= 3 || ticks == 20) {
            serial_puts("tick ");
            serial_hex(ticks);
            serial_puts("\n");
        }
        if (ticks >= 20) {
            serial_puts("timer ok (20 ticks @100Hz)\n");
            qemu_exit_pub(0x30);
        }
    }
    pic_eoi_master();
}
