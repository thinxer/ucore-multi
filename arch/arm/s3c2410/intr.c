#include <mach/intr.h>
#include <stdio.h>
#include <types.h>
#include <string.h>

void
intr_init(void) {
    // copy interrupt vectors to 0x0
    extern char  __intr_vector_start, __intr_vector_end;
    memcpy( (void*)0,
            (void*)&__intr_vector_start,
            &__intr_vector_end - &__intr_vector_start);
}

void
intr_enable(void) {
	asm volatile (
		"mrs r4,cpsr;"
		"bic r4,r4,#0x80;"
		"msr cpsr,r4;"
		:::"r4"
	);
}

void
intr_disable(void) {
	asm volatile (
		"mrs r4,cpsr;"
		"orr r4,r4,#0x80;"
		"msr cpsr,r4;"
		:::"r4"
	);
}

void
intr_mask(uint32_t offset) {
	*(volatile uint32_t *)INTMSK|=(1<<offset);
}

void
intr_umask(uint32_t offset) {
	*(volatile uint32_t *)INTMSK&=~(1<<offset);
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

    // enable for nested interrupt
	intr_enable();
    
    // dispatch interrupt
    cprintf("interrupt occured\n");

    // prepare to return
	intr_disable();
}
