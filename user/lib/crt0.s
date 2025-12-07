.section .text.entry
.global _start
.type _start, @function

_start:
    #clear EBP for stack traces
    xorl %ebp, %ebp

    call main

    # Exit with return value from main
    movl %eax, %ebx
    movl $0, %eax       # SYS_EXIT = 0
    int $0x80

    #should never reach here
1:
    hlt
    jmp 1b

.size _start, . - _start
