#include <stdint.h>

void qemu_exit_pub(uint8_t code)
{
    __asm__ volatile ("outb %0, %1" :: "a"(code), "d"((uint16_t)0xf4));
}
