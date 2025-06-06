.global context_switch
.type context_switch, @function

context_switch:
    # Save old context
    mov 4(%esp), %eax       # old_ctx (first argument)

    # Save registers
    mov %edi, 0(%eax)
    mov %esi, 4(%eax)
    mov %ebp, 8(%eax)
    mov %ebx, 12(%eax)
    mov %edx, 16(%eax)
    mov %ecx, 20(%eax)
    mov %esp, 24(%eax)

    # Save return address (EIP)
    mov (%esp), %ecx
    mov %ecx, 28(%eax)

    # Restore new context
    mov 8(%esp), %eax       # new_ctx (second argument)

    # Restore registers
    mov 0(%eax), %edi
    mov 4(%eax), %esi
    mov 8(%eax), %ebp
    mov 12(%eax), %ebx
    mov 16(%eax), %edx
    mov 20(%eax), %ecx

    # Switch stack
    mov 24(%eax), %esp

    # Return to new EIP
    mov 28(%eax), %ecx
    jmp *%ecx