.global _start
.type _start, @function
_start:
    andq $-16, %rsp
    call main
    movl %eax, %edi
    movl $60, %eax
    int $0x80
1:  jmp 1b
.section .note.GNU-stack,"",@progbits
