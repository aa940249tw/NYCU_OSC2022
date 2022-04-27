#include "pgtable.h"
#include "utils.h"
#include "mm.h"

void create_pgd_mapping(size_t *pgdir, size_t phys, size_t virt, size_t size) {
    unsigned long addr, end, next;
    size_t *pgdp = pgd_offset_raw(pgdir, virt);
    phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do {
		next = pgd_addr_end(addr, end);
		alloc_init_pud(pgdp, addr, next, phys);
		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

void alloc_init_pud(size_t *pgdp, size_t addr, size_t end, size_t phys) {
    unsigned long next;
    size_t *pudp;
    unsigned long pgd = *pgdp;
    unsigned long pud_phys;

    if(!pgd) {
        pud_phys = (unsigned long)kmalloc(4096);
        *pgdp |= (MASK(pud_phys) | 0b10000000011);
        pgd = *pgdp;
    }

    pudp = (size_t *)pud_offset_raw(pud_phys, addr);
    do {
		next = pud_addr_end(addr, end);
		alloc_init_pmd(pudp, addr, next, phys);
		phys += next - addr;
	} while (pudp++, addr = next, addr != end);
}

void alloc_init_pmd(size_t *pudp, size_t addr, size_t end, size_t phys) {
    unsigned long next;
    size_t *pmdp;
    unsigned long pud = *pudp;
    unsigned long pmd_phys;

    if(!pud) {
        pmd_phys = (unsigned long)kmalloc(4096);
        *pudp |= (MASK(pmd_phys) | 0b10000000011);
        pud = *pudp;
    }

    pmdp = (size_t *)pmd_offset_raw(pmd_phys, addr);
    do {
		next = pmd_addr_end(addr, end);
		alloc_init_pte(pmdp, addr, next, phys);
		phys += next - addr;
	} while (pmdp++, addr = next, addr != end);
}

void alloc_init_pte(size_t *pmdp, size_t addr, size_t end, size_t phys) {
    unsigned long next;
    size_t *ptep;
    unsigned long pmd = *pmdp;
    unsigned long pte_phys;

    if(!pmd) {
        pte_phys = (unsigned long)kmalloc(4096);
        *pmdp |= (MASK(pte_phys) | 0b10000000011);
        pmd = *pmdp;
    }

    ptep = (size_t *)pte_offset_raw(pte_phys, addr);
    do {
		next = pte_addr_end(addr, end);
		set_pte(ptep, phys);
		phys += next - addr;
	} while (ptep++, addr = next, addr != end);
}

void set_pte(size_t *ptep, size_t phys) {
    *ptep |= (MASK(phys) | 0b10000000011);
}