.section ".text.boot"

.global _start

_start:
    // save devicetree address to specified address
    ldr     x1, =__devicetree
    str     x0, [x1]
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

_prog_hang:
    // cpu id > 0, stop
1:  wfe
    b       1b

2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

_clear_bbs:
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

_from_el2_to_el1:
    ldr     x1, =_start
    msr     sp_el1, x1
    // enable CNTP for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    // enable AArch64 in EL1
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    // Setup SCTLR access
    mov     x2, #0x0800
    movk    x2, #0x30d0, lsl #16
    msr     sctlr_el1, x2
    // make el0, el1 can use Floating point and Advanced SIMD
    mrs     x0, cpacr_el1
    orr     x0, x0, #(3 << 20)
    msr     cpacr_el1, x0
    // set_exception_vector_table
    bl      _set_exception_vector_table
    // change execution level to EL1
    mov     x2, #0x3c5
    msr     spsr_el2, x2
    adr     x2, _jump_main
    msr     elr_el2, x2
    eret

1:  b    shell

.global _from_el1_to_el0
_from_el1_to_el0:
    mov     x2, 0
    msr     spsr_el1, x2
    adr     x2, 1b
    msr     elr_el1, x2
    eret

_jump_main:
    ldr     x1, =0x60000
    msr     sp_el0, x1
    msr     DAIFClr, 0xf
    // jump to C code, should not return
4:  bl      main
    // for failsafe, halt this core too
    b       1b

