.section .note.GNU-stack,"",%progbits
.section .text

.global switch_context
.global enter_usermode

#
# void switch_context(struct task_context* old, struct task_context* new_ctx)
#
# Kernel-to-kernel context switch.
# Saves current context to old, restores from new_ctx.
#
switch_context:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax
    testl %eax, %eax
    jz .load_new

    movl %edi, 0(%eax)
    movl %esi, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esp, 12(%eax)
    movl %ebx, 16(%eax)
    movl %edx, 20(%eax)
    movl %ecx, 24(%eax)

    movl 4(%ebp), %ecx
    movl %ecx, 32(%eax)

    pushfl
    popl 36(%eax)

.load_new:
    movl 12(%ebp), %eax
    testl %eax, %eax
    jz .switch_done

    movl 0(%eax), %edi
    movl 4(%eax), %esi
    movl 16(%eax), %ebx
    movl 20(%eax), %edx
    movl 24(%eax), %ecx

    pushl 36(%eax)
    popfl

    movl 12(%eax), %esp
    movl 8(%eax), %ebp

    pushl 32(%eax)
    ret

.switch_done:
    popl %ebp
    ret

#
# void enter_usermode(uint32_t entry, uint32_t user_stack, uint32_t cs, uint32_t ds)
#
# Transitions to user mode by building and executing an iret frame.
# Parameters:
#   entry      - User-space entry point (EIP)
#   user_stack - User-space stack pointer (ESP)
#   cs         - User code segment selector (0x1B)
#   ds         - User data segment selector (0x23)
#
enter_usermode:
    movl 4(%esp), %eax
    movl 8(%esp), %ebx
    movl 12(%esp), %ecx
    movl 16(%esp), %edx

    movw %dx, %ds
    movw %dx, %es
    movw %dx, %fs
    movw %dx, %gs

    pushl %edx
    pushl %ebx
    pushl $0x202
    pushl %ecx
    pushl %eax

    iret
