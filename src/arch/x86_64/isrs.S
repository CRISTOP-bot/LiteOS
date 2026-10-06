/* Stubs de excepciones: empujan vector y saltan al handler comun */
.text
.extern exc_c

.macro ISR_ERR n, vec
.global idt_stub_\n
idt_stub_\n:
    pushq $\vec
    jmp isr_common
.endm

.macro ISR_ERRCODE n, vec
.global idt_stub_\n
idt_stub_\n:
    addq $8, %rsp   /* descartar error code de la CPU */
    pushq $\vec
    jmp isr_common
.endm

ISR_ERR 0, 0
ISR_ERR 1, 1
ISR_ERR 2, 2
ISR_ERR 3, 3
ISR_ERR 6, 6
ISR_ERRCODE 8, 8
ISR_ERRCODE 10, 10
ISR_ERRCODE 11, 11
ISR_ERRCODE 12, 12
ISR_ERRCODE 13, 13
ISR_ERRCODE 14, 14

.macro IRQ n, vec
.global idt_irq_\n
idt_irq_\n:
    pushq $\vec
    jmp irq_common
.endm

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.global irq_common
irq_common:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    subq $8, %rsp
    movq 80(%rsp), %rdi
    call irq_c
    addq $8, %rsp
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    addq $8, %rsp
    iretq

.extern irq_c

.global isr_common
isr_common:
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    movq 72(%rsp), %rdi   /* vector (saltado los 9 regs guardados + vector) */
    call exc_c
    addq $8, %rsp
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    addq $8, %rsp
    iretq
