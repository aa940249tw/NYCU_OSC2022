#include "initrd.h"
#include "utils.h"
#include "uart.h"
#include "thread.h"
#include "devicetree.h"
#include "mm.h"
#include "mem.h"

struct cpio_newc_header* Header = (struct cpio_newc_header*)CPIO_ADDR;
extern unsigned char kernel_virt;

unsigned long align(unsigned long addr, unsigned long align_size) {
    unsigned long tmp = addr % align_size;
    if(tmp == 0) return addr;
    else return addr + (align_size - tmp);
}

void cpio_init() {
    unsigned long addr = get_initramfs("linux,initrd-start");
    Header = (struct cpio_newc_header *)((addr == 0 ? CPIO_ADDR : addr) + (unsigned long)&kernel_virt);
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

extern void to_el0();
void __exec(char *filename, char **argv) {
    struct thread_t *cur = (struct thread_t *)get_current();
    struct exec_t *execute = cpio_find(filename);
    /*
    void *user_program = kmalloc(execute->len);
    memcpy(user_program, execute->filecontext, execute->len);

    cur->mm->start_code = (unsigned long)user_program;
    cur->mm->end_code = (unsigned long)user_program + execute->len;
    mappages((pagetable_t)cur->mm->pgd, USER_TEXT, execute->len, (uint64_t)user_program, PT_AF | PT_USER | PT_MEM | PT_RW);
    */
    if(cur->mm->mmap) clear_vma(cur->mm);
    //clear_pgd(cur->mm);
    // Map text sections
    create_vma(cur->mm, USER_TEXT, execute->len, 0, 0);   // Text vma
    int u_size = execute->len;
    for(int i = 0; i <= (execute->len / PAGE_SIZE); i++) {
        void *user_program = alloc_pages(0);
        u_size -= PAGE_SIZE;
        int m_size = (u_size > 0) ? PAGE_SIZE : (u_size + PAGE_SIZE);
        memcpy(user_program, (execute->filecontext + PAGE_SIZE * i), m_size);
        mappages((pagetable_t)cur->mm->pgd, (USER_TEXT + PAGE_SIZE * i), m_size, (uint64_t)user_program, PT_AF | PT_USER | PT_MEM | PT_RW);
    }
    // Map User stack
    create_vma(cur->mm, USER_STACK, THREAD_SIZE, 0, 0);   // Stack vma
    cur->user_stack = (uint64_t)kmalloc(THREAD_SIZE) + THREAD_SIZE;
    mappages((pagetable_t)cur->mm->pgd, USER_STACK, THREAD_SIZE, cur->user_stack - THREAD_SIZE, PT_AF | PT_USER | PT_MEM | PT_RW);
    // TODO: Pass Args
    to_el0(0, 0x0000fffffffff000, cur->mm->pgd);
}

