#include "irq.h"
#include "uart.h"

void enable_interrupt() { 
    asm volatile("msr DAIFClr, 0xf"); 
}

void disable_interrupt() { 
    asm volatile("msr DAIFSet, 0xf"); 
}

void irq_handler() {
    disable_interrupt();
    if(*IRQ_BASIC_PENDING & AUX_INT) {
        uart_puts("irq\n");
        uart_handler();
    }
    //uart_handler();
    //uart_puts("irq\n");
    enable_interrupt();
}
