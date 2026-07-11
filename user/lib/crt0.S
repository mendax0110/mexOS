#include "../shared/syscall_numbers.h"

.section .text.entry
.global _start
.type _start, @function

_start:
    #clear EBP for stack traces
    xorl %ebp, %ebp

    movl (%esp), %eax
    leal 4(%esp), %ebx
    pushl %ebx
    pushl %eax
    call main
    addl $8, %esp

    # Exit with return value from main
    movl %eax, %ebx
    movl $SYS_EXIT, %eax       # SYS_EXIT = 0
    int $0x80

    #should never reach here
1:
    hlt
    jmp 1b

.size _start, . - _start

.section .note.GNU-stack,"",@progbits
