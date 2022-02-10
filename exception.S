.section ".text"

// save general registers to stack
.macro save_all
    sub     sp, sp, 32 * 9
    stp     x0, x1, [sp ,16 * 0]
    stp     x2, x3, [sp ,16 * 1]
    stp     x4, x5, [sp ,16 * 2]
    stp     x6, x7, [sp ,16 * 3]
    stp     x8, x9, [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    str     x30, [sp, 16 * 15]
    mrs     x22, elr_el1
    mrs     x23, spsr_el1
    stp     x22, x23, [sp, #16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp     x22, x23, [sp, #16 * 16]
    msr     elr_el1, x22
    msr     spsr_el1, x23
    ldp     x0, x1, [sp ,16 * 0]
    ldp     x2, x3, [sp ,16 * 1]
    ldp     x4, x5, [sp ,16 * 2]
    ldp     x6, x7, [sp ,16 * 3]
    ldp     x8, x9, [sp ,16 * 4]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x28, x29, [sp ,16 * 14]
    ldr     x30, [sp, 16 * 15]
    add     sp, sp, 32 * 9
.endm

exception_handler:
    bl      _get_exception_info
    bl      exception_entry
    load_all
    eret

.align 11                       // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    b       synchronous_sp_el0  // branch to a handler function.
    .align 7                    // entry size is 0x80, .align will pad 0
    b       irq_sp_el0
    .align 7
    b       fiq_sp_el0
    .align 7
    b       serror_sp_el0
    .align 7

    b       synchronous_sp_elx
    .align 7
    b       irq_sp_elx
    .align 7
    b       fiq_sp_elx
    .align 7
    b       serror_sp_elx
    .align 7
    b       synchronous_sp_el64
    .align 7
    b       irq_el64
    .align 7
    b       fiq_el64
    .align 7
    b       serror_el64
    .align 7

    b       synchronous_el32
    .align 7
    b       irq_el32
    .align 7
    b       fiq_el32
    .align 7
    b       serror_el32
    .align 7

.global _set_exception_vector_table
_set_exception_vector_table:
    adr     x0, exception_vector_table
    msr     vbar_el1, x0
    ret

.global _get_exception_info
_get_exception_info:
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    ret

.global _exception_simulate
_exception_simulate:
    // Simulate Supervisor Call (SVC) instruction enables User mode programs to request an OS service
    mov     x0, 0
1:
    add     x0, x0, 1
    svc     0
    cmp     x0, 5
    blt     1b
    ret

synchronous_sp_el0:
    save_all
    mov     x0, #0
    b       exception_handler


irq_sp_el0:
    save_all
    mov     x0, #1
    b       exception_handler

fiq_sp_el0:
    save_all
    mov     x0, #2
    b       exception_handler

serror_sp_el0:
    save_all
    mov     x0, #3
    b       exception_handler

synchronous_sp_elx:
    save_all
    mov     x0, #4
    bl      _get_exception_info
    bl      svc_handler
    load_all
    eret

irq_sp_elx:
    msr     DAIFSet, 0xf
    save_all
    mov     x0, #5
    bl      _get_exception_info
    bl      irq_handler
    load_all
    msr     DAIFClr, 0xf
    eret

fiq_sp_elx:
    save_all
    mov     x0, #6
    b       exception_handler

serror_sp_elx:
    save_all
    mov     x0, #7
    b       exception_handler

synchronous_sp_el64:
    save_all
    mov     x0, #8 
    bl      _get_exception_info
    bl      svc_handler
    load_all
    eret

irq_el64:
    msr     DAIFSet, 0xf
    save_all
    mov     x0, #9
    bl      _get_exception_info
    bl      irq_handler
    load_all
    msr     DAIFClr, 0xf
    eret
    
fiq_el64:
    save_all
    mov     x0, #10
    b       exception_handler

serror_el64:
    save_all
    mov     x0, #11
    b       exception_handler

synchronous_el32:
    save_all
    mov     x0, #12
    b       exception_handler

irq_el32:
    save_all
    mov     x0, #13
    b       exception_handler

fiq_el32:
    save_all
    mov     x0, #14
    b       exception_handler

serror_el32:
    save_all
    mov     x0, #16
    b       exception_handler