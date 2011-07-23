#include <types.h>
#include <memlayout.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <assert.h>
#include <console.h>
#include <proc.h>
#include <sched.h>
#include <unistd.h>

#define TICK_NUM 5

static void print_ticks() {
    //cprintf("%d ticks\n",ticks);
	cprintf(".");
#ifdef DEBUG_GRADE
    cprintf("End of Test.\n");
    panic("EOT: kernel seems ok.");
#endif
}

static const char *
trapname(int trapno) {
    static const char * const excnames[] = {
        "Reset",
        "Undefined instruction",
        "Software interrupt",
        "Prefetch abort",
        "Data abort",
        "Reserved",
        "Interrupt request",
        "Fast interrupt request"
    };
	
    if (trapno < sizeof(excnames)/sizeof(const char * const)) {
        return excnames[trapno];
    }
    if (trapno == T_SYSCALL) {
        return "System call";
    }
	if (trapno >= IRQ_OFFSET && trapno <= IRQ_OFFSET + IRQ_MAX_RANGE) {
		return "Hardware Interrupt";
    }
    return "(unknown trap)";
}

static const char * const modenames[] = {
	"User",
	"FIQ",
	"IRQ",
	"Supervisor",
	"","","",
	"Abort",
	"","","",
	"Undefined",
	"","","",
	"System"
};

/* trap_in_kernel - test if trap happened in kernel */
bool
trap_in_kernel(struct trapframe *tf) {
    //return (tf->tf_cs == (uint16_t)KERNEL_CS);
	//cprintf("trap_in_kernel: at pc:0x%08x\n", tf->tf_epc);
	//return (tf->tf_epc >= KERNBASE && tf->tf_epc <= KERNTOP);
	return 1;
}

void
print_trapframe(struct trapframe *tf) {
    cprintf("trapframe at %p\n", tf);
    print_regs(&tf->tf_regs);
	cprintf("  sp   0x%08x\n", tf->tf_esp);
	cprintf("  lr   0x%08x\n", tf->tf_epc);
	cprintf("  spsr 0x%08x %s\n", tf->tf_sr, modenames[tf->tf_sr & 0xF]);
    cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
    cprintf("  err  0x%08x\n", tf->tf_err);
}

void
print_regs(struct pushregs *regs) {
	int i;
	for (i = 0; i < 11; i++) {
		cprintf("  r%02d  0x%08x\n", i, regs->reg_r[i]);
	}
	cprintf("  fp   0x%08x\n", regs->reg_r[11]);
	cprintf("  ip   0x%08x\n", regs->reg_r[12]);
}
/*
static inline void
print_pgfault(struct trapframe *tf) {
	//print_trapframe(tf);
	uint32_t ttb;
	asm volatile("MRC p15, 0, %0, c2, c0, 0" :"=r" (ttb));
	cprintf("%s page fault at (0x%08x) 0x%08x 0x%03x: %s-%s %s\n", 
		tf->tf_trapno == T_PABT ? "instruction" : tf->tf_trapno == T_DABT ? "data" : "unknown",
		ttb, far(), tf->tf_err & 0xFFF,
		tf->tf_err & 0x2 ? "Page" : "Section",
		(tf->tf_err & 0xC) == 0xC ? "Permission" : 
		(tf->tf_err & 0xC) == 0x8 ? "Domain" :
		(tf->tf_err & 0xC) == 0x4 ? "Translation" :
		"Alignment",
		//((fsr_v & 0xC) == 0) || ((fsr_v & 0xE) == 0x4) ? "Domain invalid" : "Domain valid",
		(tf->tf_err & 1<<11) ? "W" : "R");*/
	/* error_code:
     * bit 0 == 0 means no page found, 1 means protection fault // translation or domain/permission
     * bit 1 == 0 means read, 1 means write // permission
     * bit 2 == 0 means kernel, 1 means user // can't know
     * */
    //~ cprintf("page fault at 0x%08x: %c/%c [%s].\n", rcr2(),
            //~ (tf->tf_err & 4) ? 'U' : 'K',
            //~ (tf->tf_err & 2) ? 'W' : 'R',
            //~ (tf->tf_err & 1) ? "protection fault" : "no page found");
//}
/*
static int
pgfault_handler(struct trapframe *tf) {

    extern struct mm_struct *check_mm_struct;
    struct mm_struct *mm;
    if (check_mm_struct != NULL) {
        assert(current == idleproc);
        mm = check_mm_struct;
    }
    else {
        if (current == NULL) {
            print_trapframe(tf);
            print_pgfault(tf);
            panic("unhandled page fault.\n");
        }
        mm = current->mm;
    }
	print_pgfault(tf);
    return do_pgfault(mm, tf->tf_err, far());
}
*/
/* trap_dispatch - dispatch based on what type of trap occurred */
static void
trap_dispatch(struct trapframe *tf) {
    char c;

    int ret;

    cprintf("Trap\n");
/*
    switch (tf->tf_trapno) {
	// Prefetch Abort service routine
	// Data Abort service routine
	case T_PABT:
	case T_DABT:
		if ((ret = pgfault_handler(tf)) != 0) {
            print_trapframe(tf);
            if (current == NULL) {
                panic("handle pgfault failed. %e\n", ret);
            }
            else {
                if (trap_in_kernel(tf)) {
                    panic("handle pgfault failed in kernel mode. %e\n", ret);
                }
                cprintf("killed by kernel.\n");
                do_exit(-E_KILLED);
            }
        }
        break;
    case T_SYSCALL:
        syscall();
        break;
	// IRQ Service Routine
	case IRQ_OFFSET + INT_TIMER4:
		ticks ++;
		if (ticks % TICK_NUM == 0) {
			print_ticks();
			//print_trapframe(tf);
		}
		break;
	case IRQ_OFFSET + INT_UART0:
		c = cons_getc();
		cprintf("serial [%03d] %c\n", c, c);
		break;
	// SWI Service Routine
    case T_SWITCH_TOK: // a random System call
		cprintf("Random system call\n") ;
		print_cur_status();
		//print_stackframe();
        break;
	case T_PANIC: // System call
		cprintf("Game over\n") ;
		print_cur_status();
		//print_stackframe();
        break;
    default:*/
        print_trapframe(tf);
        if (current != NULL) {
            cprintf("unhandled trap.\n");
            //do_exit(-E_KILLED);
        }
        panic("unexpected trap in kernel.\n");
    //}
}

/* *
 * trap - handles or dispatches an exception/interrupt. if and when trap() returns,
 * the code in kern/trap/trapentry.S will restore the state before the exception.
 * */
void
trap(struct trapframe *tf) {
    // used for previous projects
    if (current == NULL) {
        trap_dispatch(tf);
    }
    else {
        // keep a trapframe chain in stack
        struct trapframe *otf = current->tf;
        current->tf = tf;

        bool in_kernel = trap_in_kernel(tf);

        trap_dispatch(tf);

        current->tf = otf;
        if (!in_kernel) {
            if (current->flags & PF_EXITING) {
                //do_exit(-E_KILLED);
            }
            if (current->need_resched) {
                schedule();
            }
        }
    }
}

