#include <assert.h>
#include <stdio.h>
#include <types.h>
#include <string.h>
#include <mach/intr.h>
#include <irqflags.h>
#include <clock.h>
#include <sched.h>

#define TICK_NUM 10

void
intr_init(void) {
    // The interrupt table has been mapped to 0x0 in pmm_init, no need to do
    // anything here.
    extern char __intr_vector_start[], __intr_vector_end[];
    memcpy(0, __intr_vector_start, __intr_vector_end - __intr_vector_start);
    cprintf("vector:%08lx\n", *(unsigned int*)0);
}

void
intr_enable(void) {
    arch_local_irq_enable();
}

void
intr_disable(void) {
    arch_local_irq_disable();
}

void
intr_mask(uint32_t offset) {
	//*(volatile uint32_t *)INTMSK |= (1<<offset);
}

void
intr_umask(uint32_t offset) {
//	*(volatile uint32_t *)INTMSK &= ~(1<<offset);
}

static void
intr_clearpending(uint32_t offset) {
//    *(volatile uint32_t *)SRCPND |= 1 << offset;
//    *(volatile uint32_t *)INTPND |= 1 << offset;
}

void
intr_irq(void) {
	cprintf("!");
	ticks++;
	run_timer_list();
	if (ticks % TICK_NUM == 0){
		assert(current != NULL);
		current->need_resched = 1;
	}
/*
	uint32_t int_offset = *(volatile uint32_t *) INTOFFSET;
    intr_clearpending(int_offset);
	cprintf("%d\t", int_offset);
*/
    // enable interrupt nesting
	//intr_enable();

    // dispatch interrupt
    //cprintf("interrupt occured\n");

    // disable interrupt and prepare to return
	//intr_disable();
}
void unhandled_mode(void) {
    panic("unhandled mode\n");
}
