#include <types.h>
#include <stdio.h>
#include <string.h>
#include <arch/mmu.h>
#include <mach/memlayout.h>
#include <arch/pmm.h>
#include <buddy_pmm.h>
#include <irqflags.h>
#include <error.h>

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;
// physical address of boot-time page directory
uintptr_t boot_pgdir_p;

// physical memory management
const struct pmm_manager *pmm_manager;

/* *
 * The page directory entry corresponding to the virtual address range
 * [VPT, VPT + PTSIZE) points to the page directory itself. Thus, the page
 * directory is treated as a page table as well as a page directory.
 *
 * One result of treating the page directory as a page table is that all PTEs
 * can be accessed though a "virtual page table" at virtual address VPT. And the
 * PTE for number n is stored in vpt[n].
 *
 * A second consequence is that the contents of the current page directory will
 * always available at virtual address PGADDR(PDX(VPT), PDX(VPT), 0), to which
 * vpd is set bellow.
 * */
//pte_t * const vpt = (pte_t *)VPT;
//pde_t * const vpd = (pde_t *)PGADDR(PDX(VPT), PDX(VPT), 0);

static void check_alloc_page(void);
static void check_pgdir(void);

//init_pmm_manager - initialize a pmm_manager instance
static void
init_pmm_manager(void) {
    pmm_manager = &buddy_pmm_manager;
    cprintf("memory management: %s\n", pmm_manager->name);
    pmm_manager->init();
}

// call pmm->init_memmap to build Page struct for free memory
static void
init_memmap(struct Page *base, size_t n) {
    pmm_manager->init_memmap(base, n);
}

// call pmm->alloc_pages to allocate a continuous n*PAGESIZE memory
struct Page *
alloc_pages(size_t n) {
    struct Page *page;
    unsigned long intr_flag;
    local_irq_save(intr_flag);
    {
        page = pmm_manager->alloc_pages(n);
    }
    local_irq_restore(intr_flag);
    return page;
}

// call pmm->free_pages to free a continuous n*PAGESIZE memory
void
free_pages(struct Page *base, size_t n) {
    unsigned long intr_flag;
    local_irq_save(intr_flag);
    {
        pmm_manager->free_pages(base, n);
    }
    local_irq_restore(intr_flag);
}

// call pmm->nr_free_pages to get the size (nr*PAGESIZE) of current free memory
size_t
nr_free_pages(void) {
    size_t ret;
    unsigned long intr_flag;
    local_irq_save(intr_flag);
    {
        ret = pmm_manager->nr_free_pages();
    }
    local_irq_restore(intr_flag);
    return ret;
}

// initialize physical pages.
static void
page_init(void) {
    struct e820map *memmap = (struct e820map *)(0x8000 + KERNBASE);
    uint64_t maxpa = 0;

    fill_physical_memory_map(memmap);

    cprintf("e820map:\n");
    int i;
    for (i = 0; i < memmap->nr_map; i ++) {
        uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
        cprintf("  memory: %08llx, [%08llx, %08llx], type = %d.\n",
                memmap->map[i].size, begin, end - 1, memmap->map[i].type);
        if (memmap->map[i].type == E820_ARM) {
            if (maxpa < end && begin < PKERNBASE + KMEMSIZE) {
                maxpa = end;
            }
        }
    }

    if (maxpa > PKERNBASE + KMEMSIZE) {
        maxpa = PKERNBASE + KMEMSIZE;
    }
    cprintf("maxpa: %08llx\n", maxpa);

    extern char end[];

    npage = maxpa / PGSIZE;
    cprintf("npage: %d\n", npage);
    pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);

    for (i = 0; i < npage; i ++) {
        SetPageReserved(pages + i);
    }

    uintptr_t freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage);
    // Round up to 4 pages so that buddy pmm will align (ARM only).
    freemem = ROUNDUP(freemem, 4 * PGSIZE);
    cprintf("freemem: %08lx\n", freemem);

    for (i = 0; i < memmap->nr_map; i ++) {
        uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
        if (memmap->map[i].type == E820_ARM) {
            if (begin < freemem) {
                begin = freemem;
            }
            if (end > PKERNBASE + KMEMSIZE) {
                end = PKERNBASE + KMEMSIZE;
            }
            if (begin < end) {
                begin = ROUNDUP(begin, PGSIZE);
                end = ROUNDDOWN(end, PGSIZE);
                if (begin < end) {
                    init_memmap(pa2page(begin), (end - begin) / PGSIZE);
                }
            }
        }
    }
}

void
enable_paging() {
    arch_load_page_dir(boot_pgdir_p);
}

// setup & enable the paging mechanism
// parameters
//  la:   linear address of this memory need to map (after x86 segment map)
//  size: memory size
//  pa:   physical address of this memory
//  perm: permission of this memory
static void
boot_map_segment(pde_t *pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
    assert(PGOFF(la) == PGOFF(pa));
    la = ROUNDDOWN(la, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);
    for (; size > 0; size -= PGSIZE, la += PGSIZE, pa += PGSIZE) {
        // cprintf("mapping: la@%08lx to pa@%08lx\n", la, pa);
        pte_t *ptep = get_pte(pgdir, la, 1);
        assert(ptep != NULL);
        *ptep = pa | PTE_P | perm;
    }
}

/**
 * allocate pages for boot pgdir using pmm->alloc_pages
 *
 * note: this function is used to get the memory for PDT (Page Directory Table)
 *       & PT (Page Table)
 *
 * @return the kernel virtual address of this allocated page
 */
static void *
boot_alloc_page(void) {
    struct Page *p;
    p = alloc_pages(PGDIRSIZE / PGSIZE);
    if (p == NULL)
        panic("boot_alloc_page failed.\n");
    return page2kva(p);
}

// setup a pmm to manage physical memory, build PDT&PT to setup paging
// mechanism check the correctness of pmm & paging mechanism, print
// PDT&PT
void
pmm_init(void) {
    // We need to alloc/free the physical memory (granularity is 4KB or other
    // size).  So a framework of physical memory manager (struct pmm_manager)is
    // defined in pmm.h First we should init a physical memory manager (pmm)
    // based on the framework.  Then pmm can alloc/free the physical memory.
    // Now the first_fit/best_fit/worst_fit/buddy_system pmm are available.
    init_pmm_manager();

    // detect physical memory space, reserve already used memory, then use
    // pmm->init_memmap to create free page list
    page_init();

    // use pmm->check to verify the correctness of the alloc/free function in a pmm
    check_alloc_page();

    panic("over!");

    // create boot_pgdir, an initial page directory(Page Directory Table, PDT)
    boot_pgdir = boot_alloc_page();
    assert(((uintptr_t)boot_pgdir & 0x3fff) == 0);
    memset(boot_pgdir, 0, PGDIRSIZE);
    cprintf("pgdir is at: %08lx\n", boot_pgdir);
    boot_pgdir_p = PADDR(boot_pgdir);
    cprintf("pgdir is at (p): %08lx\n", boot_pgdir_p);

    check_pgdir();

    static_assert(KERNBASE % PTSIZE == 0 && KERNTOP % PTSIZE == 0);

    // recursively insert boot_pgdir in itself to form a virtual page table at
    // virtual address VPT
    //boot_map_segment(boot_pgdir, VPT, PGDIRSIZE, boot_pgdir_p, PTE_W);
    // boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;

    cprintf("mapping initial pages... ");

    // Kernel
    boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, PKERNBASE, PTE_W);
    // IO
    boot_map_segment(boot_pgdir, 0x48000000, 0x10000000, 0x48000000, PTE_W);
    // map intr section to 0x0
    extern int  __intr_vector_start[];
    boot_map_segment(boot_pgdir, 0x0, PGSIZE, (uintptr_t) PADDR(__intr_vector_start), PTE_W);

    cprintf("done!\n");

    cprintf("enabling paging... ");
    enable_paging();

    cprintf("success!\n");

    // print_pgdir();
}

/* Get pte and return the kernel virtual address of this pte for la if the PT
 * contians this pte didn't exist, alloc a page for PT.
 *
 * @param pgdir     the kernel virtual base address of PDT
 * @param la        the linear address need to map
 * @param create    a logical value to decide if alloc a page for PT
 * @return          the kernel virtual address of this pte
 */
pte_t *
get_pte(pde_t *pgdir, uintptr_t la, bool create) {
    pde_t *pdep = &pgdir[PDX(la)];
    if (!(*pdep & PDE_P)) {
        struct Page *page;
        if (!create || (page = alloc_page()) == NULL) {
            return NULL;
        }
        set_page_ref(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
        *pdep = pa | PDE_COARSE | PDE_P;
    }
    return &((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)];
}

//get_page - get related Page struct for linear address la using PDT pgdir
struct Page *
get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store) {
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep_store != NULL) {
        *ptep_store = ptep;
    }
    if (ptep != NULL && *ptep & PTE_P) {
        return pa2page(*ptep);
    }
    return NULL;
}

//page_remove_pte - free an Page sturct which is related linear address la
//                - and clean(invalidate) pte which is related linear address la
//note: PT is changed, so the TLB need to be invalidate
static inline void
page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep) {
    if (*ptep & PTE_P) {
        struct Page *page = pte2page(*ptep);
        if (page_ref_dec(page) == 0) {
            free_page(page);
        }
        *ptep = 0;
        tlb_invalidate(pgdir, la);
    }
}

//page_remove - free an Page which is related linear address la and has an validated pte
void
page_remove(pde_t *pgdir, uintptr_t la) {
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL) {
        page_remove_pte(pgdir, la, ptep);
    }
}

/* build the map of phy addr of an Page with the linear addr la
 * note: PT is changed, so the TLB need to be invalidate
 *
 * @param pgdir the kernel virtual base address of PDT
 * @param page  the Page which need to map
 * @param la    the linear address need to map
 * @param perm  the permission of this Page which is setted in related pte
 * @return always 0
 */
int
page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm) {
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL) {
        return -E_NO_MEM;
    }
    page_ref_inc(page);
    if (*ptep & PTE_P) {
        struct Page *p = pte2page(*ptep);
        if (p == page) {
            page_ref_dec(page);
        }
        else {
            page_remove_pte(pgdir, la, ptep);
        }
    }
    *ptep = page2pa(page) | PTE_P | perm;
    tlb_invalidate(pgdir, la);
    return 0;
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
// TODO invalidate only la entry.
void
tlb_invalidate(pde_t *pgdir, uintptr_t la) {
    asm volatile (
    "mcr p15,0,%0,c8,c7,0;"
    ::"r" (0)
    );
}

// call alloc_page & page_insert functions to allocate a page size memory &
// setup an addr map pa<->la with linear address la and the PDT pgdir
struct Page *
pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm) {
    struct Page *page = alloc_page();
    if (page != NULL) {
        if (page_insert(pgdir, page, la, perm) != 0) {
            free_page(page);
            return NULL;
        }
    }
    return page;
}

void
unmap_range(pde_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do {
        pte_t *ptep = get_pte(pgdir, start, 0);
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue ;
        }
        if (*ptep != 0) {
            page_remove_pte(pgdir, start, ptep);
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
}

void
exit_range(pde_t *pgdir, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    start = ROUNDDOWN(start, PTSIZE);
    do {
        int pde_idx = PDX(start);
        if (pgdir[pde_idx] & PDE_P) {
            free_page(pde2page(pgdir[pde_idx]));
            pgdir[pde_idx] = 0;
        }
        start += PTSIZE;
    } while (start != 0 && start < end);
}

int
copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end, bool share) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do {
        pte_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue ;
        }
        if (*ptep != 0) {
            if ((nptep = get_pte(to, start, 1)) == NULL) {
                return -E_NO_MEM;
            }
            int ret;
            struct Page *page, *newpage = alloc_page();
            assert(*ptep != 0 && *nptep == 0);
            if (*ptep & PTE_P) {
                uint32_t perm = (*ptep & PTE_USER);
                if (!share) {
                    if ((page = newpage) == NULL) {
                        return -E_NO_MEM;
                    }
                    newpage = NULL;
                    memcpy(page2kva(page), page2kva(pte2page(*ptep)), PGSIZE);
                } else {
                    page = pte2page(*ptep);
                }
                ret = page_insert(to, page, start, perm);
                assert(ret == 0);
            } else {
                // XXX swap
                panic("need swap");
            }
            if (newpage != NULL) {
                free_page(newpage);
            }
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}


static void
check_alloc_page(void) {
    pmm_manager->check();
    cprintf("check_alloc_page() succeeded!\n");
}

static void
check_pgdir(void) {
    assert(npage <= (PKERNBASE + KMEMSIZE) / PGSIZE);
    assert(boot_pgdir != NULL && (uint32_t)PGOFF(boot_pgdir) == 0);
    assert(get_page(boot_pgdir, 0x0, NULL) == NULL);

    struct Page *p1, *p2;
    p1 = alloc_page();
    assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);

    pte_t *ptep;
    assert((ptep = get_pte(boot_pgdir, 0x0, 0)) != NULL);
    assert(pa2page(*ptep) == p1);
    assert(page_ref(p1) == 1);

    ptep = &((pte_t *)KADDR(PDE_ADDR(boot_pgdir[0])))[1];
    assert(get_pte(boot_pgdir, PGSIZE, 0) == ptep);

    p2 = alloc_page();
    assert(page_insert(boot_pgdir, p2, PGSIZE, PTE_U | PTE_W) == 0);
    assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
    assert(*ptep & PTE_U);
    assert(*ptep & PTE_W);
    // XXX what's the next line for?
    // assert(boot_pgdir[0] & PTE_U);
    assert(page_ref(p2) == 1);

    assert(page_insert(boot_pgdir, p1, PGSIZE, 0) == 0);
    assert(page_ref(p1) == 2);
    assert(page_ref(p2) == 0);
    assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
    assert(pa2page(*ptep) == p1);
    assert((*ptep & PTE_U) == 0);

    page_remove(boot_pgdir, 0x0);
    assert(page_ref(p1) == 1);
    assert(page_ref(p2) == 0);

    page_remove(boot_pgdir, PGSIZE);
    assert(page_ref(p1) == 0);
    assert(page_ref(p2) == 0);

    assert(page_ref(pa2page(boot_pgdir[0])) == 1);
    free_page(pa2page(boot_pgdir[0]));
    boot_pgdir[0] = 0;

    cprintf("check_pgdir() succeeded!\n");
}

/*
//perm2str - use string 'u,r,w,-' to present the permission
static const char *
perm2str(int perm) {
    static char str[4];
    str[0] = (perm & PTE_U) ? 'u' : '-';
    str[1] = 'r';
    str[2] = (perm & PTE_W) ? 'w' : '-';
    str[3] = '\0';
    return str;
}

//get_pgtable_items - In [left, right] range of PDT or PT, find a continuous linear addr space
//                  - (left_store*X_SIZE~right_store*X_SIZE) for PDT or PT
//                  - X_SIZE=PTSIZE=4M, if PDT; X_SIZE=PGSIZE=4K, if PT
// paramemters:
//  left:        no use ???
//  right:       the high side of table's range
//  start:       the low side of table's range
//  table:       the beginning addr of table
//  left_store:  the pointer of the high side of table's next range
//  right_store: the pointer of the low side of table's next range
// return value: 0 - not a invalid item range, perm - a valid item range with perm permission 
static int
get_pgtable_items(size_t left, size_t right, size_t start, uintptr_t *table, size_t *left_store, size_t *right_store) {
    if (start >= right) {
        return 0;
    }
    while (start < right && !(table[start] & PTE_P)) {
        start ++;
    }
    if (start < right) {
        if (left_store != NULL) {
            *left_store = start;
        }
        int perm = (table[start ++] & PTE_USER);
        while (start < right && (table[start] & PTE_USER) == perm) {
            start ++;
        }
        if (right_store != NULL) {
            *right_store = start;
        }
        return perm;
    }
    return 0;
}

//print_pgdir - print the PDT&PT
void
print_pgdir(void) {
    cprintf("-------------------- BEGIN --------------------\n");
    size_t left, right = 0, perm;
    while ((perm = get_pgtable_items(0, NPDEENTRY, right, vpd, &left, &right)) != 0) {
        cprintf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
                left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE, perm2str(perm));
        size_t l, r = left * NPTEENTRY;
        while ((perm = get_pgtable_items(left * NPTEENTRY, right * NPTEENTRY, r, vpt, &l, &r)) != 0) {
            cprintf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
                    l * PGSIZE, r * PGSIZE, (r - l) * PGSIZE, perm2str(perm));
        }
    }
    cprintf("--------------------- END ---------------------\n");
}
*/
