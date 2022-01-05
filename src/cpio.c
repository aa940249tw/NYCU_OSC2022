#include "__cpio.h"
#include "utils.h"
#include "uart.h"

unsigned long align(unsigned long addr, unsigned long align_size) {
    unsigned long tmp = addr % align_size;
    if(tmp == 0) return addr;
    else return addr + (align_size - tmp);
}

void cpio_ls(unsigned long addr) {
    struct cpio_newc_header* header = (struct cpio_newc_header*)(addr == 0 ? CPIO_ADDR : addr);
    while(1) {
        unsigned long file_size = hex_to_int(header->c_filesize, 8);
        unsigned long name_size = hex_to_int(header->c_namesize, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        printf("%s\n", pathname);
        header = (struct cpio_newc_header*)((char *)header + align(CPIO_SIZE + name_size, 4) + align(file_size, 4));
    }
}

void cpio_cat(unsigned long addr) {
    char cmd[20];
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
		printf("\r# cat %s \r\e[%dC", cmd, idx + 6);
	}

    struct cpio_newc_header* header = (struct cpio_newc_header*)(addr == 0 ? CPIO_ADDR : addr);
    int flag = 0;
    unsigned long file_size, name_size;
    while(1) {
        file_size = hex_to_int(header->c_filesize, 8);
        name_size = hex_to_int(header->c_namesize, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        else if(!strcmp(pathname, cmd)) {
            flag = 1;
            break;
        }
        header = (struct cpio_newc_header*)((char *)header + align(CPIO_SIZE + name_size, 4) + align(file_size, 4));
    }
    if(flag) {
        char *file_content = (char *)((char *)header + align(CPIO_SIZE + name_size, 4));
        printf("\n");
        for(int i = 0; i < file_size; i++) printf("%c", *(file_content + i));
        printf("\n");
    }
    else printf("\nError: No such file found!!\n");
}

