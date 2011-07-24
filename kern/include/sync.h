#ifndef __KERN_SYNC_SYNC_H__
#define __KERN_SYNC_SYNC_H__

#include <intr.h>
#include <assert.h>
#include <atomic.h>
#include <sched.h>

static inline bool
__intr_save(void) {
	if (read_psrflags() & PSR_I) {
		return 0;
	}
	intr_disable();
	return 1;
}

static inline void
__intr_restore(bool flag) {
	if (flag) {
		intr_enable();
	}
}

#define local_intr_save(x)      do { x = __intr_save(); } while (0)
#define local_intr_restore(x)   __intr_restore(x);

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

