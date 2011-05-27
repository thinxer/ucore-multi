#ifndef __KERN_MM_MEMLAYOUT_H__
#define __KERN_MM_MEMLAYOUT_H__

#include <types.h>
#include <atomic.h>
#include <bitops.h>
#include <list.h>

/**
 * struct Page - Page descriptor structures. Each Page describes one physical
 * page. In kern/mm/pmm.h, you can find lots of useful functions that convert
 * Page to other data types, such as phyical address.
 */
struct Page {
    // Page frame's reference counter.
    atomic_t ref;

    // Array of flags that describe the status of the page frame.
    unsigned long flags;

    // Used in buddy system, stores the order (the X in 2^X) of the continuous
    // memory block.
    unsigned int property;

    // Used in buddy system, the No. of zone which the page belongs to.
    int zone_num;

    // Free list link.
    list_entry_t page_link;
};

// Convert list entry to page.
#define le2page(le, member)         to_struct((le), struct Page, member)


/* Flags describing the status of a page frame */

// the page descriptor is reserved for kernel or unusable
#define PG_reserved                 0
// the member 'property' is valid
#define PG_property                 1
// page frame is included in a slab
#define PG_slab                     2
// the page has been modified
#define PG_dirty                    3
// the page is in the active or inactive page list (and swap hash table)
#define PG_swap                     4
// the page is in the active page list
#define PG_active                   5

/* Helper methods. */
#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))
#define SetPageSlab(page)           set_bit(PG_slab, &((page)->flags))
#define ClearPageSlab(page)         clear_bit(PG_slab, &((page)->flags))
#define PageSlab(page)              test_bit(PG_slab, &((page)->flags))
#define SetPageDirty(page)          set_bit(PG_dirty, &((page)->flags))
#define ClearPageDirty(page)        clear_bit(PG_dirty, &((page)->flags))
#define PageDirty(page)             test_bit(PG_dirty, &((page)->flags))
#define SetPageSwap(page)           set_bit(PG_swap, &((page)->flags))
#define ClearPageSwap(page)         clear_bit(PG_swap, &((page)->flags))
#define PageSwap(page)              test_bit(PG_swap, &((page)->flags))
#define SetPageActive(page)         set_bit(PG_active, &((page)->flags))
#define ClearPageActive(page)       clear_bit(PG_active, &((page)->flags))
#define PageActive(page)            test_bit(PG_active, &((page)->flags))

static inline int
page_ref(struct Page *page) {
    return atomic_read(&(page->ref));
}

static inline void
set_page_ref(struct Page *page, int val) {
    atomic_set(&(page->ref), val);
}

static inline int
page_ref_inc(struct Page *page) {
    return atomic_add_return(&(page->ref), 1);
}

static inline int
page_ref_dec(struct Page *page) {
    return atomic_sub_return(&(page->ref), 1);
}

// free_area_t - maintains a doubly linked list to record free (unused) pages
typedef struct {
    // the list header
    list_entry_t free_list;

    // # of free pages in this free list
    unsigned int nr_free;
} free_area_t;

#endif /* !__KERN_MM_MEMLAYOUT_H__ */
