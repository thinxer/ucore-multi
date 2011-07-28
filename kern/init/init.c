#include <assert.h>
#include <clock.h>
#include <console.h>
#include <intr.h>
#include <stdio.h>
#include <string.h>
#include <atomic.h>
#include <bitops.h>
#include <pmm.h>
#include <slab.h>
#include <vmm.h>
#include <proc.h>

/* Before calling into kern_init, we must make sure the memory is properly
 * mapped. This is because the kernel is compiled against KERNBASE but it's not
 * the address it would be loaded into the memory.
 * */
int kern_init(void) __attribute__((noreturn));

int
kern_init(void) {
    // clear bss
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    // console
    cons_init();

    cprintf("initializing\n");

    // physical memory management
    pmm_init();
    slab_init();

    // interrupts
    intr_init();

    // virtual memory management
    vmm_init();

    // process management
    proc_init();

    // timer
    clock_init();

    // enable interrupts
    intr_enable();

    // welcome message
    cprintf("\n(THU.CST) ucore\n");

    // do nothing
    cpu_idle();
}
