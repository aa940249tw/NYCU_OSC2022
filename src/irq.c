#include "irq.h"
#include "uart.h"
#include "timer.h"

void enable_interrupt() { 
    asm volatile("msr DAIFClr, 0xf"); 
}

void disable_interrupt() { 
    asm volatile("msr DAIFSet, 0xf"); 
}

void irq_handler() {
    if(*IRQ_BASIC_PENDING & AUX_INT) {
        uart_handler();
    }
    else if(*CORE0_INTERRUPT_SOURCE & 0x2) {
        core_timer_handler();
    }
}
