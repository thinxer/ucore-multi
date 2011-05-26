#include <mach/memlayout.h>

void
fill_physical_memory_map(struct e820map* memmap) {
    memmap->nr_map = 2;
    memmap->map[0].addr = 0x00000000;
    memmap->map[0].size = 0x30000000;
    memmap->map[0].type = E820_ARR;
    memmap->map[1].addr = 0x30000000;
    memmap->map[1].size = 0x01800000;
    memmap->map[1].type = E820_ARM;
}
