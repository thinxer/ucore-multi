#include <console.h>

#define UST0  ((volatile unsigned int *)(0xC1000000))
#define UDT0  ((volatile unsigned int *)(0xC0000000))

static void
uart_putc(int c) {
    while (((*UST0) & 2) == 0);
    *UDT0 = c;
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
