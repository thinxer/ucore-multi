#include <console.h>

#define UFCON0  ((volatile unsigned int *)(0x50000020))

static void
uart_putc(int c) {
    *UFCON0 = c;
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
