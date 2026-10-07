extern int need_resched;
/* LiteOS: PIT 8253 a 100 Hz + TSC.
 * El tick habilita la planificacion preventiva y mide el
 * tiempo de actividad. */
#include <stdint.h>
#include "kernel.h"

static volatile uint64_t ticks;
static uint64_t tsc_at_boot;
static uint64_t tsc_per_tick;   /* ticks de TSC por tick de PIT */

void timer_init(void)
{
    uint16_t divisor = (uint16_t)(1193182 / 100);   /* 100 Hz */
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
    ticks = 0;
    tsc_at_boot = rdtsc();
}

/* Calibra el TSC contra el PIT (espera 10 ticks = 100 ms). */
void tsc_calibrate(void)
{
    uint64_t t0 = ticks;
    uint64_t c0 = rdtsc();
    while (ticks - t0 < 10)
        __asm__ volatile ("pause");
    uint64_t c1 = rdtsc();
    tsc_per_tick = (c1 - c0) / 10;
    kvprintf("timer: PIT 100Hz, TSC ~%u Hz\n",
             (uint32_t)(tsc_per_tick * 100));
}

uint64_t tsc_hz(void)
{
    return tsc_per_tick * 100;
}

uint64_t uptime_ticks(void)
{
    return ticks;
}

void timer_tick(void)
{
    ticks++;
    need_resched = 1;
}
