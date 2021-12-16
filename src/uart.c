#include "gpio.h"
#include "sprintf.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

extern volatile unsigned char _end;

void uart_init()
{
    register unsigned int r;
	
	// Initiallize UART
    *AUX_ENABLE |=1;		// Enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;	   	// Disable TX, RX during initiallization 
    *AUX_MU_LCR = 3;       	// Set data size to 8 bits
    *AUX_MU_MCR = 0;		// Don't need auto flow control (??)
    *AUX_MU_IER = 0;		// Disable interrupts
    *AUX_MU_IIR = 0xc6;    	// No FIFO
    *AUX_MU_BAUD = 270;    	// Set baud rate to 115200 

	// Map UART to GPIO 
	
	// Change gpio 14, gpio 15 to function 'ALT5'
    r = *GPFSEL1;
    r &= ~ ((7<<12) | (7<<15)); // Reset gpio14, gpio15
    r |= (2<<12) | (2<<15);    	// Set to alternate function 'alt5'
    *GPFSEL1 = r;
    
    *GPPUD = 0;            	// Enable pins 14 and 15 (??)
    r = 150; 
    while (r--) asm volatile("nop");// wait 150 cycles
    *GPPUDCLK0 = (1<<14) | (1<<15);	// Clock the control signal into the GPIO pads (??)
    r = 150; 
    while (r--) asm volatile("nop");// wait 150 cycles
    *GPPUDCLK0 = 0;        	// 」lush GPIO setup
    *AUX_MU_CNTL = 3;      	// Enable Tx, Rx
}

void uart_send(unsigned int c) {
	// Wait until we can send, check transmitter idle field
    do {
    	asm volatile("nop");
    } while(!(*AUX_MU_LSR & 0x20));
    // write
    *AUX_MU_IO = c;
}

char uart_getc() {
    char r;
    // Check data ready field, wait until something is in the buffer
    do {
    	asm volatile("nop");
    } while(!(*AUX_MU_LSR & 0x01));
    // read
    r = (char)(*AUX_MU_IO);
    // Convert carrige return to newline
    return r == '\r' ? '\n' : r;
}

void uart_puts(char *s) {
    while(*s) {
        if(*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

void uart_flush() {
	// Wait until nothing is in the buffer
    while (*AUX_MU_LSR & 0x01) {
        *AUX_MU_IO;
    }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n=(d>>c)&0xF;
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

void printf(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    // we don't have memory allocation yet, so we
    // simply place our string after our code
    char *s = (char*)&_end;
    // use sprintf to format our string
    vsprintf(s,fmt,args);
    // print out as usual
    while(*s) {
        /* convert newline to carrige return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}
