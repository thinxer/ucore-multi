/*
 * Stolen from the Linux Kernel.
 * Removed some constant optimizations for simplicity.
 *
 *
 * Copyright 1995, Russell King.
 * Various bits and pieces copyrights include:
 *  Linus Torvalds (test_bit).
 * Big endian support: Copyright 2001, Nicolas Pitre
 *  reworked by rmk.
 *
 * bit 0 is the LSB of an "unsigned long" quantity.
 *
 * Please note that the code in this file should never be included
 * from user space.  Many of these are not implemented in assembler
 * since they would be too costly.  Also, they require privileged
 * instructions (which are not available from user mode) to ensure
 * that they are atomic.
 */

#ifndef __ASM_ARM_BITOPS_H
#define __ASM_ARM_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <lib/bitops.h> can be included directly
#endif

#include <irqflags.h>

/*
 * These functions are the basis of our bit ops.
 *
 * First, the atomic bitops. These use native endian.
 */
static inline void ____atomic_set_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p |= mask;
	raw_local_irq_restore(flags);
}

static inline void ____atomic_clear_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p &= ~mask;
	raw_local_irq_restore(flags);
}

static inline void ____atomic_change_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p ^= mask;
	raw_local_irq_restore(flags);
}

static inline int
____atomic_test_and_set_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	res = *p;
	*p = res | mask;
	raw_local_irq_restore(flags);

	return (res & mask) != 0;
}

static inline int
____atomic_test_and_clear_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	res = *p;
	*p = res & ~mask;
	raw_local_irq_restore(flags);

	return (res & mask) != 0;
}

static inline int
____atomic_test_and_change_bit(unsigned int bit, volatile unsigned long *p)
{
	unsigned long flags;
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	res = *p;
	*p = res ^ mask;
	raw_local_irq_restore(flags);

	return (res & mask) != 0;
}

#include <bitops/non-atomic.h>

#define	ATOMIC_BITOP_LE(name,nr,p) (____atomic_##name(nr, p))

/*
 * These are the little endian, atomic definitions.
 */
#define set_bit(nr,p)			ATOMIC_BITOP_LE(set_bit,nr,p)
#define clear_bit(nr,p)			ATOMIC_BITOP_LE(clear_bit,nr,p)
#define change_bit(nr,p)		ATOMIC_BITOP_LE(change_bit,nr,p)
#define test_and_set_bit(nr,p)		ATOMIC_BITOP_LE(test_and_set_bit,nr,p)
#define test_and_clear_bit(nr,p)	ATOMIC_BITOP_LE(test_and_clear_bit,nr,p)
#define test_and_change_bit(nr,p)	ATOMIC_BITOP_LE(test_and_change_bit,nr,p)

#endif /* _ARM_BITOPS_H */
