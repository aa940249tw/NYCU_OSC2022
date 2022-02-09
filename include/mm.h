#ifndef __MM_H__
#define __MM_H__

#include "utils.h"

#define PAGE_SIZE           4096
#define MAX_BUDDY_ORDER     8   // 4KB ~ 1MB
#define PAGE_NUM            (1 << MAX_BUDDY_ORDER)
#define PAGE_INIT           0x10000000
#define MAX_DYNAMIC_PAGE    16
#define MEMPOOL_TYPE        12  // can't allocate under 8 bytes (3 ~ 11)

typedef enum {AVAL, USED, F} USAGE;

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

#endif