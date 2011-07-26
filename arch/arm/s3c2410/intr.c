#include <assert.h>
#include <stdio.h>
#include <types.h>
#include <string.h>
#include <mach/intr.h>
#include <irqflags.h>
#include <arch/mmu.h>
#include <vmm.h>

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

static void
print_tf(struct trapframe* tf) {
    int i;
    cprintf("tf->pc: %08lx\n", tf->tf_pc);
    cprintf("tf->spsr: %08lx\n", tf->tf_spsr);
    for (i=0; i<13; i++) {
        cprintf("tf->regs[%d]: %08lx\n", i, tf->tf_regs.reg_r[i]);
    }
}

void
intr_dispatch(struct trapframe* tf, uint32_t code) {
	uint32_t int_offset = *(volatile uint32_t *) INTOFFSET;
    intr_clearpending(int_offset);
	cprintf("%d:%d\t", code, int_offset);

    // enable interrupt nesting
	intr_enable();

    // dispatch interrupt
    cprintf("tf@0x%08lx\n", tf);
    cprintf("tf->tf_pc@0x%08lx\n", &tf->tf_pc);
    print_tf(tf);
    cprintf("interrupt occured\n");

    // disable interrupt and prepare to return
	intr_disable();
}

void
intr_data_abort(void) {
    extern struct mm_struct *check_mm_struct;
    unsigned long fsr;
    uint32_t code;
    uintptr_t far;
    read_fsr(fsr);
    read_far(far);
    cprintf("pgfault@%08lx, type: %ld\n", far, fsr);
    // TODO need further investigation
    if (fsr & 0x5) {
        code = 2;
    } else if (fsr & 0xd) {
        code = 1;
    } else {
        panic("unknown pgfault type.\n");
    }
    if (check_mm_struct != NULL) {
        int ret = do_pgfault(check_mm_struct, code, far);
        if (ret != 0) {
            panic("do_pgfault failed! code: %d\n", ret);
        }
        return;
    }
    panic("unhandled page fault.\n");
}

void unhandled_mode(void) {
    panic("unhandled mode\n");
}
