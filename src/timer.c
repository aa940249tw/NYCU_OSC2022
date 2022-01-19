#include "timer.h"
#include "uart.h"

void get_coretime() {
    unsigned int count, freq;
    int time = 0;
    asm volatile("mrs  %[result], cntpct_el0": [result]"=r"(count));
    asm volatile("mrs  %[result], cntfrq_el0": [result]"=r"(freq));
    time = (1000 * count) / freq;
    printf("[%2d] core timer interrupt\n", time);
}

void core_timer_handler() {
    asm volatile("mrs x0, cntfrq_el0\n" "msr cntp_tval_el0, x0\n");
    asm volatile("mrs     x0, cntfrq_el0\n"
                 "lsl     x0, x0, #1\n"
                 "msr     cntp_tval_el0, x0\n");
    get_coretime();
}