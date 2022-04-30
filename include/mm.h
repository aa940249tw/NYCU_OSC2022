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

#define PROT_READ        0x1                /* Page can be read.  */
#define PROT_WRITE       0x2                /* Page can be written.  */
#define PROT_EXEC        0x4                /* Page can be executed.  */
#define PROT_NONE        0x0                /* Page can not be accessed.  */

#define MAP_FIXED        0x10
#define MAP_ANONYMOUS    0x20
#define MAP_POPULATE     0x08000

typedef enum {AVAL, USED, F} USAGE;

struct mm_struct {
    struct vma_area_struct *mmap;
    unsigned long pgd;
    unsigned long mmap_base;
    int mm_count;
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk, start_stack;
    struct PAGE_FRAME *used_p;
};

struct vma_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    struct vma_area_struct *vm_next;
    struct mm_struct *vm_mm;
    int vm_prot;
    int vm_flag;
};

struct PAGE_LIST {
    struct PAGE_LIST *next, *prev;
};

struct PAGE_FRAME {
    struct PAGE_LIST list;
    USAGE used;
    int order;
    int reference_cnt;  // For copy on write
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
void clear_pgd(struct mm_struct *);
void clear_vma(struct mm_struct *);
void vma_insert(struct mm_struct *, struct vma_area_struct *);
unsigned long create_vma(struct mm_struct *, unsigned long, unsigned long, int, int);
unsigned long chk_vma_valid(struct mm_struct *, unsigned long, unsigned long);
void copy_vma(struct mm_struct *, struct mm_struct *);

#endif