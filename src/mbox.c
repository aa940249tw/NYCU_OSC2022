#include "gpio.h"
#include "uart.h"
#include "mbox.h"

volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

#define MBOX_BASE    	MMIO_BASE + 0x0000B880

#define MBOX_READ    	((volatile unsigned int*)(MBOX_BASE + 0x0))
#define MBOX_POLL    	((volatile unsigned int*)(MBOX_BASE + 0x10))
#define MBOX_SENDER  	((volatile unsigned int*)(MBOX_BASE + 0x14))
#define MBOX_STATUS  	((volatile unsigned int*)(MBOX_BASE + 0x18))
#define MBOX_CONFIG  	((volatile unsigned int*)(MBOX_BASE + 0x1C))
#define MBOX_WRITE   	((volatile unsigned int*)(MBOX_BASE + 0x20))

#define MBOX_EMPTY   	0x40000000
#define MBOX_FULL    	0x80000000
#define MBOX_RESPONSE   0x80000000

int mbox_call(unsigned char ch)
{
    unsigned int r = (((unsigned int)((unsigned long)&mailbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */

    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */

        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mailbox[1]==MBOX_RESPONSE;
    }
    return 0;
}

void get_revision() {
	mailbox[0] = 7 * 4; // buffer size in bytes
  	mailbox[1] = REQUEST_CODE;
  	// tags begin
  	mailbox[2] = GET_BOARD_REVISION; // tag identifier
  	mailbox[3] = 4; // maximum of request and response value buffer's length.
  	mailbox[4] = TAG_REQUEST_CODE;
  	mailbox[5] = 0; // value buffer
  	// tags end
  	mailbox[6] = END_TAG;
	if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("\nMy board revision is: ");
        uart_hex(mailbox[5]);
        uart_puts("\n");
    } 
    else	
    	uart_puts("\nUnable to query serial!\n");
}

void get_address() {
	mailbox[0] = 8 * 4; // buffer size in bytes
  	mailbox[1] = REQUEST_CODE;
  	// tags begin
  	mailbox[2] = GET_ARM_ADDRESS; // tag identifier
  	mailbox[3] = 8; // maximum of request and response value buffer's length.
  	mailbox[4] = TAG_REQUEST_CODE;
  	mailbox[5] = 0; // value buffer
  	mailbox[6] = 0;
  	// tags end
  	mailbox[7] = END_TAG;
	if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("\nMy ARM memory base address is: ");
        uart_hex(mailbox[5]);
        uart_puts("\nMy ARM memory size is: ");
        uart_hex(mailbox[6]);
        uart_puts("\n");
    } 
    else	
    	uart_puts("\nUnable to query serial!\n");
}
