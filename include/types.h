#ifndef __LIB_TYPES_H__
#define __LIB_TYPES_H__

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __ASSEMBLER__

/* Represents true-or-false values */
typedef int bool;

/* Explicitly-sized versions of integer types */
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

/* *
 * Pointers and addresses are 32 bits long.
 * We use pointer types to represent addresses,
 * uintptr_t to represent the numerical values of addresses.
 * */
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

/* size_t is used for memory object sizes */
typedef uintptr_t size_t;

/* used for page numbers */
typedef size_t ppn_t;

/* *
 * Rounding operations (efficient when n is a power of 2)
 * Round down to the nearest multiple of n
 * */
#define ROUNDDOWN(a, n) ({                                          \
            size_t __a = (size_t)(a);                               \
            (typeof(a))(__a - __a % (n));                           \
        })

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n) ({                                            \
            size_t __n = (size_t)(n);                               \
            (typeof(a))(ROUNDDOWN((size_t)(a) + __n - 1, __n));     \
        })

/**
 * @return the offset of 'member' relative to the beginning of a struct type
 */
#define offsetof(type, member)                                      \
    ((size_t)(&((type *)0)->member))

/**
 * to_struct - get the struct from a ptr
 * @param ptr       a struct pointer of member
 * @param type      the type of the struct this is embedded in
 * @param member    the name of the member within the struct
 */
#define to_struct(ptr, type, member)                               \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif /* !__ASSEMBLER__ */

#define BITS_PER_LONG (sizeof(long)*8)

#endif /* !__LIBS_TYPES_H__ */

