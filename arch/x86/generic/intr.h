#ifndef __ARCH_X86_INTR_H__
#define __ARCH_X86_INTR_H__

#include <types.h>

/* Trap Numbers */

/* Processor-defined: */
#define T_DIVIDE                0   // divide error
#define T_DEBUG                 1   // debug exception
#define T_NMI                   2   // non-maskable interrupt
#define T_BRKPT                 3   // breakpoint
#define T_OFLOW                 4   // overflow
#define T_BOUND                 5   // bounds check
#define T_ILLOP                 6   // illegal opcode
#define T_DEVICE                7   // device not available
#define T_DBLFLT                8   // double fault
// #define T_COPROC             9   // reserved (not used since 486)
#define T_TSS                   10  // invalid task switch segment
#define T_SEGNP                 11  // segment not present
#define T_STACK                 12  // stack exception
#define T_GPFLT                 13  // general protection fault
#define T_PGFLT                 14  // page fault
// #define T_RES                15  // reserved
#define T_FPERR                 16  // floating point error
#define T_ALIGN                 17  // aligment check
#define T_MCHK                  18  // machine check
#define T_SIMDERR               19  // SIMD floating point error

/* Hardware IRQ numbers. We receive these as (IRQ_OFFSET + IRQ_xx) */
#define IRQ_OFFSET              32  // IRQ 0 corresponds to int IRQ_OFFSET

#define IRQ_TIMER               0
#define IRQ_KBD                 1
#define IRQ_COM1                4
#define IRQ_IDE1                14
#define IRQ_IDE2                15
#define IRQ_ERROR               19
#define IRQ_SPURIOUS            31

/*
 * Initialize interrupts.
 * Currently it'll set up the interrupt vector table.
 */
void intr_init(void);

/*
 * Enable interrupts.
 */
void intr_enable(void);
/*
 * Disable interrupts.
 */
void intr_disable(void);
/*
 * Mask an interrupt (disable the interrupt).
 */
void intr_mask(uint32_t offset);
/*
 * Unmask an interrupt (enable the interrupt).
 */
void intr_umask(uint32_t offset);

/*
 * Main interrupt dispatcher.
 */
void intr_dispatch(void);

#endif /* !__ARCH_X86_INTR_H__ */
