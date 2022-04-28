#ifndef __MM_H__
#define __MM_H__

#include "utils.h"

extern unsigned char kernel_virt;

#define PAGE_SIZE           4096
#define MAX_BUDDY_ORDER     15  // 4KB ~ 4MB
#define PAGE_NUM            (1 << MAX_BUDDY_ORDER)
#define PAGE_INIT           ((unsigned long)&kernel_virt + 0x10000000)
#define MAX_DYNAMIC_PAGE    16
#define MEMPOOL_TYPE        12  // can't allocate under 8 bytes (3 ~ 11)
#define USER_STACK          0xffffffffe000
#define USER_TEXT           0x0
#define POSIX_SP            USER_STACK - 0x20000

typedef enum {AVAL, USED, F} USAGE;

struct mm_struct {
    struct vm_area_struct *mmap;
    unsigned long pgd;
    unsigned long mmap_base;
    int mm_count;
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk, start_stack;
};

struct vma_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    struct vm_area_struct *vm_next, *vm_prev;
    struct mm_struct *vm_mm;
};

struct PAGE_LIST {
    struct PAGE_LIST *next, *prev;
};

struct PAGE_FRAME {
    struct PAGE_LIST list;
    USAGE used;
    int order;
};

struct BUDDY_SYSTEM {
    struct PAGE_LIST head;
    int pg_free;
};

struct DYNAMIC_PAGE {
    struct PAGE_LIST freelist;
    int used;
    int free_obj;
    unsigned long page_addr;
};

struct MEM_POOL {
    int size;
    struct DYNAMIC_PAGE d_page[MAX_DYNAMIC_PAGE];
};

void mm_init();
void *alloc_pages(int);
void free_pages(void *);
void *dynamic_allocator(int);
void dynamic_free(struct DYNAMIC_PAGE *, void *);
void *kmalloc(int);
void kfree(void *);
void mem_reserve(unsigned long, int);
void buddy_test();

void init_mm(struct mm_struct *);
void mem_abort_handler(unsigned long, unsigned long);

#endif