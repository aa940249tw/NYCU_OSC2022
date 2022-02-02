#include "irq.h"
#include "uart.h"
#include "timer.h"

void enable_interrupt() { 
    asm volatile("msr DAIFClr, 0xf"); 
}

void disable_interrupt() { 
    asm volatile("msr DAIFSet, 0xf"); 
}

void irq_handler(int type, unsigned long esr, unsigned long elr) {
    if(*IRQ_BASIC_PENDING & AUX_INT) {
        uart_handler();
    }
    else if(*CORE0_INTERRUPT_SOURCE & 0x2) {
        core_timer_handler();
    }
    else {
        printf("Exception eccurs!!!\n");
        if(type == 9)
            printf("Excpetion type:  irq_el64\tESR:  0x%x\tELR:  0x%x\n", esr, elr);
        else
            printf("Excpetion type:  irq_sp_elx\tESR:  0x%x\tELR:  0x%x\n", esr, elr);
    }
}
