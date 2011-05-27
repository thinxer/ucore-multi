#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <types.h>
#include <mach/memlayout.h>

// pmm_manager is a physical memory management class. A special pmm manager -
// XXX_pmm_manager only needs to implement the methods in pmm_manager class,
// then XXX_pmm_manager can be used by ucore to manage the total physical memory
// space.
struct pmm_manager {
    // pmm_manager's name
    const char *name;

    // initialize internal description&management data structure (free block
    // list, number of free block) of XXX_pmm_manager
    void (*init)(void);

    // setup description&management data structcure according to the initial
    // free physical memory space
    void (*init_memmap)(struct Page *base, size_t n);

    // allocate >=n pages, depend on the allocation algorithm
    struct Page *(*alloc_pages)(size_t n);

    // free >=n pages with "base" addr of Page descriptor
    // structures (memlayout.h)
    void (*free_pages)(struct Page *base, size_t n);

    // return the number of free pages
    size_t (*nr_free_pages)(void);

    // check the correctness of XXX_pmm_manager
    void (*check)(void);
};

extern const struct pmm_manager *pmm_manager;

void pmm_init(void);
struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);
struct Page *pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm);

void unmap_range(pde_t *pgdir, uintptr_t start, uintptr_t end);
void exit_range(pde_t *pgdir, uintptr_t start, uintptr_t end);
int copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end, bool share);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

#include <arch/pmm.h>

// XXX used by check_mmap()
#define load_page_dir(pgdir) arch_load_page_dir(pgdir)

#endif /* !__KERN_MM_PMM_H__ */
