; switch.s - Proper NASM syntax for x86 privilege switching
section .text
global switch_to_userspace_asm

switch_to_userspace_asm:
    mov eax, [esp+4]    ; Load EIP (entry point)
    mov esp, [esp+8]    ; Load new stack pointer

    ; Set up stack for iret
    push 0x23           ; User data segment (SS)
    push esp            ; ESP
    push 0x200          ; EFLAGS (interrupts enabled)
    push 0x1B           ; User code segment (CS)
    push eax            ; EIP

    iretd               ; Interrupt return (to userspace)

