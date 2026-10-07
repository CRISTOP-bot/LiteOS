#include "kernel.h"

extern int need_resched;
extern void syscall_dispatch(struct trap_frame *tf);

void trap_handler(struct trap_frame *f)
{
    if (f->vector == 0x80) {
        syscall_dispatch(f);
        return;
    }
    if (f->vector >= 32 && f->vector < 48) {
        int irq = (int)f->vector - 32;
        if (irq == 0) timer_tick();
        if (irq == 1) keyboard_irq();
        pic_eoi(irq);
        if (irq == 0 && need_resched) schedule();
        return;
    }
    kvprintf("trap: vector=%u rip=%p addr=%p err=%p\n",
             (u64)f->vector, f->rip, cr2_read(), f->err);
    if ((f->cs & 3) == 3) {
        proc_exit(128 + SIGSEGV);
        return;
    }
    qemu_exit(0xFF);
    for (;;) __asm__ volatile ("hlt");
}
