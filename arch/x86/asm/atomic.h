#ifndef __ASM_X86_ATOMIC_H__
#define __ASM_X86_ATOMIC_H__

#ifndef __LIB_ATOMIC_H__
#error can only include 'lib/atomic.h' directly.
#endif

#include <irqflags.h>

/* Atomic operations that C can't guarantee us. Useful for resource counting etc.. */

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

/* *
 * arch_atomic_read - read atomic variable
 * @v:  polonger of type atomic_t
 *
 * Atomically reads the value of @v.
 * */
static inline long
arch_atomic_read(const atomic_t *v) {
    return v->counter;
}

/* *
 * arch_atomic_set - set atomic variable
 * @v:  polonger of type atomic_t
 * @i:  required value
 *
 * Atomically sets the value of @v to @i.
 * */
static inline void
arch_atomic_set(atomic_t *v, long i) {
    v->counter = i;
}

/* *
 * arch_atomic_add - add longeger to atomic variable
 * @v:  polonger of type atomic_t
 * @i:  longeger value to add
 *
 * Atomically adds @i to @v.
 * */
static inline void
arch_atomic_add(atomic_t *v, long i) {
    asm volatile ("addl %1, %0" : "+m" (v->counter) : "ir" (i));
}

/* *
 * arch_atomic_sub - subtract longeger from atomic variable
 * @v:  polonger of type atomic_t
 * @i:  longeger value to subtract
 *
 * Atomically subtracts @i from @v.
 * */
static inline void
arch_atomic_sub(atomic_t *v, long i) {
    asm volatile("subl %1, %0" : "+m" (v->counter) : "ir" (i));
}

/* *
 * arch_atomic_sub_test_zero - subtract value from variable and test result
 * @v:  polonger of type atomic_t
 * @i:  longeger value to subtract
 *
 * Atomically subtracts @i from @v and
 * returns true if the result is zero, or false for all other cases.
 * */
static inline bool
arch_atomic_sub_test_zero(atomic_t *v, long i) {
    unsigned char c;
    asm volatile("subl %2, %0; sete %1" : "+m" (v->counter), "=qm" (c) : "ir" (i) : "memory");
    return c != 0;
}

/* *
 * arch_atomic_inc - increment atomic variable
 * @v:  polonger of type atomic_t
 *
 * Atomically increments @v by 1.
 * */
static inline void
arch_atomic_inc(atomic_t *v) {
    asm volatile("incl %0" : "+m" (v->counter));
}

/* *
 * arch_atomic_dec - decrement atomic variable
 * @v:  polonger of type atomic_t
 *
 * Atomically decrements @v by 1.
 * */
static inline void
arch_atomic_dec(atomic_t *v) {
    asm volatile("decl %0" : "+m" (v->counter));
}

/* *
 * arch_atomic_inc_test_zero - increment and test
 * @v:  polonger of type atomic_t
 *
 * Atomically increments @v by 1 and
 * returns true if the result is zero, or false for all other cases.
 * */
static inline bool
arch_atomic_inc_test_zero(atomic_t *v) {
    unsigned char c;
    asm volatile("incl %0; sete %1" : "+m" (v->counter), "=qm" (c) :: "memory");
    return c != 0;
}

/* *
 * arch_atomic_dec_test_zero - decrement and test
 * @v:  polonger of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other cases.
 * */
static inline bool
arch_atomic_dec_test_zero(atomic_t *v) {
    unsigned char c;
    asm volatile("decl %0; sete %1" : "+m" (v->counter), "=qm" (c) :: "memory");
    return c != 0;
}

/* *
 * arch_atomic_add_return - add longeger and return
 * @i:  longeger value to add
 * @v:  polonger of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 * Requires Modern 486+ processor
 * */
static inline long
arch_atomic_add_return(atomic_t *v, long i) {
    long __i = i;
    asm volatile("xaddl %0, %1" : "+r" (i), "+m" (v->counter) :: "memory");
    return i + __i;
}

/* *
 * arch_atomic_sub_return - subtract longeger and return
 * @v:  polonger of type atomic_t
 * @i:  longeger value to subtract
 *
 * Atomically subtracts @i from @v and returns @v - @i
 * */
static inline long
arch_atomic_sub_return(atomic_t *v, long i) {
    return arch_atomic_add_return(v, -i);
}

#endif
