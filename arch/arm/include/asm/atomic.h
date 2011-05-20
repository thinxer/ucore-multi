#ifndef __ARCH_ARM_ASM_ATOMIC_H__
#define __ARCH_ARM_ASM_ATOMIC_H__

#ifndef __LIB_ATOMIC_H__
#error can only include 'lib/atomic.h' directly.
#endif

#include <irqflags.h>

/*
 * For documentation, please refer to lib/atomic.h
 *
 * Note that atomic_t is defined there.
 */

static inline long arch_atomic_read(const atomic_t *v) __attribute__((always_inline));
static inline void arch_atomic_set(atomic_t *v, long i) __attribute__((always_inline));
static inline void arch_atomic_add(atomic_t *v, long i) __attribute__((always_inline));
static inline void arch_atomic_sub(atomic_t *v, long i) __attribute__((always_inline));
static inline bool arch_atomic_sub_test_zero(atomic_t *v, long i) __attribute__((always_inline));
static inline void arch_atomic_inc(atomic_t *v) __attribute__((always_inline));
static inline void arch_atomic_dec(atomic_t *v) __attribute__((always_inline));
static inline bool arch_atomic_inc_test_zero(atomic_t *v) __attribute__((always_inline));
static inline bool arch_atomic_dec_test_zero(atomic_t *v) __attribute__((always_inline));
static inline long arch_atomic_add_return(atomic_t *v, long i) __attribute__((always_inline));
static inline long arch_atomic_sub_return(atomic_t *v, long i) __attribute__((always_inline));

static inline long
arch_atomic_read(const atomic_t *v) {
    return v->counter;
}

static inline void
arch_atomic_set(atomic_t *v, long i) {
    v->counter = i;
}

static inline void
arch_atomic_add(atomic_t *v, long i) {
    unsigned long flags;

    raw_local_irq_save(flags);
    v->counter += i;
    raw_local_irq_restore(flags);
}

static inline void
arch_atomic_sub(atomic_t *v, long i) {
    unsigned long flags;

    raw_local_irq_save(flags);
    v->counter -= i;
    raw_local_irq_restore(flags);
}

static inline bool
arch_atomic_sub_test_zero(atomic_t *v, long i) {
    unsigned long flags;
    unsigned long val;

    raw_local_irq_save(flags);
    v->counter -= i;
    val = v->counter;
    raw_local_irq_restore(flags);

    return val == 0;
}

static inline void
arch_atomic_inc(atomic_t *v) {
    unsigned long flags;

    raw_local_irq_save(flags);
    v->counter ++;
    raw_local_irq_restore(flags);
}

static inline void
arch_atomic_dec(atomic_t *v) {
    unsigned long flags;

    raw_local_irq_save(flags);
    v->counter --;
    raw_local_irq_restore(flags);
}

static inline bool
arch_atomic_inc_test_zero(atomic_t *v) {
    unsigned long flags;
    unsigned long val;

    raw_local_irq_save(flags);
    v->counter ++;
    val = v->counter;
    raw_local_irq_restore(flags);

    return val == 0;
}

static inline bool
arch_atomic_dec_test_zero(atomic_t *v) {
    unsigned long flags;
    unsigned long val;

    raw_local_irq_save(flags);
    v->counter --;
    val = v->counter;
    raw_local_irq_restore(flags);

    return val == 0;
}

static inline long
arch_atomic_add_return(atomic_t *v, long i) {
    unsigned long flags;
    unsigned long val;

    raw_local_irq_save(flags);
    v->counter += i;
    val = v->counter;
    raw_local_irq_restore(flags);

    return val;
}

static inline long
arch_atomic_sub_return(atomic_t *v, long i) {
    return arch_atomic_add_return(v, -i);
}

#endif /* !__ARCH_ARM_ASM_ATOMIC_H__ */

