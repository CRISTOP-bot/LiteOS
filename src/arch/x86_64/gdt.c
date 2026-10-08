#include <stdint.h>

struct gdt_ptr { uint16_t limit; uint64_t base; } __attribute__((packed));

struct tss64 {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;
} __attribute__((packed));

static uint64_t gdt[7];
static struct { uint16_t limit; uint64_t base; } __attribute__((packed)) gdtp;
static struct tss64 tss;

void tss_set_rsp0(uint64_t rsp0)
{
    tss.rsp0 = rsp0;
}

void gdt_init(void)
{
    uint64_t kcode = 0x00af9a000000ffffULL;
    uint64_t kdata = 0x00af92000000ffffULL;
    uint64_t ucode = 0x00affa000000ffffULL; /* DPL=3 */
    uint64_t udata = 0x00aff2000000ffffULL; /* DPL=3 */

    gdt[0] = 0;
    gdt[1] = kcode;
    gdt[2] = kdata;
    gdt[3] = ucode;
    gdt[4] = udata;

    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    uint64_t lo = (uint64_t)(limit & 0xFFFF)
                | ((uint64_t)(base & 0xFFFFFF) << 16)
                | ((uint64_t)0x89 << 40)          /* P=1, DPL=0, type=0x9 */
                | ((uint64_t)(limit & 0xF0000) << 32)
                | ((base >> 24 & 0xFF) << 56);
    uint64_t hi = base >> 32;
    gdt[5] = lo;
    gdt[6] = hi;

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base = (uint64_t)gdt;

    __asm__ volatile ("lgdt (%0)" :: "r"(&gdtp));
    /* recargar CS via far return */
    __asm__ volatile (
        "lea 1f(%%rip), %%rax\n"
        "pushq $0x08\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movq $0x10, %%rax\n"
        "movq %%rax, %%ds\n"
        "movq %%rax, %%es\n"
        "movq %%rax, %%ss\n"
        "movq %%rax, %%fs\n"
        "movq %%rax, %%gs\n"
        ::: "rax", "memory");

    __asm__ volatile ("ltr %0" :: "r"((uint16_t)0x28));
}
