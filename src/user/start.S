#include <sys/liteos_numbers.h>
.global _start
.type _start, @function
_start:
    andq $-16, %rsp
    call main
    movl %eax, %edi
    movl $SYS_exit, %eax
    int $0x80
1:  jmp 1b
.section .note.GNU-stack,"",@progbits
