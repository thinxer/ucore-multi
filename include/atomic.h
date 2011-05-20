#ifndef __LIB_ATOMIC_H__
#define __LIB_ATOMIC_H__

/* Atomic operations that C can't guarantee us. Useful for resource counting etc.. */

typedef struct {
    volatile long counter;
} atomic_t;

// Include arch dependent declaration
#include <arch/asm/atomic.h>

/* *
 * atomic_read - read atomic variable
 * @v:  pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 * */
#define atomic_read(v)              arch_atomic_read(v)

/* *
 * atomic_set - set atomic variable
 * @v:  pointer of type atomic_t
 * @i:  required value
 *
 * Atomically sets the value of @v to @i.
 * */
#define atomic_set(v, i)            arch_atomic_set(v, i)

/* *
 * atomic_add - add integer to atomic variable
 * @v:  pointer of type atomic_t
 * @i:  integer value to add
 *
 * Atomically adds @i to @v.
 * */
#define atomic_add(v, i)            arch_atomic_add(v, i)

/* *
 * atomic_sub - subtract integer from atomic variable
 * @v:  pointer of type atomic_t
 * @i:  integer value to subtract
 *
 * Atomically subtracts @i from @v.
 * */
#define atomic_sub(v, i)            arch_atomic_sub(v, i)

/* *
 * atomic_sub_test_zero - subtract value from variable and test result
 * @v:  pointer of type atomic_t
 * @i:  integer value to subtract
 *
 * Atomically subtracts @i from @v and
 * returns true if the result is zero, or false for all other cases.
 * */
#define atomic_sub_test_zero(v, i)  arch_atomic_sub_test_zero(v, i)

/* *
 * atomic_inc - increment atomic variable
 * @v:  pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 * */
#define atomic_inc(v)               arch_atomic_inc(v)

/* *
 * atomic_dec - decrement atomic variable
 * @v:  pointer of type atomic_t
 *
 * Atomically decrements @v by 1.
 * */
#define atomic_dec(v)               arch_atomic_dec(v)

/* *
 * atomic_inc_test_zero - increment and test
 * @v:  pointer of type atomic_t
 *
 * Atomically increments @v by 1 and
 * returns true if the result is zero, or false for all other cases.
 * */
#define atomic_inc_test_zero(v)     arch_atomic_inc_test_zero(v)

/* *
 * atomic_dec_test_zero - decrement and test
 * @v:  pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other cases.
 * */
#define atomic_dec_test_zero(v)     arch_atomic_dec_test_zero(v)

/* *
 * atomic_add_return - add integer and return
 * @i:  integer value to add
 * @v:  pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 * Requires Modern 486+ processor
 * */

#define atomic_add_return(v, i)     arch_atomic_add_return(v, i)

/* *
 * atomic_sub_return - subtract integer and return
 * @v:  pointer of type atomic_t
 * @i:  integer value to subtract
 *
 * Atomically subtracts @i from @v and returns @v - @i
 * */
#define atomic_sub_return(v, i)     arch_atomic_sub_return(v, i)


#endif /* !__LIB_ATOMIC_H__ */
