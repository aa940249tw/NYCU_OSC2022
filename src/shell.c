#include "uart.h"
#include "reset.h"
#include "mbox.h"


int strcmp(const char *X, const char *Y) {
    while (*X) {
        if (*X != *Y)
            break;
        X++;
        Y++;
    }
    return *(const unsigned char *)X - *(const unsigned char *)Y;
}

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
		uart_puts("\nhelp:\tprint this list.\n");
		uart_puts("hello:\tprint \"Hello World!\".\n");
		uart_puts("reboot:\treboot rpi3.\n");
		uart_puts("status:\tget the hardware’s information.\n");
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
		//printf("%d %d\n", idx, end);
		//printf("\r# %s \r", cmd);
	}
}

