#ifndef __ARCH_ARM_INTR_H__
#define __ARCH_ARM_INTR_H__

#include <types.h>

#define INT_BASE	(0x4a000000)
#define INTMSK		(INT_BASE+0x8)
#define INTOFFSET	(INT_BASE+0x14)
#define INTPND		(INT_BASE+0x10)
#define SRCPND		(INT_BASE+0x0)

struct pushregs {
    uint32_t reg_r [13];
} __attribute__((packed));

struct trapframe {
    struct pushregs tf_regs;
    uint32_t tf_spsr;
    uint32_t tf_pc;
} __attribute__((packed));

#include <intr.h>

#endif /* !__ARCH_ARM_INTR_H__ */
