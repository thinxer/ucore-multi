#include <clock.h>
#include <intr.h>
#include <types.h>

// TODO put the magic numbers into s3c2410.h

#define TIMER_BASE  (0x51000000)
#define TCFG0   ((volatile uint32_t *)(TIMER_BASE+0x0))
#define TCFG1   ((volatile uint32_t *)(TIMER_BASE+0x4))
#define TCON    ((volatile uint32_t *)(TIMER_BASE+0x8))
#define TCONB4  ((volatile uint32_t *)(TIMER_BASE+0x3c))

volatile size_t ticks;

void
clock_init(void) {
    ticks = 0;

    // The higher byte is a frequency divisor
	// *TCFG0|=0x800;
	*TCFG0|=0xff00;
	*TCON&=(~(7<<20));
	*TCON|=(1<<22);
	*TCON|=(1<<21);

    // XXX this counter seems not working...
	*TCONB4=10000000;

	*TCON|=(1<<20);
	*TCON&=~(1<<21);

	intr_umask(14);
}
