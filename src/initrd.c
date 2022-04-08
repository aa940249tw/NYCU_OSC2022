#include "initrd.h"
#include "utils.h"
#include "uart.h"
#include "thread.h"
#include "devicetree.h"
#include "mm.h"

struct cpio_newc_header* Header = (struct cpio_newc_header*)CPIO_ADDR;

unsigned long align(unsigned long addr, unsigned long align_size) {
    unsigned long tmp = addr % align_size;
    if(tmp == 0) return addr;
    else return addr + (align_size - tmp);
}

void cpio_init() {
    unsigned long addr = get_initramfs("linux,initrd-start");
    Header = (struct cpio_newc_header *)(addr == 0 ? CPIO_ADDR : addr);
}

void cpio_ls() {
    struct cpio_newc_header *header = Header;
    while(1) {
        unsigned long file_size = hex_to_int(header->c_filesize, 8);
        unsigned long name_size = hex_to_int(header->c_namesize, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        printf("%s\n", pathname);
        header = (struct cpio_newc_header*)((char *)header + align(CPIO_SIZE + name_size, 4) + align(file_size, 4));
    }
}

void cpio_cat(char *file) {
    struct cpio_newc_header *header = Header;
    int flag = 0;
    unsigned long file_size, name_size;
    while(1) {
        file_size = hex_to_int(header->c_filesize, 8);
        name_size = hex_to_int(header->c_namesize, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        else if(!strcmp(pathname, file)) {
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

struct exec_t *cpio_find(char *file) {
    struct cpio_newc_header *header = Header;
    int flag = 0;
    unsigned long file_size, name_size;
    while(1) {
        file_size = hex_to_int(header->c_filesize, 8);
        name_size = hex_to_int(header->c_namesize, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        else if(!strcmp(pathname, file)) {
            flag = 1;
            break;
        }
        header = (struct cpio_newc_header*)((char *)header + align(CPIO_SIZE + name_size, 4) + align(file_size, 4));
    }
    if(flag) {
        struct exec_t *ret = (struct exec_t *)simple_malloc(sizeof(struct exec_t));
        ret->head = header;
        ret->filecontext = (char *)((char *)header + align(CPIO_SIZE + name_size, 4));
        ret->len = file_size;
        return ret;
    }
    else return 0;
}

void __exec(char *filename, char **argv) {
    struct exec_t *execute = cpio_find(filename);
    void *user_program = kmalloc(execute->len);
    memcpy(user_program, execute->filecontext, execute->len);
    // TODO: Pass Args
    asm("msr spsr_el1, %0"::"r"((uint64_t)0x0));
    asm("msr elr_el1, %0"::"r"(user_program));
    asm("eret");
}

