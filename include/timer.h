#ifndef __TIMER_H__
#define __TIMER_H__

#define CORE0_TIMER_IRQ_CTRL 0x40000040

extern void svc_timer();
void get_coretime();
void core_timer_handler();

#endif