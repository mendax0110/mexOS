.section .note.GNU-stack,"",%progbits
.section .text

.global switch_context
.global enter_usermode

switch_context:
    pushl %ebp
    movl %esp, %ebp

    # Save old context
    movl 8(%ebp), %eax
    testl %eax, %eax
    jz .skip_save

    movl %edi, 0(%eax)
    movl %esi, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esp, 12(%eax)
    movl %ebx, 16(%eax)
    movl %edx, 20(%eax)
    movl %ecx, 24(%eax)
    movl 4(%ebp), %ecx
    movl %ecx, 32(%eax)      # save EIP
    pushfl
    popl 36(%eax)

    # Save CR3 safely
    movl %cr3, %edx
    movl %edx, 40(%eax)

.skip_save:
    movl 12(%ebp), %eax
    testl %eax, %eax
    jz .done

    movb 45(%eax), %cl
    cmpb $0, %cl
    je .user_task

    # Kernel task restore
    movl 0(%eax), %edi
    movl 4(%eax), %esi
    movl 16(%eax), %ebx
    movl 20(%eax), %edx
    movl 24(%eax), %ecx

    pushl 36(%eax)
    popfl

    movl 12(%eax), %esp
    movl 8(%eax), %ebp

    movl 40(%eax), %edx
    testl %edx, %edx
    jz .skip_cr3_k
    movl %edx, %cr3
.skip_cr3_k:

    pushl 32(%eax)
    ret

.user_task:
    movl 0(%eax), %edi
    movl 4(%eax), %esi
    movl 16(%eax), %ebx
    movl 20(%eax), %edx
    movl 24(%eax), %ecx

    movl 40(%eax), %edx
    testl %edx, %edx
    jz .skip_cr3_u
    movl %edx, %cr3
.skip_cr3_u:

    movl 12(%eax), %esp
    jmp *32(%eax)

.done:
    popl %ebp
    ret

enter_usermode:
    movw $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    iret

.global user_task_trampoline
user_task_trampoline:
    iret
