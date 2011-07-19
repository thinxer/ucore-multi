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
    return;
    ticks = 0;

	//intr_umask(14);
}
