#ifndef __ASM_ARM_IRQFLAGS_H
#define __ASM_ARM_IRQFLAGS_H

#define PSR_I 0x00000080

/*
 * Save the current interrupt enable state & disable IRQs
 */
#define arch_local_irq_save(x)					\
	({							\
		unsigned long temp;				\
		(void) (&temp == &x);				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_save\n"	\
"	orr	%1, %0, #128\n"					\
"	msr	cpsr_c, %1"					\
	: "=r" (x), "=r" (temp)					\
	:							\
	: "memory", "cc");					\
	})

/*
 * Enable IRQs
 */
#define arch_local_irq_enable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_enable\n"	\
"	bic	%0, %0, #128\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

/*
 * Disable IRQs
 */
#define arch_local_irq_disable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_irq_disable\n"	\
"	orr	%0, %0, #128\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

/*
 * Enable FIQs
 */
#define local_fiq_enable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ stf\n"		\
"	bic	%0, %0, #64\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

/*
 * Disable FIQs
 */
#define local_fiq_disable()					\
	({							\
		unsigned long temp;				\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ clf\n"		\
"	orr	%0, %0, #64\n"					\
"	msr	cpsr_c, %0"					\
	: "=r" (temp)						\
	:							\
	: "memory", "cc");					\
	})

/*
 * Save the current interrupt enable state.
 */
#define arch_local_save_flags(x)					\
	({							\
	__asm__ __volatile__(					\
	"mrs	%0, cpsr		@ local_save_flags"	\
	: "=r" (x) : : "memory", "cc");				\
	})

/*
 * restore saved IRQ & FIQ state
 */
#define arch_local_irq_restore(x)				\
	__asm__ __volatile__(					\
	"msr	cpsr_c, %0		@ local_irq_restore\n"	\
	:							\
	: "r" (x)						\
	: "memory", "cc")

#define arch_irqs_disabled_flags(flags)	\
({					\
	(int)((flags) & PSR_I_BIT);	\
})

#endif

/* equivalent of sti assembly instruction */
static inline void
irq_flag_enable(void) {
	asm volatile ("mrs r0, cpsr;"
		"bic r0, r0, #0x80;"
		"msr cpsr, r0;"
	:::"r0", "memory", "cc");
}

/* equivalent of cti assembly instruction */
static inline void
irq_flag_disable(void) {
	asm volatile ("mrs r0, cpsr;"
		"orr r0, r0, #0x80;"
		"msr cpsr, r0;"
	:::"r0", "memory", "cc");
}

static inline uint32_t
read_psrflags(void) {
	uint32_t psrflags;
	asm volatile ("mrs %0, cpsr" : "=r" (psrflags));
	return psrflags;
}

