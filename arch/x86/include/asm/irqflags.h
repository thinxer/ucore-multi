#ifndef _X86_IRQFLAGS_H_
#define _X86_IRQFLAGS_H_

/*
 * Interrupt control:
 */

static inline unsigned long native_save_fl(void)
{
	unsigned long flags;

	/*
	 * "=rm" is safe here, because "pop" adjusts the stack before
	 * it evaluates its effective address -- this is part of the
	 * documented behavior of the "pop" instruction.
	 */
	asm volatile("# __raw_save_flags\n\t"
		     "pushf ; pop %0"
		     : "=rm" (flags)
		     : /* no input */
		     : "memory");

	return flags;
}

static inline void native_restore_fl(unsigned long flags)
{
	asm volatile("push %0 ; popf"
		     : /* no output */
		     :"g" (flags)
		     :"memory", "cc");
}

static inline void native_irq_disable(void)
{
	asm volatile("cli": : :"memory");
}

static inline void native_irq_enable(void)
{
	asm volatile("sti": : :"memory");
}

static inline unsigned long __arch_local_save_flags(void)
{
	return native_save_fl();
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	native_restore_fl(flags);
}

static inline void arch_local_irq_disable(void)
{
	native_irq_disable();
}

static inline void arch_local_irq_enable(void)
{
	native_irq_enable();
}

/*
 * For spinlocks, etc:
 */
static inline unsigned long __arch_local_irq_save(void)
{
	unsigned long flags = __arch_local_save_flags();

	arch_local_irq_disable();

	return flags;
}

#define arch_local_save_flags(flags)				\
	do { (flags) = __arch_local_save_flags(); } while (0)

#define arch_local_irq_save(flags)				\
	do { (flags) = __arch_local_irq_save(); } while (0)

#define X86_EFLAGS_IF	0x00000200 /* Interrupt Flag */
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & X86_EFLAGS_IF);
}

static inline int arch_irqs_disabled(void)
{
	unsigned long flags = __arch_local_save_flags();

	return arch_irqs_disabled_flags(flags);
}

#endif

