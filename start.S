#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_ACCESS (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

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
    // change execution level to EL1
    mov     x2, #0x3c5
    msr     spsr_el2, x2
    adr     x2, _el1_entry
    msr     elr_el2, x2
    eret

_el1_entry:
     // set_exception_vector_table
    bl      _set_exception_vector_table
    // setup tcr
    ldr     x0, =TCR_CONFIG_DEFAULT
    msr     tcr_el1, x0
    // setup mair
    ldr x0, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr     mair_el1, x0
    // create page table
    bl      page_table_setup

_clear_bbs:
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

_jump_main:
    ldr     x1, =0x60000
    ldr     x2, =kernel_virt
    add     x1, x1, x2
    msr     sp_el0, x1
    msr     DAIFClr, 0xf
    // jump to C code, should not return
4:  bl      main
    // for failsafe, halt this core too
    b       1b

page_table_setup:
    mov     x0, 0           // PGD's page frame at 0x0
    mov     x1, 0x1000      // PUD's page frame at 0x1000

    ldr     x2, = BOOT_PGD_ATTR
    orr     x2, x1, x2      // combine the physical address of next level page with attribute.
    str     x2, [x0]

    ldr     x2, = BOOT_PUD_ATTR
    mov     x3, 0x00000000
    orr     x3, x2, x3
    str     x3, [x1]        // 1st 1GB mapped by the 1st entry of PUD
    mov     x3, 0x40000000
    orr     x3, x2, x3
    str     x3, [x1, 8]     // 2nd 1GB mapped by the 2nd entry of PUD

    msr    ttbr0_el1, x0    // load PGD to the bottom translation based register.
    msr    ttbr1_el1, x0    // also load PGD to the upper translation based register.
    mrs    x3, sctlr_el1
    orr    x3, x3, 1
    msr    sctlr_el1, x3    // enable MMU, cache remains disabled
    ldr    x2, =_jump_main  // indirect branch to the virtual address
    br     x2

/*test jump to el0 */ 
1:  b    shell

.global _from_el1_to_el0
_from_el1_to_el0:
    mov     x2, 0
    msr     spsr_el1, x2
    adr     x2, 1b
    msr     elr_el1, x2
    eret

