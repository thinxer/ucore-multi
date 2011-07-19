#include <types.h>
#include <assert.h>
#include <bitops.h>

typedef volatile unsigned long lock_t;

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
    // deadlock
    while (!try_lock(lock));
}

static inline void
unlock(lock_t *lock) {
    if (!test_and_clear_bit(0, lock)) {
        panic("Unlock failed.\n");
    }
}

