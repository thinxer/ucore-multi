#ifndef __KERN_TRAP_TRAP_H__
#define __KERN_TRAP_TRAP_H__

#include <types.h>

/* Trap Numbers */

/* Processor-defined: */
#define T_RESET			0   // Reset
#define T_UNDEF			1   // Undefined instruction
#define T_SWI			2   // software interrupt
#define T_PABT			3   // Prefetch abort
#define T_DABT			4   // Data abort
#define T_IRQ			6   // Interrupt request
#define T_FIQ			7   // Fast interrupt request

/* Hardware interrupt:
 * range from 32 to 63
 * */
#define IRQ_OFFSET		32   // Interrupt request
#define IRQ_MAX_RANGE		63

/* General purpose registers minus fp, sp and pc */
struct pushregs {
    uint32_t reg_r [13];
};

/* Trapframe structure */
struct trapframe {
    uint32_t tf_sr;		// saved status register
    uint32_t tf_trapno;		// Trap number
    uint32_t tf_err;		// Error code
    struct pushregs tf_regs;
    uint32_t tf_esp;		// esp
    uint32_t tf_epc;		// eip, actually lr
} __attribute__((packed));


void idt_init(void);
void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);

#endif /* !__KERN_TRAP_TRAP_H__ */

