.section ".text"

.global switch_to
switch_to:
    stp     x19, x20, [x0, 16 * 0]
    stp     x21, x22, [x0, 16 * 1]
    stp     x23, x24, [x0, 16 * 2]
    stp     x25, x26, [x0, 16 * 3]
    stp     x27, x28, [x0, 16 * 4]
    stp     fp, lr, [x0, 16 * 5]
    mrs     x9, sp_el0
    mov     x8, sp
    stp     x9, x8, [x0, 16 * 6]
    
    ldp     x19, x20, [x1, 16 * 0]
    ldp     x21, x22, [x1, 16 * 1]
    ldp     x23, x24, [x1, 16 * 2]
    ldp     x25, x26, [x1, 16 * 3]
    ldp     x27, x28, [x1, 16 * 4]
    ldp     fp, lr, [x1, 16 * 5]
    ldp     x9, x8, [x1, 16 * 6]
    msr     sp_el0,  x9
    mov     sp,  x8
    msr     tpidr_el1, x1
    ret

/*
.global switch_to
switch_to:
    stp     x19, x20, [x0, 16 * 0]
    stp     x21, x22, [x0, 16 * 1]
    stp     x23, x24, [x0, 16 * 2]
    stp     x25, x26, [x0, 16 * 3]
    stp     x27, x28, [x0, 16 * 4]
    stp     fp, lr, [x0, 16 * 5]
    mrs     x9, sp_el0
    mov     x8, sp
    stp     x9, x8, [x0, 16 * 6]
    ldp     x9, x8, [x1, 16 * 6]
    mov     sp,  x8
    msr     tpidr_el1, x1
    mov     x0, x1
    b       check_posix
*/
.global load_context
load_context:
    ldp     x19, x20, [x0, 16 * 0]
    ldp     x21, x22, [x0, 16 * 1]
    ldp     x23, x24, [x0, 16 * 2]
    ldp     x25, x26, [x0, 16 * 3]
    ldp     x27, x28, [x0, 16 * 4]
    ldp     fp, lr, [x0, 16 * 5]
    ldp     x9, x8, [x0, 16 * 6]
    msr     sp_el0,  x9
    mov     sp,  x8
    msr     tpidr_el1, x0
    ret


.global get_current
get_current:
    mrs     x0, tpidr_el1
    ret

.global run_posix
run_posix:
    msr    elr_el1,  x0    
    msr    sp_el0, x1
    mov    x0, #0        
    msr    spsr_el1, x0    
    mov    x30, x2         
    eret