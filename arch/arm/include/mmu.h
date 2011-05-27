#ifndef __ARM_MMU_H__
#define __ARM_MMU_H__

// A linear address 'la' has a three-part structure as follows:
//
// +--------12--------+-----10-------+-+-------12----------+
// | Page Directory   | Page Table   | Offset within Page  |
// |      Index       |   Index      |                     |
// +------------------+--------------+---------------------+
//  \--- PDX(la) ----/ \- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
//
// The PDX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// page directory index
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0xFFF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x3FF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PGSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o) ((uintptr_t)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintptr_t)(pte) & ~0xFFF)
#define PDE_ADDR(pde)   PTE_ADDR(pde)

/* page directory and page table constants */
#define NPDEENTRY       4096                    // page directory entries per page directory
#define NPTEENTRY       1024                    // page table entries per page table

#define PGSIZE          4096                    // bytes mapped by a page
#define PGSHIFT         12                      // log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY / 4)    // bytes mapped by a page directory entry
#define PTSHIFT         20                      // log2(PTSIZE)

#define PTXSHIFT        10                      // offset of PTX in a linear address
#define PDXSHIFT        20                      // offset of PDX in a linear address

#define PDE_FINE        3

#define PDE_P           (1<<4)
// Really, this is not pte_p, but the flag for small page
#define PTE_P           (1<<1)
#define PTE_U           (1<<5 | 1<<7 | 1<<9 | 1<<11)
#define PTE_W           (1<<4 | 1<<6 | 1<<8 | 1<<10)

// read fault status register of cp15
#define read_fsr(c5)        do { \
	asm volatile ("mrc p15, 0, %0, c5, c0, 0;" :"=r" (c5)); \
    } while (0);

// read fault address register of cp15
#define read_far(c6)        do { \
	asm volatile ("mrc p15, 0, %0, c6, c0, 0;" :"=r" (c6)); \
    } while (0);

#endif /* !__ARM_MMU_H__ */
