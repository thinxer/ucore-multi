#ifndef __ARM_MEMLAYOUT_H__
#define __ARM_MEMLAYOUT_H__

/**
 * This file contains the definitions for memory management in our OS for the
 * arm architecture.
 */

// Where the kernel will be loaded to the memory.
// Unlike X86, which would always be 0x0, PKERNBASE differs on different ARM
// boards.
#define PKERNBASE           0x20000000
// All kernel physical memory mapped at this address.
#define KERNBASE            0x20000000
// The maximum amount of physical memory used by kernel.
#define KMEMSIZE            0x03000000          /* 64MB */
#define KERNTOP             (KERNBASE + KMEMSIZE)

// # of pages in kernel stack
#define KSTACKPAGE          2
// sizeof kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)

#define USERTOP             0x24000000
#define USERBASE            0x23000000
#define USTACKPAGE          2
#define USTACKSIZE          (USTACKPAGE * PGSIZE)

#define USER_ACCESS(start, end)                     \
    (USERBASE <= (start) && (start) < (end) && (end) <= USERTOP)
#define KERN_ACCESS(start, end)                     \
    (KERNBASE <= (start) && (start) < (end) && (end) <= KERNTOP)

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

