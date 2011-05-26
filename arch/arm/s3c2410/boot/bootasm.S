# init exception vector table
.code 32
.align 0

.global _start
.global __intr_vector_start
.global __intr_vector_end

.extern bootmain
.extern __bss_start__
.extern __bss_end__

.section .startup

_start:

	ldr	pc,_vector_reset
	ldr pc,_vector_undefined
	ldr pc,_vector_swi
	ldr pc,_vector_prefetch_abort
	ldr pc,_vector_data_abort
	ldr pc,_vector_reserved
	ldr pc,_vector_irq
	ldr pc,_vector_fiq

.align 4

__intr_vector_start:

_vector_reset:	.word	__vector_reset
_vector_undefined:	.word	__vector_undefined
_vector_swi:	.word	__vector_swi
_vector_prefetch_abort:	.word	__vector_prefetch_abort
_vector_data_abort:	.word	__vector_data_abort
_vector_reserved:	.word	__vector_reserved
_vector_irq:	.word	__vector_irq
_vector_fiq:	.word	__vector_fiq

__intr_vector_end:

# reset
.equ DISABLE_IRQ,		0x80
.equ DISABLE_FIQ,		0x40
.equ SYS_MOD,			0x1f
.equ IRQ_MOD,			0x12
.equ FIQ_MOD,			0x11
.equ SVC_MOD,			0x13
.equ ABT_MOD,			0x17
.equ UND_MOD,			0x1b

.equ MEM_SIZE,			0x04000000
.equ TEXT_BASE,			0x30000000

.equ _SVC_STACK,		(TEXT_BASE+MEM_SIZE-4)
.equ _IRQ_STACK,		(_SVC_STACK-0x400)
.equ _FIQ_STACK,		(_IRQ_STACK-0x400)
.equ _ABT_STACK,		(_FIQ_STACK-0x400)
.equ _UND_STACK,		(_ABT_STACK-0x400)
.equ _SYS_STACK,		(_UND_STACK-0x400)

__vector_reset:
    msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SVC_MOD)
	ldr	sp,=_SVC_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|IRQ_MOD)
	ldr	sp,=_IRQ_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|FIQ_MOD)
	ldr	sp,=_FIQ_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|ABT_MOD)
	ldr	sp,=_ABT_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|UND_MOD)
	ldr	sp,=_UND_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SYS_MOD)
	ldr	sp,=_SYS_STACK

_clear_bss:
	ldr r1,_bss_start_
	ldr r3,_bss_end_
	mov r2,#0x0
1:
	cmp r1,r3
	beq _main
	str r2,[r1],#0x4
	b	1b

_main:
	b bootmain

# exception modes
__vector_undefined:
	nop
__vector_swi:
	nop
__vector_prefetch_abort:
	nop
__vector_data_abort:
	nop
__vector_reserved:
	nop
__vector_irq:
	nop
__vector_fiq:
	nop

_bss_start_:.word   __bss_start__
_bss_end_:.word   __bss_end__
