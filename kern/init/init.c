#include <assert.h>
#include <clock.h>
#include <console.h>
#include <intr.h>
#include <stdio.h>
#include <string.h>

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
    intr_enable();

    // init message
    cprintf("\n(THU.CST) ucore(arm)\n");

EMPTY_LOOP:
    // do nothing
    while(1);
}
