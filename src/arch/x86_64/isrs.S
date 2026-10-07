/* LiteOS x86_64: stubs de excepcion/IRQ/syscall, cambio de contexto
 * y primera entrada a modo usuario.
 *
 * Frame en la pila del kernel (ver struct trap_frame en kernel.h):
 *   [rax rcx rdx rsi rdi r8 r9 r10 r11][r15 r14 r13 r12 rbx rbp]
 *   [vector][err][rip][cs][rflags][rsp][ss]
 * Los dos ultimos pares los empuja la CPU (interrupciones) o el stub
 * (excepciones sin codigo de error: se empuja un 0 ficticio para
 * uniformar el frame).
 */

.extern trap_handler
.extern current
.extern first_user_frame

.macro STUB n
.global idt_stub_\n
idt_stub_\n:
    pushq $0
    pushq $\n
    jmp trap_common
.endm

.macro STUB_ERR n
.global idt_stub_\n
idt_stub_\n:
    pushq $\n
    jmp trap_common
.endm

.macro IRQ_STUB n vec
.global idt_irq_\n
idt_irq_\n:
    pushq $0
    pushq $\vec
    jmp trap_common
.endm

/* ---- Excepciones de CPU (0..31) ---- */
STUB      0       /* #DE  division por cero */
STUB      1       /* #DB  debug */
STUB      2       /* NMI */
STUB      3       /* #BP  breakpoint */
STUB      4       /* #OF  overflow */
STUB      5       /* #BR  bounds */
STUB      6       /* #UD  opcode invalido */
STUB      7       /* #NM  coprocesador no disponible */
STUB_ERR  8       /* #DF  doble fallo */
STUB      9       /* coprocesor segment overrun */
STUB_ERR  10      /* #TS  TSS invalido */
STUB_ERR  11      /* #NP  segmento no presente */
STUB_ERR  12      /* #SS  stack fault */
STUB_ERR  13      /* #GP  proteccion general */
STUB_ERR  14      /* #PF  page fault */
STUB      15      /* reservado */
STUB      16      /* #MF  FPU */
STUB_ERR  17      /* #AC  alignment check */
STUB      18      /* #MC  machine check */
STUB_ERR  19      /* #XM  SIMD */
STUB      20      /* #PK  virtualization */
STUB      21
STUB      22
STUB      23
STUB      24
STUB      25
STUB      26
STUB      27
STUB      28
STUB      29
STUB      30
STUB      31

/* ---- IRQs 0..15 -> vectores 32..47 ---- */
IRQ_STUB  0, 32
IRQ_STUB  1, 33
IRQ_STUB  2, 34
IRQ_STUB  3, 35
IRQ_STUB  4, 36
IRQ_STUB  5, 37
IRQ_STUB  6, 38
IRQ_STUB  7, 39
IRQ_STUB  8, 40
IRQ_STUB  9, 41
IRQ_STUB  10, 42
IRQ_STUB  11, 43
IRQ_STUB  12, 44
IRQ_STUB  13, 45
IRQ_STUB  14, 46
IRQ_STUB  15, 47

/* ---- int 0x80: syscalls ---- */
.global idt_int80
idt_int80:
    pushq $0
    pushq $0x80
    jmp trap_common

/* ---- Camino comun ---- */
trap_common:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rax
    movq  %rsp, %rdi
    call  trap_handler
    popq  %rax
    popq  %rcx
    popq  %rdx
    popq  %rsi
    popq  %rdi
    popq  %r8
    popq  %r9
    popq  %r10
    popq  %r11
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    addq  $16, %rsp
    iretq

/* ---- Cambio de contexto ----
 * void switch_to(struct proc *next)
 * ksp es el primer campo de struct proc (offset 0).
 * Salva los registros callee-saved en la pila del proceso
 * actual y restaura los del siguiente. */
.global switch_to
switch_to:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq  current(%rip), %rax
    movq  %rsp, (%rax)
    movq  %rdi, current(%rip)
    movq  (%rdi), %rsp
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    retq

/* El frame del hijo empieza justo encima de la dirección de retorno. */
.global proc_child_entry
proc_child_entry:
    movq %rsp, %rdi
    jmp user_enter

/* ---- Primera entrada a modo usuario ----
 * void user_enter(struct trap_frame *frame)  (nunca retorna) */
.global user_enter
user_enter:
    movq  %rdi, %rsp
    popq  %rax
    popq  %rcx
    popq  %rdx
    popq  %rsi
    popq  %rdi
    popq  %r8
    popq  %r9
    popq  %r10
    popq  %r11
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    addq  $16, %rsp
    iretq

/* Trampolin para el primer proceso: devuelve el frame de
 * entrada y salta a user_enter. */
.global proc_trampoline
proc_trampoline:
    movq  first_user_frame(%rip), %rdi
    jmp   user_enter

/* Tabla de stubs para la IDT (excepciones + IRQs). */
.section .data
.global idt_stub_table
idt_stub_table:
    .quad idt_stub_0
    .quad idt_stub_1
    .quad idt_stub_2
    .quad idt_stub_3
    .quad idt_stub_4
    .quad idt_stub_5
    .quad idt_stub_6
    .quad idt_stub_7
    .quad idt_stub_8
    .quad idt_stub_9
    .quad idt_stub_10
    .quad idt_stub_11
    .quad idt_stub_12
    .quad idt_stub_13
    .quad idt_stub_14
    .quad idt_stub_15
    .quad idt_stub_16
    .quad idt_stub_17
    .quad idt_stub_18
    .quad idt_stub_19
    .quad idt_stub_20
    .quad idt_stub_21
    .quad idt_stub_22
    .quad idt_stub_23
    .quad idt_stub_24
    .quad idt_stub_25
    .quad idt_stub_26
    .quad idt_stub_27
    .quad idt_stub_28
    .quad idt_stub_29
    .quad idt_stub_30
    .quad idt_stub_31
    .quad idt_irq_0
    .quad idt_irq_1
    .quad idt_irq_2
    .quad idt_irq_3
    .quad idt_irq_4
    .quad idt_irq_5
    .quad idt_irq_6
    .quad idt_irq_7
    .quad idt_irq_8
    .quad idt_irq_9
    .quad idt_irq_10
    .quad idt_irq_11
    .quad idt_irq_12
    .quad idt_irq_13
    .quad idt_irq_14
    .quad idt_irq_15
