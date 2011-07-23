#include <console.h>

#define UST0  ((volatile unsigned int *)(0x50000010))
#define URX0  ((volatile unsigned int *)(0x50000020))
#define UTX0  ((volatile unsigned int *)(0x50000020))

static void
uart_putc(int c) {
    while (((*UST0) & 2) == 0);
    *UTX0 = c;
}

void
cons_init(void) {
    // do nothing
    return;
}

void
cons_putc(int c) {
    uart_putc(c);
}

int
cons_getc(void) {
    // TODO implement this.
    return 0;
}
