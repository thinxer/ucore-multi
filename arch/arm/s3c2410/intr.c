#include <arch/intr.h>
#include <stdio.h>
#include <types.h>
#include <string.h>
#include <irqflags.h>

void
intr_init(void) {
    // The interrupt table has been mapped to 0x0 in pmm_init, no need to do
    // anything here.
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
	*(volatile uint32_t *)INTMSK |= (1<<offset);
}

void
intr_umask(uint32_t offset) {
	*(volatile uint32_t *)INTMSK &= ~(1<<offset);
}

static void
intr_clearpending(uint32_t offset) {
    *(volatile uint32_t *)SRCPND |= 1 << offset;
    *(volatile uint32_t *)INTPND |= 1 << offset;
}

void
intr_dispatch(void) {
	uint32_t int_offset = *(volatile uint32_t *) INTOFFSET;
    intr_clearpending(int_offset);
	cprintf("%d\t", int_offset);

    // enable interrupt nesting
	intr_enable();

    // dispatch interrupt
    cprintf("interrupt occured\n");

    // disable interrupt and prepare to return
	intr_disable();
}
