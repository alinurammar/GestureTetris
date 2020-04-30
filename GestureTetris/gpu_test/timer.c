#include "timer.h"

static volatile unsigned int *TIME  =  (unsigned int *) 0x20003004;

void timer_init(void) {
}

unsigned int timer_get_ticks(void) {
    return *TIME;
}

void timer_delay_us(unsigned int usecs) {
    unsigned int start = timer_get_ticks();
    while (timer_get_ticks() - start < usecs) { /* spin */ }
}

void timer_delay_ms(unsigned int msecs) {
    timer_delay_us(1000*msecs);
}

void timer_delay(unsigned int secs) {
    timer_delay_us(1000000*secs);
}
