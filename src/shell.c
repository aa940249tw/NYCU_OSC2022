#include "uart.h"
#include "reset.h"
#include "mbox.h"
#include "uart_boot.h"
#include "utils.h"
#include "__cpio.h"

void shell_init() {
	uart_init();
	uart_flush();
	uart_puts("Welcome to the system !!!\n");
}

void shell_select(char *cmd) {
	if(!strcmp(cmd, "")) {
		printf("\n");
		return;
	}
	else if(!strcmp(cmd, "help")) {
		uart_puts("\nhelp:\t\tPrint this list.\n");
		uart_puts("hello:\t\tPrint \"Hello World!\".\n");
		uart_puts("reboot:\t\tReboot rpi3.\n");
		uart_puts("status:\t\tGet the hardware’s information.\n");
		uart_puts("load_img:\tLoad a new kernel image through uart.\n");
		uart_puts("ls:\tList cpio archive files.\n");
		uart_puts("cat:\tEnter a filename to get file content.\n");
	}
	else if(!strcmp(cmd, "hello")) {
		uart_puts("\nHello World!\n");
	}
	else if(!strcmp(cmd, "reboot")) {
		uart_puts("\nRebooting the device....\n");
		reset(0);
		while(1);
	}
	else if(!strcmp(cmd, "status")) {
		get_revision();
		get_address();
	}
	else if(!strcmp(cmd, "load_img")) {
		uart_puts("\nStarting loading process...\n");
		reallocate();
	}
	else if(!strcmp(cmd, "ls")) {
		printf("\n");
		cpio_ls();
	}
	else if(!strcmp(cmd, "cat")) {
		printf(" ");
		cpio_cat();
	}
	else if(cmd[0] != '\0') uart_puts("\nshell: command not found.\n");
}

void shell_input(char *cmd) {
	uart_puts("\r# ");
	char c;
	int idx = 0, end = 0;
	cmd[0] = '\0';
	while((c = uart_getc()) != '\n') {
		if(c == 8 || c == 127) {
			if (idx > 0) {
                idx--;
                for (int i = idx; i < end; i++) {
                    cmd[i] = cmd[i + 1];
                }
                cmd[--end] = '\0';
            }
		}
		else if (c == 3) {
            cmd[0] = '\0';
            break;
        }
        else if(c == 27) {
        	c = uart_getc();
        	c = uart_getc();
        	switch (c) {
        	case 'C' :
        		if (idx < end) idx++;
        		break;
        	case 'D' :
        		if (idx > 0) idx--;
        		break;
        	default : uart_flush();
        	}
        }
		else {
			if (idx < end) {
                for (int i = end; i > idx; i--) {
                    cmd[i] = cmd[i - 1];
                }
            }
			cmd[idx++] = c;
            cmd[++end] = '\0';
		}
		printf("\r# %s \r\e[%dC", cmd, idx + 2);
	}
}
