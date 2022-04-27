#ifndef __PGTABLE_H__
#define __PGTABLE_H__

#include "utils.h"
#include "mm.h"

#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define __PA(addr)  (addr - 0xffff000000000000)
#define MASK(addr)  (addr & 0x0000fffffffff000)

#define PAGE_SHIFT  12
#define PAGE_MASK   (~((1 << PAGE_SHIFT) - 1))
#define PAGE_ALIGN(addr)    (((addr)+PAGE_SIZE-1) & PAGE_MASK)

#define CONFIG_PGTABLE_LEVELS   4
#define PTRS_PER_PTE    (1 << (PAGE_SHIFT - 3))
#define ARM64_HW_PGTABLE_LEVEL_SHIFT(n)     ((PAGE_SHIFT - 3) * (4 - (n)) + 3)

#define PGDIR_SHIFT ARM64_HW_PGTABLE_LEVEL_SHIFT(0)
#define PGDIR_SIZE  (_AC(1, UL) << PGDIR_SHIFT)
#define PGDIR_MASK  (~(PGDIR_SIZE - 1))
#define PTRS_PER_PGD PTRS_PER_PTE

#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define pgd_offset_raw(pgd, addr)	((pgd) + pgd_index(addr))
#define pgd_offset(mm, addr)	(pgd_offset_raw((mm)->pgd, (addr)))
#define pgd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#define PUD_SHIFT ARM64_HW_PGTABLE_LEVEL_SHIFT(1)
#define PUD_SIZE  (_AC(1, UL) << PUD_SHIFT)
#define PUD_MASK  (~(PUD_SIZE - 1))
#define PTRS_PER_PUD PTRS_PER_PTE

#define pud_index(addr)		(((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))
#define pud_offset_raw(pud, addr)	((pud) + pud_index(addr))
#define pud_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PUD_SIZE) & PUD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#define PMD_SHIFT ARM64_HW_PGTABLE_LEVEL_SHIFT(2)
#define PMD_SIZE  (_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK  (~(PMD_SIZE - 1))
#define PTRS_PER_PMD PTRS_PER_PTE

#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
#define pmd_offset_raw(pmd, addr)	((pmd) + pmd_index(addr))
#define pmd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset_raw(pte, addr)	((pte) + pte_index(addr))
#define pte_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PAGE_SIZE) & PAGE_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

void create_pgd_mapping(size_t *, size_t, size_t, size_t);
void alloc_init_pud(size_t *, size_t, size_t, size_t);
void alloc_init_pmd(size_t *, size_t, size_t, size_t);
void alloc_init_pte(size_t *, size_t, size_t, size_t);
void set_pte(size_t *, size_t);

#endif