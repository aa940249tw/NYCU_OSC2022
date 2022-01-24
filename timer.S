.section ".text"
#core timer interrupt

#define CORE0_TIMER_IRQ_CTRL 0x40000040

.global core_timer_enable
core_timer_enable:
    mov     x1, #1
    msr     cntp_ctl_el0, x1 // enable
    mrs     x1, cntfrq_el0
    lsl     x1, x1, #1
    msr     cntp_tval_el0, x1 // set expired time   
    mov     x2, #2
    ldr     x1, =0x40000040
    str     w2, [x1] // unmask timer interrupt
    ret

.global core_timer_disable
core_timer_disable:
    mov     x0, #0
    msr     cntp_ctl_el0, x0 // disable
    ret

.global svc_timer
svc_timer:
    svc     1
    ret
/*
.global core_timer_enable
core_timer_enable:
    mov     x0, #1
    msr     cntp_ctl_el0, x0 // enable 
    mrs     x1, cntfrq_el0
    lsl     x1, x1, #10
    msr     cntp_tval_el0, x1 // set expired time   
    mov     x0, #2
    ldr     x1, =0x40000040
    str     w0, [x1] // unmask timer interrupt
    ret
*/
