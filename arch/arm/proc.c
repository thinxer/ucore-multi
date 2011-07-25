#include <proc.h>
#include <slab.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <trap.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* ------------- process/thread mechanism design&implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  ucore implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in lab6), etc. ucore needs to
manage all these details efficiently. In ucore, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc, 
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:
                                            
  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+ 
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  + 
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep 
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit   
SYS_getpid      : get the process's pid

*/

// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT          10
#define HASH_LIST_SIZE      (1 << HASH_SHIFT)
#define pid_hashfn(x)       (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

// idle proc
struct proc_struct *idleproc = NULL;
// init proc
struct proc_struct *initproc = NULL;
// current proc
struct proc_struct *current = NULL;

static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);

static int __do_exit(void);
static int __do_kill(struct proc_struct *proc, int error_code); 

// alloc_proc - create a proc struct and init fields
static struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        memset(&(proc->context), 0, sizeof(struct context));
        proc->tf = NULL;
        proc->flags = 0;
        memset(proc->name, 0, PROC_NAME_LEN);
        proc->wait_state = 0;
        proc->cptr = proc->optr = proc->yptr = NULL;
        list_init(&(proc->thread_group));
    }
    return proc;
}

// set_proc_name - set the name of proc
char *
set_proc_name(struct proc_struct *proc, const char *name) {
    memset(proc->name, 0, sizeof(proc->name));
    return memcpy(proc->name, name, PROC_NAME_LEN);
}

// get_proc_name - get the name of proc
char *
get_proc_name(struct proc_struct *proc) {
    static char name[PROC_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, proc->name, PROC_NAME_LEN);
}

// set_links - set the relation links of process
static void
set_links(struct proc_struct *proc) {
    list_add(&proc_list, &(proc->list_link));
    proc->yptr = NULL;
    if ((proc->optr = proc->parent->cptr) != NULL) {
        proc->optr->yptr = proc;
    }
    proc->parent->cptr = proc;
    nr_process ++;
}

// remove_links - clean the relation links of process
static void
remove_links(struct proc_struct *proc) {
    list_del(&(proc->list_link));
    if (proc->optr != NULL) {
        proc->optr->yptr = proc->yptr;
    }
    if (proc->yptr != NULL) {
        proc->yptr->optr = proc->optr;
    }
    else {
       proc->parent->cptr = proc->optr;
    }
    nr_process --;
}

// get_pid - alloc a unique pid for process
static int
get_pid(void) {
    static_assert(MAX_PID > MAX_PROCESS);
    struct proc_struct *proc;
    list_entry_t *list = &proc_list, *le;
    static int next_safe = MAX_PID, last_pid = MAX_PID;
    if (++ last_pid >= MAX_PID) {
        last_pid = 1;
        goto inside;
    }
    if (last_pid >= next_safe) {
    inside:
        next_safe = MAX_PID;
    repeat:
        le = list;
        while ((le = list_next(le)) != list) {
            proc = le2proc(le, list_link);
            if (proc->pid == last_pid) {
                if (++ last_pid >= next_safe) {
                    if (last_pid >= MAX_PID) {
                        last_pid = 1;
                    }
                    next_safe = MAX_PID;
                    goto repeat;
                }
            }
            else if (proc->pid > last_pid && next_safe > proc->pid) {
                next_safe = proc->pid;
            }
        }
    }
    return last_pid;
}

// proc_run - make process "proc" running on cpu
// NOTE: before call switch_to, should load  base addr of "proc"'s new PDT
void
proc_run(struct proc_struct *proc) {
    if (proc != current) {
        bool intr_flag;
        struct proc_struct *prev = current, *next = proc;
        local_intr_save(intr_flag);
        {
            current = proc;
            //load_esp0(next->kstack + KSTACKSIZE);
            //lcr3(next->cr3);
            switch_to(&(prev->context), &(next->context));
        }
        local_intr_restore(intr_flag);
    }
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void
forkret(void) {
    forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void
hash_proc(struct proc_struct *proc) {
    list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}

// unhash_proc - delete proc from proc hash_list
static void
unhash_proc(struct proc_struct *proc) {
    list_del(&(proc->hash_link));
}

// find_proc - find proc frome proc hash_list according to pid
struct proc_struct *
find_proc(int pid) {
    if (0 < pid && pid < MAX_PID) {
        list_entry_t *list = hash_list + pid_hashfn(pid), *le = list;
        while ((le = list_next(le)) != list) {
            struct proc_struct *proc = le2proc(le, hash_link);
            if (proc->pid == pid) {
                return proc;
            }
        }
    }
    return NULL;
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int
kernel_thread(int (*fn)(void *), void *arg) {
    struct trapframe tf;
    memset(&tf, 0, sizeof(struct trapframe));
    tf.tf_regs.reg_r[0] = (uint32_t)arg;
    tf.tf_regs.reg_r[1] = (uint32_t)fn; // address to jump to (fn) is in r1, arg is in r0
    tf.tf_epc = (uint32_t)kernel_thread_entry; // look at entry.S
    asm volatile ("mrs %0, cpsr" : "=r" (tf.tf_sr)); // get spsr to be restored
    return do_fork(0, 0, &tf);
}

// setup_kstack - alloc pages with size KSTACKPAGE as process kernel stack
static int
setup_kstack(struct proc_struct *proc) {
    struct Page *page = alloc_pages(KSTACKPAGE);
    if (page != NULL) {
        proc->kstack = (uintptr_t)page; //(uintptr_t)page2kva(page);
        return 0;
    }
    return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void
put_kstack(struct proc_struct *proc) {
    free_pages((void*)(proc->kstack), KSTACKPAGE); //kva2page((void *)(proc->kstack)), KSTACKPAGE);
}

// de_thread - delete this thread "proc" from thread_group list
static void
de_thread(struct proc_struct *proc) {
    if (!list_empty(&(proc->thread_group))) {
        unsigned long intr_flag;
        local_intr_save(intr_flag);
        {
            list_del_init(&(proc->thread_group));
        }
        local_intr_restore(intr_flag);
    }
}

// next_thread - get the next thread "proc" from thread_group list
static struct proc_struct *
next_thread(struct proc_struct *proc) {
    return le2proc(list_next(&(proc->thread_group)), thread_group);
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf) {
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    assert((uintptr_t)proc->tf + sizeof(struct trapframe) == proc->kstack + KSTACKSIZE);
    *(proc->tf) = *tf;
    proc->tf->tf_regs.reg_r[3] = 0;
    proc->tf->tf_esp = esp;
    proc->tf->tf_sr &= ~0x80;

    proc->context.epc = (uintptr_t)forkret;
    proc->context.esp = (uintptr_t)(proc->tf);
}

// may_killed - check if current thread should be killed, should be called before go back to user space
void
may_killed(void) {
    // killed by other process, already set exit_code and call __do_exit directly
    if (current->flags & PF_EXITING) {
        __do_exit();
    }
}

// do_fork - parent process for a new child process
//    1. call alloc_proc to allocate a proc_struct
//    2. call setup_kstack to allocate a kernel stack for child process
//    3. call copy_mm to dup OR share mm according clone_flag
//    4. call wakup_proc to make the new child process RUNNABLE 
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }

    ret = -E_NO_MEM;

    if ((proc = alloc_proc()) == NULL) {
	cprintf("alloc_proc failed\n");
        goto fork_out;
    }

    proc->parent = current;
    list_init(&(proc->thread_group));
    assert(current->wait_state == 0);

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    copy_thread(proc, (uintptr_t)(proc->kstack), tf);

    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        set_links(proc);
    }
    local_intr_restore(intr_flag);

    wakeup_proc(proc);
	
    ret = proc->pid;
fork_out:
    return ret;

bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}

// __do_exit - cause a thread exit (use do_exit, do_exit_thread instead)
//   1. call exit_mmap & put_pgdir & mm_destroy to free the almost all memory space of process
//   2. set process' state as PROC_ZOMBIE, then call wakeup_proc(parent) to ask parent reclaim itself.
//   3. call scheduler to switch to other process
static int
__do_exit(void) {
    if (current == idleproc) {
        panic("idleproc exit.\n");
    }
    if (current == initproc) {
        panic("initproc exit.\n");
    }

    current->state = PROC_ZOMBIE;

    //cprintf("__do_exit\n");
    
    bool intr_flag;
    struct proc_struct *proc, *parent;
    local_intr_save(intr_flag);
    {
        proc = parent = current->parent;
        do {
            if (proc->wait_state == WT_CHILD) {
                wakeup_proc(proc);
            }
            proc = next_thread(proc);
        } while (proc != parent);

        if ((parent = next_thread(current)) == current) {
            parent = initproc;
        }
        de_thread(current);
        while (current->cptr != NULL) {
            proc = current->cptr;
            current->cptr = proc->optr;

            proc->yptr = NULL;
            if ((proc->optr = parent->cptr) != NULL) {
                parent->cptr->yptr = proc;
            }
            proc->parent = parent;
            parent->cptr = proc;
            if (proc->state == PROC_ZOMBIE) {
                if (parent->wait_state == WT_CHILD) {
                    wakeup_proc(parent);
                }
            }
        }
    }
    local_intr_restore(intr_flag);

    //cprintf("__do_exit done\n");
    schedule();
    panic("__do_exit will not return!! %d %d.\n", current->pid, current->exit_code);
}

// do_exit - kill a thread group, called by syscall or trap handler
int
do_exit(int error_code) {
    unsigned long intr_flag;
    //cprintf("do_exit\n");
    local_intr_save(intr_flag);
    {
        list_entry_t *list = &(current->thread_group), *le = list;
        while ((le = list_next(le)) != list) {
            struct proc_struct *proc = le2proc(le, thread_group);
            __do_kill(proc, error_code);
        }
    }
    local_intr_restore(intr_flag);
    return do_exit_thread(error_code);
}

// do_exit_thread - kill a single thread
int
do_exit_thread(int error_code) {
    current->exit_code = error_code;
    return __do_exit();
}

// do_yield - ask the scheduler to reschedule
int
do_yield(void) {
    current->need_resched = 1;
    return 0;
}

// do_wait - wait one OR any children with PROC_ZOMBIE state, and free memory space of kernel stack
//         - proc struct of this child.
// NOTE: only after do_wait function, all resources of the child proces are free.
int
do_wait(int pid, int *code_store) {
    //cprintf("do_wait\n");
    struct proc_struct *proc, *cproc;
    unsigned long intr_flag, haskid;
repeat:
    cproc = current;
    haskid = 0;
    if (pid != 0) {
        proc = find_proc(pid);
        if (proc != NULL) {
            do {
                if (proc->parent == cproc) {
                    haskid = 1;
                    if (proc->state == PROC_ZOMBIE) {
                        goto found;
                    }
                    break;
                }
                cproc = next_thread(cproc);
            } while (cproc != current);
        }
    }
    else {
        do {
            proc = cproc->cptr;
            for (; proc != NULL; proc = proc->optr) {
                haskid = 1;
                if (proc->state == PROC_ZOMBIE) {
                    goto found;
                }
            }
            cproc = next_thread(cproc);
        } while (cproc != current);
    }
    if (haskid) {
	//cprintf("found runnable child, goto sleep\n");
        current->state = PROC_SLEEPING;
        current->wait_state = WT_CHILD;
        schedule();
	//cprintf("schedule back\n");
        may_killed();
        goto repeat;
    }
    return -E_BAD_PROC;

found:
    //cprintf("found zombie child\n");
    if (proc == idleproc || proc == initproc) {
        panic("wait idleproc or initproc.\n");
    }
    int exit_code = proc->exit_code;
    local_intr_save(intr_flag);
    {
        unhash_proc(proc);
        remove_links(proc);
    }
    local_intr_restore(intr_flag);
    put_kstack(proc);
    kfree(proc);

    int ret = 0;
    return ret;
}

// __do_kill - kill a process with PCB by set this process's flags with PF_EXITING
static int
__do_kill(struct proc_struct *proc, int error_code) {
    if (!(proc->flags & PF_EXITING)) {
        proc->flags |= PF_EXITING;
        proc->exit_code = error_code;
        if (proc->wait_state & WT_INTERRUPTED) {
            wakeup_proc(proc);
        }
        return 0;
    }
    return -E_KILLED;
}

// do_sleep - set current process state to sleep and add timer with "time"
//          - then call scheduler. if process run again, delete timer first.
int
do_sleep(unsigned int time) {
    if (time == 0) {
        return 0;
    }
    unsigned long intr_flag;
    local_intr_save(intr_flag);
    timer_t __timer, *timer = timer_init(&__timer, current, time);
    current->state = PROC_SLEEPING;
    current->wait_state = WT_TIMER;
    add_timer(timer);
    local_intr_restore(intr_flag);

    schedule();

    del_timer(timer);
    return 0;
}


// kernel_execve - do SYS_exec syscall to exec a user program called by user_main kernel_thread
static int
kernel_execve(const char *name, unsigned char *binary, size_t size) {
    int ret, len = strlen(name);
    asm volatile (
	"mov r4, %2\n\t"
	"mov r5, %3\n\t"
	"mov r6, %4\n\t"
	"mov r7, %5\n\t"
	"mov r8, %6\n\t"
	"swi %1\n\t"
	: "=r" (ret)
	: "I" (T_SYSCALL), "r" (SYS_exec), "r" (name), "r" (len), "r" (binary), "r" (size)
	: "memory");
    return ret;
}

// user_main - kernel thread used to exec a user program
static int
user_main(void *arg) {
    cprintf("Hello World!\n");
    int i;
    for(i=0; i<10; ++i){
        cprintf("[%d]do some thing\n", *(int*)arg);
	do_sleep(20);
    }
    return 0;
}

// init_main - the second kernel thread used to create kswapd_main & user_main kernel threads
static int
init_main(void *arg) {
    cprintf("init_main\n");
    int pid;

    size_t nr_free_pages_store = nr_free_pages();
    size_t slab_allocated_store = slab_allocated();

    int a = 1;
    pid = kernel_thread(user_main, &a);
    if (pid <= 0)
        panic("create user_main failed.\n");

    int i;
    for(i=0; i<10; ++i){
        cprintf("do some thing else in init_main\n");
	do_sleep(5);
    }

    while (do_wait(0, NULL) == 0) ;

    cprintf("all user-mode processes have quit.\n");
    assert(nr_process == 2);
    assert(nr_free_pages_store == nr_free_pages());
    assert(slab_allocated_store == slab_allocated());
    cprintf("init check memory pass.\n");
    return 0;
}

// proc_init - set up the first kernel thread idleproc "idle" by itself and 
//           - create the second kernel thread init_main
void
proc_init(void) {
    int i;

    list_init(&proc_list);
    for (i = 0; i < HASH_LIST_SIZE; i ++) {
        list_init(hash_list + i);
    }

    if ((idleproc = alloc_proc()) == NULL) {
        panic("cannot alloc idleproc.\n");
    }

    idleproc->pid = 0;
    idleproc->state = PROC_RUNNABLE;
    idleproc->kstack = (uintptr_t)bootstack;
    idleproc->need_resched = 1;
    set_proc_name(idleproc, "idle");
    nr_process ++;

    current = idleproc;

    int pid = kernel_thread(init_main, NULL);
    if (pid <= 0) {
        panic("create init_main failed.\n");
    }

    initproc = find_proc(pid);
    set_proc_name(initproc, "init");

    assert(idleproc != NULL && idleproc->pid == 0);
    assert(initproc != NULL && initproc->pid == 1);
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void
cpu_idle(void) {
    while (1) {
        if (current->need_resched) {
	    //cprintf("need reschedule\n");
            //schedule();
        }
    }
}

