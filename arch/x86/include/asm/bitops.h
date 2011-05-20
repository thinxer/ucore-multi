#ifndef __ASM_X86_BITOPS_H
#define __ASM_X86_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <lib/bitops.h> can be included directly
#endif

#include <irqflags.h>
#include <bitops/non-atomic.h>

static inline void set_bit(int nr, volatile void *addr) __attribute__((always_inline));
static inline void clear_bit(int nr, volatile void *addr) __attribute__((always_inline));
static inline void change_bit(int nr, volatile void *addr) __attribute__((always_inline));
static inline bool test_and_set_bit(int nr, volatile void *addr) __attribute__((always_inline));
static inline bool test_and_clear_bit(int nr, volatile void *addr) __attribute__((always_inline));
static inline bool test_and_change_bit(int nr, volatile void *addr) __attribute__((always_inline));

/* *
 * set_bit - Atomically set a bit in memory
 * @nr:     the bit to set
 * @addr:   the address to start counting from
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 * */
static inline void
set_bit(int nr, volatile void *addr) {
    asm volatile ("btsl %1, %0" :"=m" (*(volatile long *)addr) : "Ir" (nr));
}

/* *
 * clear_bit - Atomically clears a bit in memory
 * @nr:     the bit to clear
 * @addr:   the address to start counting from
 * */
static inline void
clear_bit(int nr, volatile void *addr) {
    asm volatile ("btrl %1, %0" :"=m" (*(volatile long *)addr) : "Ir" (nr));
}

/* *
 * change_bit - Atomically toggle a bit in memory
 * @nr:     the bit to change
 * @addr:   the address to start counting from
 * */
static inline void
change_bit(int nr, volatile void *addr) {
    asm volatile ("btcl %1, %0" :"=m" (*(volatile long *)addr) : "Ir" (nr));
}

/* *
 * test_and_set_bit - Atomically set a bit and return its old value
 * @nr:     the bit to set
 * @addr:   the address to count from
 * */
static inline bool
test_and_set_bit(int nr, volatile void *addr) {
    int oldbit;
    asm volatile ("btsl %2, %1; sbbl %0, %0" : "=r" (oldbit), "=m" (*(volatile long *)addr) : "Ir" (nr) : "memory");
    return oldbit != 0;
}

/* *
 * test_and_clear_bit - Atomically clear a bit and return its old value
 * @nr:     the bit to clear
 * @addr:   the address to count from
 * */
static inline bool
test_and_clear_bit(int nr, volatile void *addr) {
    int oldbit;
    asm volatile ("btrl %2, %1; sbbl %0, %0" : "=r" (oldbit), "=m" (*(volatile long *)addr) : "Ir" (nr) : "memory");
    return oldbit != 0;
}

/* *
 * test_and_change_bit - Atomically change a bit and return its old value
 * @nr:     the bit to change
 * @addr:   the address to count from
 * */
static inline bool
test_and_change_bit(int nr, volatile void *addr) {
    int oldbit;
    asm volatile ("btcl %2, %1; sbbl %0, %0" : "=r" (oldbit), "=m" (*(volatile long *)addr) : "Ir" (nr) : "memory");
    return oldbit != 0;
}

#endif
