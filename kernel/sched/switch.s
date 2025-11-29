.section .note.GNU-stack,"",%progbits
.section .text
.global switch_context

# void switch_context(struct task_context* old, struct task_context* new_ctx)
switch_context:
    # Check if old context is NULL
    mov 4(%esp), %eax
    test %eax, %eax
    jz .restore_only

    # Save current context to old
    mov %edi, 0(%eax)
    mov %esi, 4(%eax)
    mov %ebp, 8(%eax)
    mov %esp, 12(%eax)
    mov %ebx, 16(%eax)
    mov %edx, 20(%eax)
    mov %ecx, 24(%eax)
    pushfl
    pop 36(%eax)

    # Save return address
    mov (%esp), %ecx
    mov %ecx, 32(%eax)

.restore_only:
    # Load new context
    mov 8(%esp), %eax
    test %eax, %eax
    jz .done

    # Restore registers
    mov 0(%eax), %edi
    mov 4(%eax), %esi
    mov 8(%eax), %ebp
    mov 16(%eax), %ebx
    mov 20(%eax), %edx
    mov 24(%eax), %ecx

    # Restore EFLAGS
    push 36(%eax)
    popfl

    # Switch stack
    mov 12(%eax), %esp

    # Jump to new EIP
    push 32(%eax)
    ret

.done:
    ret
