#include <assert.h>
#include <clock.h>
#include <console.h>
#include <intr.h>
#include <stdio.h>
#include <string.h>
#include <atomic.h>
#include <bitops.h>

int kern_init(void) __attribute__((noreturn));

int
kern_init(void) {
    // clear bss
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    // init console
    cons_init();

    cprintf("initializing\n");

    // init interrupts
    intr_init();

    // init timer
    clock_init();

    // enable interrupts
    // intr_enable();

    atomic_t a;
    atomic_set(&a, 7);
    cprintf("a: %d\n", atomic_read(&a));    // a: 7

    long b=16;
    set_bit(1, &b);
    cprintf("b: %d\n", b);  // b: 18

    // init message
    cprintf("\n(THU.CST) ucore(arm)\n");

    // do nothing
    while(1);
}
