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

    // memory management
    pmm_init();
    slab_init();

    // interrupts
    intr_init();

    // timer
    clock_init();

    // enable interrupts
    intr_enable();

    atomic_t a;
    atomic_set(&a, 7);
    cprintf("a: %d\n", atomic_read(&a));    // a: 7

    unsigned long b=16;
    set_bit(1, &b);
    cprintf("b: %d\n", b);  // b: 18

    // welcome message
    cprintf("\n(THU.CST) ucore\n");

    // do nothing
    while(1);
}
