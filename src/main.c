#include "shell.h"
#include "uart.h"
#include "timer.h"
#define CMD_LEN 128

void main() {
	shell_init();
	timer_init();
	printf( "Hello %s!\n"
            "This is character '%c', a hex number: %x and in decimal: %d\n"
            "Padding test: '%8x', '%8d'\n",
            "World", 'A', 32767, 32767, 0x7FFF, -123);
	while(1) {
		char cmd[CMD_LEN];
		shell_input(cmd);
		shell_select(cmd);
	}
}
