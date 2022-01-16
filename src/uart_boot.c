#include "uart_boot.h"
#include "uart.h"

extern unsigned char __end, __start, __devicetree;

void reallocate() {
    unsigned char *start_address = (unsigned char *)&__start;
    unsigned char *end_address = (unsigned char *)&__end;
    unsigned char *new_address = (unsigned char *)NEW_ADDR;

    while(start_address <= end_address) {
        *new_address = *start_address;
        ++start_address;
        ++new_address;
    }

    printf("Kernel has reallocated to address 0x60000.\n");
    void (*func_ptr)() = load_img + NEW_ADDR - (unsigned long)&__start;
    func_ptr();
}

void load_img() {
    unsigned char **dtb_address = (unsigned char **)&__devicetree;
    printf("%x\n", *dtb_address);
    printf("Now trying to connect to uart.\n");
    int img_size = 0;

    for(int i = 0; i < 4; ++i) {
        img_size <<= 8; 
        img_size += (int)uart_getc_raw();
    }
    printf("Recieved Data!!!\n");
    printf("Image Size: %d\n", img_size);

    unsigned char *kernel_address = (unsigned char *)KERNEL_ADDR;

    for(int i = 0; i < img_size; ++i) {
        *(kernel_address + i) = uart_getc_raw();
    }
    
    printf("Kernel Image has loaded successfully!!!\n");
    void (*start_new_kernel)(unsigned long) = (void *)KERNEL_ADDR;
    start_new_kernel((unsigned long)*dtb_address);
}