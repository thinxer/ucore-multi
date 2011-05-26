#ifndef __ARM_MEMLAYOUT_H__
#define __ARM_MEMLAYOUT_H__

/**
 * This file contains the definitions for memory management in our OS for the
 * arm architecture.
 */

// Where the kernel will be loaded to the memory.
// Unlike X86, which would always be 0x0, PKERNBASE differs on different ARM
// boards.
#define PKERNBASE           0x30000000
// All kernel physical memory mapped at this address.
#define KERNBASE            0x30000000
// The maximum amount of physical memory used by kernel.
#define KMEMSIZE            0x04000000          /* 64MB */
#define KERNTOP             (KERNBASE + KMEMSIZE)

/**
 * Virtual page table. Entry PDX[VPT] in the PD (Page Directory) contains a
 * pointer to the page directory itself, thereby turning the PD into a page
 * table, which maps all the PTEs (Page Table Entry) containing the page
 * mappings for the entire virtual address space into that 4 Meg region starting
 * at VPT.
 */
#define VPT                 0xFAC00000

// # of pages in kernel stack
#define KSTACKPAGE          2
// sizeof kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)

#ifndef __ASSEMBLER__

#include <types.h>
#include <atomic.h>
#include <list.h>
#include <memlayout.h>

typedef uintptr_t pte_t;
typedef uintptr_t pde_t;

// some constants for bios interrupt 15h AX = 0xE820
#define E820MAX             20      // number of entries in E820MAP
#define E820_ARM            1       // address range memory
#define E820_ARR            2       // address range reserved

struct e820map {
    int nr_map;
    struct {
        uint64_t addr;
        uint64_t size;
        uint32_t type;
    } __attribute__((packed)) map[E820MAX];
};

void fill_physical_memory_map(struct e820map* memmap);

#endif /* !__ASSEMBLER__ */

#endif /* !__ARM_MEMLAYOUT_H__ */

