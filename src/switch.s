.global context_switch
.type context_switch, @function

// void context_switch(ProcessContext* old_ctx, ProcessContext* new_ctx)
context_switch:
    // Stack layout on entry:
    // esp+4 = old_ctx pointer
    // esp+8 = new_ctx pointer

    // Save arguments to registers for easier access
    mov 4(%esp), %eax    // old_ctx
    mov 8(%esp), %edx    // new_ctx

    // Save all general-purpose registers to old_ctx
    mov %edi, 0(%eax)
    mov %esi, 4(%eax)
    mov %ebp, 8(%eax)

    // Save current esp to old_ctx->esp (pointing to this function's frame)
    lea 4(%esp), %ecx        // Address of return address on stack
    mov %ecx, 12(%eax)

    mov %ebx, 16(%eax)
    // edx is temporarily holding new_ctx, so save edx *after*
    push %edx
    mov %edx, 20(%eax)
    pop %edx

    mov %ecx, 24(%eax)       // We just used ecx as esp+4, save it as ecx for simplicity
    mov %eax, 28(%eax)       // Save eax last after using it for old_ctx pointer

    mov (%esp), %ecx          // SS
    mov %ecx, 36(%eax)
    mov 4(%esp), %ecx         // ESP
    mov %ecx, 40(%eax)
    mov 8(%esp), %ecx         // EFLAGS
    mov %ecx, 44(%eax)
    mov 12(%esp), %ecx        // CS
    mov %ecx, 48(%eax)
    mov 16(%esp), %ecx        // EIP
    mov %ecx, 32(%eax)        // Save EIP just after general regs

    // Now restore registers from new_ctx
    mov 0(%edx), %edi
    mov 4(%edx), %esi
    mov 8(%edx), %ebp
    mov 12(%edx), %esp      // Load new stack pointer
    mov 16(%edx), %ebx
    mov 20(%edx), %edx
    mov 24(%edx), %ecx
    mov 28(%edx), %eax

    // Push new context's SS, ESP, EFLAGS, CS, EIP for iret
    mov 36(%edx), %ecx     // SS
    push %ecx
    mov 40(%edx), %ecx     // ESP (user_esp)
    push %ecx
    mov 44(%edx), %ecx     // EFLAGS
    push %ecx
    mov 48(%edx), %ecx     // CS
    push %ecx
    mov 32(%edx), %ecx     // EIP
    push %ecx

    iret                    // Return to new context user mode

    // Should never reach here
    hlt
