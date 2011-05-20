#ifndef  _DIV64_INC
#define  _DIV64_INC

#include <arch/div64.h>

#define do_div(n, base) arch_do_div(n, base)

#endif   /* ----- #ifndef _DIV64_INC  ----- */
