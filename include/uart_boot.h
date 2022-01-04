#ifndef __UART_BOOT_H__
#define __UART_BOOT_H__

#define NEW_ADDR    0x60000
#define KERNEL_ADDR 0x80000

void reallocate();
void load_img();

#endif