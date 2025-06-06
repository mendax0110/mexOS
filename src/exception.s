.global divide_by_zero_handler
.global double_fault_handler
.global general_protection_fault_handler
.global page_fault_handler
.extern exception_handler_wrapper

.macro EXCEPTION_HANDLER num
    cli
    push $\num      # Push exception number
    jmp exception_common
.endm

exception_common:
    pusha           # Save all general purpose registers
    call exception_handler_wrapper
    popa
    add $4, %esp    # Remove exception number
    iret

divide_by_zero_handler:
    EXCEPTION_HANDLER 0

double_fault_handler:
    EXCEPTION_HANDLER 8

general_protection_fault_handler:
    EXCEPTION_HANDLER 13

page_fault_handler:
    EXCEPTION_HANDLER 14
