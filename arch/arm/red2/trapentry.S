# vectors.S sends all traps here.
.text
# common part of all exceptions, exit
.globl __trapret
__trapret:
	# Get rid of spsr, trap number, error code
	# to prepare for the context restoration
	ldmfd sp!,{r0-r2}
	
	# Either spsr is the supervisor mode, so we restore the registers excluding sp
	# or if spsr is the user mode, we update the whole range of registers including sp
	#and r0, r0, #0xF
	# in case spsr is user mode, r0 should be null
	#cmp r0, #0
	#beq __restore_user
	
	# Exiting the exception handler
	# restoring the registers at the state before exception
	# valid for every "non user" mode
	ldmfd sp!,{r0-r12}
	
	# discard esp value
	add sp, sp, #4
	
	# CPSR is automatically restored
	ldmfd sp!,{pc}^

__restore_user:
	# restoring the registers to the user banks, including sp
	ldmfd sp,{r0-r13}^
	add sp, sp, #56
	ldmfd sp!,{pc}^

# .align 4
_memo:	
	.word	_memo

__save_user:
	# save sp from user bank to stack
	stmfd sp,{sp}^
	# ldr r0,=_memo
	# ldr sp, [sp, #-4]
	# str sp, [r0]
	mov pc, lr

.globl forkrets
forkrets:
    # set stack to this new process's trapframe
	mov sp, r0
	b __trapret
	ldmfd sp!,{r0-r2}
	# in case the saved register is supervisor, it may have been manually saved
	# load the register and reload the pc without ^
	# in case the saved register is user, load everything + sp
	#and r0, r0, #0xF
	#cmp r0, #0
	#beq __restore_user
	ldmfd sp!,{r0-r12}
	add sp, sp, #4
	ldmfd sp!,{pc}^


.globl __abort
__abort:

