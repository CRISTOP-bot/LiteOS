/* LiteOS: reloj RTC CMOS (tiempo de pared) + epoca de arranque. */
#include <stdint.h>
#include "kernel.h"

static uint64_t boot_epoch;

static int rtc_update_pending(void)
{
    outb(0x70, 0x0A);
    return inb(0x71) & 0x80;
}

static uint8_t rtc_read(int reg)
{
    outb(0x70, (uint8_t)reg);
    return inb(0x71);
}

static int bcd2bin(uint8_t v)
{
    return (v & 0x0F) + ((v >> 4) * 10);
}

/* Dias desde 1970-01-01 (algoritmo de Howard Hinnant). */
static long days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

void rtc_init(void)
{
    while (rtc_update_pending())
        ;

    int bcd = !(rtc_read(0x0B) & 0x04);   /* 0 => formato BCD */

    uint8_t s  = rtc_read(0x00);
    uint8_t m  = rtc_read(0x02);
    uint8_t h  = rtc_read(0x04);
    uint8_t dn = rtc_read(0x07);
    uint8_t mo = rtc_read(0x08);
    uint8_t y  = rtc_read(0x09);

    int sec  = bcd ? bcd2bin(s)  : s;
    int min  = bcd ? bcd2bin(m)  : m;
    int hour = bcd ? bcd2bin(h)  : h;
    int day  = bcd ? bcd2bin(dn) : dn;
    int mon  = bcd ? bcd2bin(mo) : mo;
    int year = 2000 + (bcd ? bcd2bin(y) : y);

    if (mon < 1 || mon > 12 || day < 1 || day > 31) {
        boot_epoch = 0;
        return;
    }

    long days = days_from_civil(year, mon, day);
    boot_epoch = (uint64_t)days * 86400ULL
               + (uint64_t)hour * 3600 + (uint64_t)min * 60 + (uint64_t)sec;

    kvprintf("rtc: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
             year, mon, day, hour, min, sec);
}

uint64_t rtc_boot_epoch(void)
{
    return boot_epoch;
}
