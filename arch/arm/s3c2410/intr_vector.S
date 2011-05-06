/* This file contains the interrupt vector.
 * __intr_vector_start and __intr_vector_end marks where the vector resides
 * after linking. This range should either be copied to 0x0 or be mapped
 * there.
 */
.global __intr_vector_start
.global __intr_vector_end
.extern intr_dispatch

/* TODO put this magic numbers into an include file. */
.equ DISABLE_IRQ,		0x80
.equ DISABLE_FIQ,		0x40
.equ SYS_MOD,			0x1f
.equ IRQ_MOD,			0x12
.equ FIQ_MOD,			0x11
.equ SVC_MOD,			0x13
.equ ABT_MOD,			0x17
.equ UND_MOD,			0x1b

.code 32

__intr_vector_start:

	ldr	pc,reset
	ldr pc,undefined
	ldr pc,swi
	ldr pc,prefetch_abort
	ldr pc,data_abort
	ldr pc,reserved
	ldr pc,irq
	ldr pc,fiq


.align 4

reset:
    .word	_kern_init
undefined:
    .word   .
swi:
    .word	.
prefetch_abort:
    .word	.
data_abort:
    .word	.
reserved:
    .word	.
irq:
    .word	irq_proc
fiq:
    .word	.

__intr_vector_end:

/* irq procedure, which will call intr_dispatch.
 */
irq_proc:
	sub lr,lr,#4
	stmfd sp!,{r14}
	mrs lr,spsr
	stmfd sp!,{r14}

    /* Go into svc mode and responds to the interrupt. */
    msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SVC_MOD)

	stmfd sp!,{r0-r3}
	bl intr_dispatch
	ldmfd sp!,{r0-r3}

    /* Return to the irq mode. */
    msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|IRQ_MOD)

	ldmfd sp!,{r14}
	msr spsr,r14
	ldmfd sp!,{pc}^
