#ifndef __LIB_STDLIB_H__
#define __LIB_STDLIB_H__

#include <types.h>

/* the largest number rand will return */
#define RAND_MAX    2147483647UL

/* lib/rand.c */
int rand(void);
void srand(unsigned int seed);

/* lib/hash.c */
uint32_t
hash32(uint32_t val, unsigned int bits);

#endif /* !__LIB_RAND_H__ */

