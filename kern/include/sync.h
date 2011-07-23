#ifndef __KERN_SYNC_SYNC_H__
#define __KERN_SYNC_SYNC_H__

#include <intr.h>
#include <assert.h>
#include <atomic.h>
#include <sched.h>

#define local_intr_save(x)      do { arch_local_irq_save(x); } while (0)
#define local_intr_restore(x)   do { arch_local_irq_restore(x); } while (0)

typedef volatile long unsigned int lock_t;

static inline void
lock_init(lock_t *lock) {
    *lock = 0;
}

static inline bool
try_lock(lock_t *lock) {
    return !test_and_set_bit(0, lock);
}

static inline void
lock(lock_t *lock) {
    while (!try_lock(lock)) {
        schedule();
    }
}

static inline void
unlock(lock_t *lock) {
    if (!test_and_clear_bit(0, lock)) {
        panic("Unlock failed.\n");
    }
}

#endif /* !__KERN_SYNC_SYNC_H__ */
