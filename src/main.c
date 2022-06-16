#include "shell.h"
#include "uart.h"
#include "timer.h"
#include "mm.h"
#include "thread.h"
#include "initrd.h"
#include "vfs.h"
#include "dev.h"
#include "sdhost.h"
#include "fat32.h"

#define CMD_LEN 128

void shell() {
	while(1) {
		char cmd[CMD_LEN];
		shell_input(cmd);
		shell_select(cmd);
	}
}

void main() {
	shell_init();
	timer_init();
	mm_init();
	cpio_init();
	rootfs_init();
	initramfs();
	dev_init();
	sd_init();
	fat32_init();
	printf( "Hello %s!\n"
            "This is character '%c', a hex number: %x and in decimal: %d\n"
            "Padding test: '%8x', '%8d'\n",
            "World", 'A', 32767, 32767, 0x7FFF, -123);
	init_thread();
	//shell();
	asm volatile("wfe\n");
}
