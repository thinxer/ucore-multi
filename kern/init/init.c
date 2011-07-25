#include <assert.h>
#include <clock.h>
#include <console.h>
#include <intr.h>
#include <stdio.h>
#include <string.h>
#include <atomic.h>
#include <bitops.h>
#include <pmm.h>
#include <proc.h>
#include <slab.h>
#include <sched.h>

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
    //asm volatile("ldr r0, =0xF0000000\nmov r1, #0xFE\nstr r1, [r0]": : :"r0", "r1");
    // console
    cons_init();
   
    //asm volatile("ldr r0, =0xF0000000\nmov r1, #0xFD\nstr r1, [r0]": : :"r0", "r1");
    cprintf("initializing\n");

    //asm volatile("ldr r0, =0xF0000000\nmov r1, #0xFC\nstr r1, [r0]": : :"r0", "r1");
    // interrupts
    intr_init();

    // physical memory management
    pmm_init();
    slab_init();

    atomic_t a;
    atomic_set(&a, 7);
    cprintf("a: %d\n", atomic_read(&a));    // a: 7

    unsigned long b=16;
    set_bit(1, &b);
    cprintf("b: %d\n", b);  // b: 18

    sched_init();

    proc_init();

    // welcome message
    cprintf("\n(THU.CST) ucore\n");

    // timer
    clock_init();

    // enable interrupts
    intr_enable();

    cpu_idle();
    // do nothing
    while(1)
	    cprintf("!");
}

