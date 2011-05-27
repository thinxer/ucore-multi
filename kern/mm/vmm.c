#include <vmm.h>
#include <irqflags.h>
#include <slab.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <pmm.h>

/* 
  vmm design include two parts: mm_struct (mm) & vma_struct (vma)
  mm is the memory manager for the set of continuous virtual memory  
  area which have the same PDT. vma is a continuous virtual memory area.
  There a linear link list for vma & a redblack link list for vma in mm.
---------------
  mm related functions:
   golbal functions
     struct mm_struct * mm_create(void)
     void mm_destroy(struct mm_struct *mm)
     int do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr)
--------------
  vma related functions:
   global functions
     struct vma_struct * vma_create (uintptr_t vm_start, uintptr_t vm_end,...)
     void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma)
     struct vma_struct * find_vma(struct mm_struct *mm, uintptr_t addr)
   local functions
     inline void check_vma_overlap(struct vma_struct *prev, struct vma_struct *next)
     inline struct vma_struct * find_vma_rb(rb_tree *tree, uintptr_t addr) 
     inline void insert_vma_rb(rb_tree *tree, struct vma_struct *vma, ....
     inline int vma_compare(rb_node *node1, rb_node *node2)
---------------
   check correctness functions
     void check_vmm(void);
     void check_vma_struct(void);
     void check_pgfault(void);
*/

static void check_vmm(void);
static void check_vma_struct(void);
static void check_pgfault(void);
static void check_mmap(void);

// mm_create -  alloc a mm_struct & initialize it.
struct mm_struct *
mm_create(void) {
    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));
    if (mm != NULL) {
        list_init(&(mm->mmap_list));
        mm->mmap_tree = NULL;
        mm->mmap_cache = NULL;
        mm->pgdir = NULL;
        mm->map_count = 0;
    }
    return mm;
}

// vma_create - alloc a vma_struct & initialize it. (addr range: vm_start~vm_end)
struct vma_struct *
vma_create(uintptr_t vm_start, uintptr_t vm_end, uint32_t vm_flags) {
    struct vma_struct *vma = kmalloc(sizeof(struct vma_struct));
    if (vma != NULL) {
        vma->vm_start = vm_start;
        vma->vm_end = vm_end;
        vma->vm_flags = vm_flags;
    }
    return vma;
}

// vma_destroy - free vma_struct
static void
vma_destroy(struct vma_struct *vma) {
    kfree(vma);
}


// find_vma_rb - find a vma  (vma->vm_start <= addr <= vma_vm_end) in rb tree
static inline struct vma_struct *
find_vma_rb(rb_tree *tree, uintptr_t addr) {
    rb_node *node = rb_node_root(tree);
    struct vma_struct *vma = NULL, *tmp;
    while (node != NULL) {
        tmp = rbn2vma(node, rb_link);
        if (tmp->vm_end > addr) {
            vma = tmp;
            if (tmp->vm_start <= addr) {
                break;
            }
            node = rb_node_left(tree, node);
        }
        else {
            node = rb_node_right(tree, node);
        }
    }
    return vma;
}

// find_vma - find a vma  (vma->vm_start <= addr <= vma_vm_end)
struct vma_struct *
find_vma(struct mm_struct *mm, uintptr_t addr) {
    struct vma_struct *vma = NULL;
    if (mm != NULL) {
        vma = mm->mmap_cache;
        if (!(vma != NULL && vma->vm_start <= addr && vma->vm_end > addr)) {
            if (mm->mmap_tree != NULL) {
                vma = find_vma_rb(mm->mmap_tree, addr);
            }
            else {
                bool found = 0;
                list_entry_t *list = &(mm->mmap_list), *le = list;
                while ((le = list_next(le)) != list) {
                    vma = le2vma(le, list_link);
                    if (addr < vma->vm_end) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    vma = NULL;
                }
            }
        }
        if (vma != NULL) {
            mm->mmap_cache = vma;
        }
    }
    return vma;
}

// vma_compare - compare vma1->vm_start < vma2->vm_start ?
static inline int
vma_compare(rb_node *node1, rb_node *node2) {
    struct vma_struct *vma1 = rbn2vma(node1, rb_link);
    struct vma_struct *vma2 = rbn2vma(node2, rb_link);
    uintptr_t start1 = vma1->vm_start, start2 = vma2->vm_start;
    return (start1 < start2) ? -1 : (start1 > start2) ? 1 : 0;
}

// check_vma_overlap - check if vma1 overlaps vma2 ?
static inline void
check_vma_overlap(struct vma_struct *prev, struct vma_struct *next) {
    assert(prev->vm_start < prev->vm_end);
    assert(prev->vm_end <= next->vm_start);
    assert(next->vm_start < next->vm_end);
}

// insert_vma_rb - insert vma in rb tree according vma->start_addr
static inline void
insert_vma_rb(rb_tree *tree, struct vma_struct *vma, struct vma_struct **vma_prevp) {
    rb_node *node = &(vma->rb_link), *prev;
    rb_insert(tree, node);
    if (vma_prevp != NULL) {
        prev = rb_node_prev(tree, node);
        *vma_prevp = (prev != NULL) ? rbn2vma(prev, rb_link) : NULL;
    }
}

// insert_vma_struct -insert vma in mm's rb tree link & list link
void
insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma) {
    assert(vma->vm_start < vma->vm_end);
    list_entry_t *list = &(mm->mmap_list);
    list_entry_t *le_prev = list, *le_next;
    if (mm->mmap_tree != NULL) {
        struct vma_struct *mmap_prev;
        insert_vma_rb(mm->mmap_tree, vma, &mmap_prev);
        if (mmap_prev != NULL) {
            le_prev = &(mmap_prev->list_link);
        }
    }
    else {
        list_entry_t *le = list;
        while ((le = list_next(le)) != list) {
            struct vma_struct *mmap_prev = le2vma(le, list_link);
            if (mmap_prev->vm_start > vma->vm_start) {
                break;
            }
            le_prev = le;
        }
    }

    le_next = list_next(le_prev);

    /* check overlap */
    if (le_prev != list) {
        check_vma_overlap(le2vma(le_prev, list_link), vma);
    }
    if (le_next != list) {
        check_vma_overlap(vma, le2vma(le_next, list_link));
    }

    vma->vm_mm = mm;
    list_add_after(le_prev, &(vma->list_link));

    mm->map_count ++;
    if (mm->mmap_tree == NULL && mm->map_count >= RB_MIN_MAP_COUNT) {

        /* try to build red-black tree now, but may fail. */
        mm->mmap_tree = rb_tree_create(vma_compare);

        if (mm->mmap_tree != NULL) {
            list_entry_t *list = &(mm->mmap_list), *le = list;
            while ((le = list_next(le)) != list) {
                insert_vma_rb(mm->mmap_tree, le2vma(le, list_link), NULL);
            }
        }
    }
}

// remove_vma_struct - remove vma from mm's rb tree link & list link
static int
remove_vma_struct(struct mm_struct *mm, struct vma_struct *vma) {
    assert(mm == vma->vm_mm);
    if (mm->mmap_tree != NULL) {
        rb_delete(mm->mmap_tree, &(vma->rb_link));
    }
    list_del(&(vma->list_link));
    if (vma == mm->mmap_cache) {
        mm->mmap_cache = NULL;
    }
    mm->map_count --;
    return 0;
}

// mm_destroy - free mm and mm internal fields
void
mm_destroy(struct mm_struct *mm) {
    if (mm->mmap_tree != NULL) {
        rb_tree_destroy(mm->mmap_tree);
    }
    list_entry_t *list = &(mm->mmap_list), *le;
    while ((le = list_next(list)) != list) {
        list_del(le);
        vma_destroy(le2vma(le, list_link));
    }
    kfree(mm);
}

// vmm_init - initialize virtual memory management
//          - now just call check_vmm to check correctness of vmm
void
vmm_init(void) {
    check_vmm();
    check_mmap();
}

int
mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags,
        struct vma_struct **vma_store) {
    uintptr_t start = ROUNDDOWN(addr, PGSIZE), end = ROUNDUP(addr + len, PGSIZE);
    if (!USER_ACCESS(start, end)) {
        return -E_INVAL;
    }

    assert(mm != NULL);

    int ret = -E_INVAL;

    struct vma_struct *vma;
    if ((vma = find_vma(mm, start)) != NULL && end > vma->vm_start) {
        goto out;
    }
    ret = -E_NO_MEM;
    if ((vma = vma_create(start, end, vm_flags)) == NULL) {
        goto out;
    }
    insert_vma_struct(mm, vma);
    if (vma_store != NULL) {
        *vma_store = vma;
    }
    ret = 0;

out:
    return ret;
}

static void
vma_resize(struct vma_struct *vma, uintptr_t start, uintptr_t end) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(vma->vm_start <= start && start < end && end <= vma->vm_end);
    vma->vm_start = start, vma->vm_end = end;
}

int
mm_unmap(struct mm_struct *mm, uintptr_t addr, size_t len) {
    uintptr_t start = ROUNDDOWN(addr, PGSIZE), end = ROUNDUP(addr + len, PGSIZE);
    if (!USER_ACCESS(start, end)) {
        return -E_INVAL;
    }

    assert(mm != NULL);

    struct vma_struct *vma;
    if ((vma = find_vma(mm, start)) == NULL || end <= vma->vm_start) {
        return 0;
    }

    if (vma->vm_start < start && end < vma->vm_end) {
        struct vma_struct *nvma;
        if ((nvma = vma_create(vma->vm_start, start, vma->vm_flags)) == NULL) {
            return -E_NO_MEM;
        }
        vma_resize(vma, end, vma->vm_end);
        insert_vma_struct(mm, nvma);
        unmap_range(mm->pgdir, start, end);
        return 0;
    }

    list_entry_t free_list, *le;
    list_init(&free_list);
    while (vma->vm_start < end) {
        le = list_next(&(vma->list_link));
        remove_vma_struct(mm, vma);
        list_add(&free_list, &(vma->list_link));
        if (le == &(mm->mmap_list)) {
            break;
        }
        vma = le2vma(le, list_link);
    }

    le = list_next(&free_list);
    while (le != &free_list) {
        vma = le2vma(le, list_link);
        le = list_next(le);
        uintptr_t un_start, un_end;
        if (vma->vm_start < start) {
            un_start = start, un_end = vma->vm_end;
            vma_resize(vma, vma->vm_start, un_start);
            insert_vma_struct(mm, vma);
        }
        else {
            un_start = vma->vm_start, un_end = vma->vm_end;
            if (end < un_end) {
                un_end = end;
                vma_resize(vma, un_end, vma->vm_end);
                insert_vma_struct(mm, vma);
            }
            else {
                vma_destroy(vma);
            }
        }
        unmap_range(mm->pgdir, un_start, un_end);
    }
    return 0;
}

int
dup_mmap(struct mm_struct *to, struct mm_struct *from) {
    assert(to != NULL && from != NULL);
    list_entry_t *list = &(from->mmap_list), *le = list;
    while ((le = list_prev(le)) != list) {
        struct vma_struct *vma, *nvma;
        vma = le2vma(le, list_link);
        nvma = vma_create(vma->vm_start, vma->vm_end, vma->vm_flags);
        if (nvma == NULL) {
            return -E_NO_MEM;
        }
        insert_vma_struct(to, nvma);
        if (copy_range(to->pgdir, from->pgdir, vma->vm_start, vma->vm_end, 0) != 0) {
            return -E_NO_MEM;
        }
    }
    return 0;
}

void
exit_mmap(struct mm_struct *mm) {
    assert(mm != NULL);
    pde_t *pgdir = mm->pgdir;
    list_entry_t *list = &(mm->mmap_list), *le = list;
    while ((le = list_next(le)) != list) {
        struct vma_struct *vma = le2vma(le, list_link);
        unmap_range(pgdir, vma->vm_start, vma->vm_end);
    }
    while ((le = list_next(le)) != list) {
        struct vma_struct *vma = le2vma(le, list_link);
        exit_range(pgdir, vma->vm_start, vma->vm_end);
    }
}

uintptr_t
get_unmapped_area(struct mm_struct *mm, size_t len) {
    if (len == 0 || len > USERTOP) {
        return 0;
    }
    uintptr_t start = USERTOP - len;
    list_entry_t *list = &(mm->mmap_list), *le = list;
    while ((le = list_prev(le)) != list) {
        struct vma_struct *vma = le2vma(le, list_link);
        if (start >= vma->vm_end) {
            return start;
        }
        if (start + len > vma->vm_start) {
            if (len >= vma->vm_start) {
                return 0;
            }
            start = vma->vm_start - len;
        }
    }
    return (start >= USERBASE) ? start : 0;
}

bool
user_mem_check(struct mm_struct *mm, uintptr_t addr, size_t len, bool write) {
    if (mm != NULL) {
        if (!USER_ACCESS(addr, addr + len)) {
            return 0;
        }
        struct vma_struct *vma;
        uintptr_t start = addr, end = addr + len;
        while (start < end) {
            if ((vma = find_vma(mm, start)) == NULL || start < vma->vm_start) {
                return 0;
            }
            if (!(vma->vm_flags & ((write) ? VM_WRITE : VM_READ))) {
                return 0;
            }
            if (write && (vma->vm_flags & VM_STACK)) {
                if (start < vma->vm_start + PGSIZE) {
                    return 0;
                }
            }
            start = vma->vm_end;
        }
        return 1;
    }
    return KERN_ACCESS(addr, addr + len);
}


// check_vmm - check correctness of vmm
static void
check_vmm(void) {
    size_t nr_free_pages_store = nr_free_pages();
    size_t slab_allocated_store = slab_allocated();

    check_vma_struct();
    check_pgfault();

    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());

    cprintf("check_vmm() succeeded.\n");
}

static void
check_vma_struct(void) {
    size_t nr_free_pages_store = nr_free_pages();
    size_t slab_allocated_store = slab_allocated();

    struct mm_struct *mm = mm_create();
    assert(mm != NULL);

    int step1 = RB_MIN_MAP_COUNT * 2, step2 = step1 * 10;

    int i;
    for (i = step1; i >= 0; i --) {
        struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL);
        insert_vma_struct(mm, vma);
    }

    for (i = step1 + 1; i <= step2; i ++) {
        struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL);
        insert_vma_struct(mm, vma);
    }

    list_entry_t *le = list_next(&(mm->mmap_list));

    for (i = 0; i <= step2; i ++) {
        assert(le != &(mm->mmap_list));
        struct vma_struct *mmap = le2vma(le, list_link);
        assert(mmap->vm_start == i * 5 && mmap->vm_end == i * 5 + 2);
        le = list_next(le);
    }

    for (i = 0; i < 5 * step2 + 2; i ++) {
        struct vma_struct *vma = find_vma(mm, i);
        assert(vma != NULL);
        int j = i / 5;
        if (i >= 5 * j + 2) {
            j ++;
        }
        assert(vma->vm_start == j * 5 && vma->vm_end == j * 5 + 2);
    }

    mm_destroy(mm);

    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());

    cprintf("check_vma_struct() succeeded!\n");
}

struct mm_struct *check_mm_struct;

/**
 * Check the correctness of pgfault handler.
 */
static void
check_pgfault(void) {
    size_t nr_free_pages_store = nr_free_pages();
    size_t slab_allocated_store = slab_allocated();

    check_mm_struct = mm_create();
    assert(check_mm_struct != NULL);

    struct mm_struct *mm = check_mm_struct;
    pde_t *pgdir = mm->pgdir = boot_pgdir;

    // Find an empty address for testing.
    uintptr_t base_addr = 0;
    while (pgdir[base_addr])
        base_addr ++;
    cprintf("checking pgfault at addr: %08lx\n", base_addr * PTSIZE);

    struct vma_struct *vma = vma_create(base_addr * PTSIZE,
            base_addr * PTSIZE + PTSIZE, VM_WRITE);
    assert(vma != NULL);

    insert_vma_struct(mm, vma);

    uintptr_t addr = base_addr * PTSIZE + 0x100;
    assert(find_vma(mm, addr) == vma);

    int i, sum = 0;
    for (i = 0; i < 100; i ++) {
        *(char *)(addr + i) = i;
        sum += i;
    }
    for (i = 0; i < 100; i ++) {
        sum -= *(char *)(addr + i);
    }
    assert(sum == 0);

    page_remove(pgdir, ROUNDDOWN(addr, PGSIZE));
    free_page(pa2page(pgdir[0]));
    pgdir[base_addr] = 0;

    mm->pgdir = NULL;
    mm_destroy(mm);
    check_mm_struct = NULL;

    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());

    cprintf("check_pgfault() succeeded!\n");
}

static void
check_mmap(void) {
    size_t nr_free_pages_store = nr_free_pages();
    size_t slab_allocated_store = slab_allocated();

    int ret, i, j;
    /*
    for (i = 0; i < max_swap_offset; i ++) {
        assert(mem_map[i] == SWAP_UNUSED);
    }
    */
    extern struct mm_struct *check_mm_struct;
    assert(check_mm_struct == NULL);

    // step1: check mm_map

    struct mm_struct *mm0 = mm_create(), *mm1;
    assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

    uintptr_t addr0, addr1;

    addr0 = 0;
    do {
        ret = mm_map(mm0, addr0, PTSIZE, 0, NULL);
        //cprintf("mmap@%08lx, ret=%d\n", addr0, ret);
        //cprintf("d@0x10: %08lx\n", *(unsigned int*)0x10);
        assert(ret == (USER_ACCESS(addr0, addr0 + PTSIZE)) ? 0 : -E_INVAL);
        addr0 += PTSIZE;
        /*
        if (addr0 > 0x7800000) {
            cprintf("===");
            *((int*)0xffffffff) = 1;
        }
        */
    } while (addr0 != 0);


    addr0 = 0;
    for (i = 0; i < 1024; i ++, addr0 += PTSIZE) {
        assert(ret == -E_INVAL);
    }

    mm_destroy(mm0);


    mm0 = mm_create();
    assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

    addr0 = 0, i = 0;
    do {
        ret = mm_map(mm0, addr0, PTSIZE - PGSIZE, 0, NULL);
        cprintf("mmap@%08lx, ret=%d\n", addr0, ret);
        assert(ret == (USER_ACCESS(addr0, addr0 + PTSIZE)) ? 0 : -E_INVAL);
        if (ret == 0) {
            i ++;
        }
        addr0 += PTSIZE;
    } while (addr0 != 0);

    addr0 = 0, j = 0;
    do {
        addr0 += PTSIZE - PGSIZE;
        ret = mm_map(mm0, addr0, PGSIZE, 0, NULL);
        assert(ret == (USER_ACCESS(addr0, addr0 + PGSIZE)) ? 0 : -E_INVAL);
        if (ret == 0) {
            j ++;
        }
        addr0 += PGSIZE;
    } while (addr0 != 0);

    assert(j + 1 >= i);

    mm_destroy(mm0);

    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());

    cprintf("check_mm: step1, mm_map ok.\n");

    // step2: check page fault

    mm0 = mm_create();
    assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

    // setup page table

    cprintf("pages: %d\n", PGDIRSIZE / PGSIZE);
    struct Page *page = alloc_pages(PGDIRSIZE / PGSIZE);
    assert(page != NULL);
    pde_t *pgdir = page2kva(page);
    cprintf("boot_pgdir@%08lx\n", (uintptr_t) boot_pgdir);
    cprintf("pgdir@%08lx\n", (uintptr_t) pgdir);
    cprintf("pgdir(p)@%08lx\n", (uintptr_t) PADDR(pgdir));
    memcpy(pgdir, boot_pgdir, PGDIRSIZE);
    pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;

    // prepare for page fault

    mm0->pgdir = pgdir;
    check_mm_struct = mm0;
    cprintf("1");
//    *((int*)10000) = 1;
    cprintf("1.1");
    load_page_dir(PADDR(mm0->pgdir));
 //   *((int*)10000) = 1;
    cprintf("2");

    uint32_t vm_flags = VM_WRITE | VM_READ;
    struct vma_struct *vma;

    addr0 = 0;
    do {
        if ((ret = mm_map(mm0, addr0, PTSIZE, vm_flags, &vma)) == 0) {
            break;
        }
        addr0 += PTSIZE;
    } while (addr0 != 0);

    cprintf("addr0: %08lx\n", addr0);

    cprintf("3");

    assert(ret == 0 && addr0 != 0 && mm0->map_count == 1);
    assert(vma->vm_start == addr0 && vma->vm_end == addr0 + PTSIZE);

    // check pte entry

    pte_t *ptep;
    for (addr1 = addr0; addr1 < addr0 + PTSIZE; addr1 += PGSIZE) {
        ptep = get_pte(pgdir, addr1, 0);
        assert(ptep == NULL);
    }

    cprintf("4");
    cprintf("4.0.1");

    memset((void *)addr0, 0xEF, PGSIZE * 2);
    cprintf("4.1");
    ptep = get_pte(pgdir, addr0, 0);
    cprintf("4.2");
    assert(ptep != NULL && (*ptep & PTE_P));
    ptep = get_pte(pgdir, addr0 + PGSIZE, 0);
    cprintf("4.3");
    assert(ptep != NULL && (*ptep & PTE_P));

    cprintf("5");

    ret = mm_unmap(mm0, - PTSIZE, PTSIZE);
    assert(ret == -E_INVAL);
    ret = mm_unmap(mm0, addr0 + PTSIZE, PGSIZE);
    assert(ret == 0);

    addr1 = addr0 + PTSIZE / 2;
    ret = mm_unmap(mm0, addr1, PGSIZE);
    assert(ret == 0 && mm0->map_count == 2);

    ret = mm_unmap(mm0, addr1 + 2 * PGSIZE, PGSIZE * 4);
    assert(ret == 0 && mm0->map_count == 3);

    ret = mm_map(mm0, addr1, PGSIZE * 6, 0, NULL);
    assert(ret == -E_INVAL);
    ret = mm_map(mm0, addr1, PGSIZE, 0, NULL);
    assert(ret == 0 && mm0->map_count == 4);
    ret = mm_map(mm0, addr1 + 2 * PGSIZE, PGSIZE * 4, 0, NULL);
    assert(ret == 0 && mm0->map_count == 5);

    ret = mm_unmap(mm0, addr1 + PGSIZE / 2, PTSIZE / 2 - PGSIZE);
    assert(ret == 0 && mm0->map_count == 1);

    addr1 = addr0 + PGSIZE;
    for (i = 0; i < PGSIZE; i ++) {
        assert(*(char *)(addr1 + i) == (char)0xEF);
    }

    cprintf("6");

    ret = mm_unmap(mm0, addr1 + PGSIZE / 2, PGSIZE / 4);
    assert(ret == 0 && mm0->map_count == 2);
    ptep = get_pte(pgdir, addr0, 0);
    assert(ptep != NULL && (*ptep & PTE_P));
    ptep = get_pte(pgdir, addr0 + PGSIZE, 0);
    assert(ptep != NULL && *ptep == 0);

    ret = mm_map(mm0, addr1, PGSIZE, vm_flags, NULL);
    memset((void *)addr1, 0x88, PGSIZE);
    assert(*(char *)addr1 == (char)0x88 && mm0->map_count == 3);

    for (i = 1; i < 16; i += 2) {
        ret = mm_unmap(mm0, addr0 + PGSIZE * i, PGSIZE);
        assert(ret == 0);
        if (i < 8) {
            ret = mm_map(mm0, addr0 + PGSIZE * i, PGSIZE, 0, NULL);
            assert(ret == 0);
        }
    }
    assert(mm0->map_count == 13);

    ret = mm_unmap(mm0, addr0 + PGSIZE * 2, PTSIZE - PGSIZE * 2);
    assert(ret == 0 && mm0->map_count == 2);

    ret = mm_unmap(mm0, addr0, PGSIZE * 2);
    assert(ret == 0 && mm0->map_count == 0);

    cprintf("check_mmap: step2, mm_unmap ok.\n");

    // step3: check exit_mmap

    ret = mm_map(mm0, addr0, PTSIZE, vm_flags, NULL);
    assert(ret == 0);

    for (i = 0, addr1 = addr0; i < 4; i ++, addr1 += PGSIZE) {
        *(char *)addr1 = (char)0xFF;
    }

    exit_mmap(mm0);
    for (i = 0; i < PDX(KERNBASE); i ++) {
        assert(pgdir[i] == 0);
    }

    cprintf("check_mmmap: step3, exit_mmap ok.\n");

    // step4: check dup_mmap
    /*
    for (i = 0; i < max_swap_offset; i ++) {
        assert(mem_map[i] == SWAP_UNUSED);
    }
    */

    ret = mm_map(mm0, addr0, PTSIZE, vm_flags, NULL);
    assert(ret != 0);

    addr1 = addr0;
    for (i = 0; i < 4; i ++, addr1 += PGSIZE) {
        *(char *)addr1 = (char)(i * i);
    }

    /*
    ret = 0;
    ret += swap_out_mm(mm0, 10);
    ret += swap_out_mm(mm0, 10);
    assert(ret == 4);
    */

    for (; i < 8; i ++, addr1 += PGSIZE) {
        *(char *)addr1 = (char)(i * i);
    }

    // setup mm1

    mm1 = mm_create();
    assert(mm1 != NULL);

    page = alloc_page();
    assert(page != NULL);
    pgdir = page2kva(page);
    memcpy(pgdir, boot_pgdir, PGSIZE);
    pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
    mm1->pgdir = pgdir;

    ret = dup_mmap(mm1, mm0);
    assert(ret == 0);

    // switch to mm1

    check_mm_struct = mm1;
    load_page_dir(PADDR(mm1->pgdir));

    addr1 = addr0;
    for (i = 0; i < 8; i ++, addr1 += PGSIZE) {
        assert(*(char *)addr1 == (char)(i * i));
        *(char *)addr1 = (char)0x88;
    }

    // switch to mm0

    check_mm_struct = mm0;
    load_page_dir(PADDR(mm0->pgdir));

    addr1 = addr0;
    for (i = 0; i < 8; i ++, addr1 += PGSIZE) {
        assert(*(char *)addr1 == (char)(i * i));
    }

    // switch to boot_pgdir

    check_mm_struct = NULL;
    load_page_dir(PADDR(boot_pgdir));

    // free memory

    exit_mmap(mm0);
    exit_mmap(mm1);

    free_page(kva2page(mm0->pgdir));
    mm_destroy(mm0);
    free_page(kva2page(mm1->pgdir));
    mm_destroy(mm1);

    cprintf("check_mmap: step4, dup_mmap ok.\n");

    /*
    refill_inactive_scan();
    page_launder();
    for (i = 0; i < max_swap_offset; i ++) {
        assert(mem_map[i] == SWAP_UNUSED);
    }
    */

    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());

    cprintf("check_mmap succeeded.\n");
}

// interrupt handler to process the page fault exception
int
do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr) {
    int ret = -E_INVAL;
    struct vma_struct *vma = find_vma(mm, addr);
    if (vma == NULL || vma->vm_start > addr) {
        goto failed;
    }

    switch (error_code & 3) {
    default:
            /* default is 3: write, present */
    case 2: /* write, not present */
        if (!(vma->vm_flags & VM_WRITE)) {
            goto failed;
        }
        break;
    case 1: /* read, present */
        goto failed;
    case 0: /* read, not present */
        if (!(vma->vm_flags & (VM_READ | VM_EXEC))) {
            goto failed;
        }
    }

    uint32_t perm = PTE_U;
    if (vma->vm_flags & VM_WRITE) {
        perm |= PTE_W;
    }
    addr = ROUNDDOWN(addr, PGSIZE);

    ret = -E_NO_MEM;

    if (pgdir_alloc_page(mm->pgdir, addr, perm) == 0) {
        goto failed;
    }
    ret = 0;

failed:
    return ret;
}

