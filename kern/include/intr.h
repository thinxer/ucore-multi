#ifndef  _KERN_INTR_INC
#define  _KERN_INTR_INC

#include <types.h>

/*
 * Initialize interrupts.
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

#endif   /* ----- #ifndef _KERN_INTR_INC  ----- */
