#include "uart.h"
#include "reset.h"
#include "mbox.h"
#include "uart_boot.h"
#include "utils.h"
#include "__cpio.h"
#include "devicetree.h"
#include "exception.h"
#include "timer.h"

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
		uart_puts("\n\n");
		uart_puts("\tCommands:\n");
		uart_puts("\thelp:\t\tPrint this list.\n");
		uart_puts("\thello:\t\tPrint \"Hello World!\".\n");
		uart_puts("\treboot:\t\tReboot rpi3.\n");
		uart_puts("\tstatus:\t\tGet the hardware’s information.\n");
		uart_puts("\tload_img:\tLoad a new kernel image through uart.\n");
		uart_puts("\tls:\t\tList cpio archive files.\n");
		uart_puts("\tcat:\t\tEnter a filename to get file content.\n");
		uart_puts("\tdtb:\t\tGet devicetree info.\n");
		uart_puts("\tdie:\t\tTry Exception.\n");
		uart_puts("\tsettimeout [string] [time(sec)]: Set timeout to print message.\n\n");
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
		unsigned long cpio_address = get_initramfs("linux,initrd-start");
		printf("Found cpio file at address %x\n", cpio_address);
		cpio_ls(cpio_address);
	}
	else if(!strncmp(cmd, "cat", 3)) {
		printf("\n");
		cpio_cat(get_initramfs("linux,initrd-start"), cmd + 4);
	}
	else if(!strcmp(cmd, "dtb")) {
		printf("\n");
		dtb_info();
	}
	else if(!strcmp(cmd, "die")) {
		printf("\n");
		_exception_simulate();
	}
	else if(!strcmp(cmd, "time")) {
		printf("\n");
		svc_timer();
	}
	else if(!strncmp(cmd, "settimeout", 10)) {
		printf("\n");
		int i, j;
		int timeouts = 0;
    	for(i = 11; i < strlen(cmd); i++) {
      		if(cmd[i] == ' ') {
        		i++;
        		break;
     	 	}
    	}
		for(j = i; j < strlen(cmd); j++) {
			timeouts *= 10;
			timeouts += (cmd[j] - '0');
		}
		cmd[i-1] = '\0';
		add_timer((void *)core_timer_print_message_callback, (unsigned int)timeouts, cmd + 11, (unsigned int)strlen(cmd + 11));
		printf("Timer added.\n");
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

