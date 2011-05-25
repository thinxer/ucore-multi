#ifndef __LIB_STDLIB_H__
#define __LIB_STDLIB_H__

/* the largest number rand will return */
#define RAND_MAX    2147483647UL

/* libs/rand.c */
int rand(void);
void srand(unsigned int seed);

#endif /* !__LIB_RAND_H__ */

