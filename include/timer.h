#ifndef __TIMER_H__
#define __TIMER_H__

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define CORE0_INTERRUPT_SOURCE  (volatile unsigned int*)(0x40000060)
#define CORE_TIMER_CALLBACK_BUFFER_SIZE 64

#include "utils.h"

struct core_timeout {
    struct core_timeout *next;
    void (*callback)(struct core_timeout *); 
    void* argv;
    unsigned int argc;
    unsigned int timeout;
    char buf[CORE_TIMER_CALLBACK_BUFFER_SIZE];
    unsigned int size;
};

extern void svc_timer();
void get_coretime();
void core_timer_handler();
void context_sw_timer();
extern void core_timer_enable();
extern void core_timer_disable();
void add_timer(void (*)(), unsigned int, char *, unsigned int);
void sort_timer();
void set_expired_time(unsigned int);
void update_all_timeout(unsigned int);
void core_timer_print_message_callback(struct core_timeout *);
void core_timer_handler_test();
void timer_init();
void core_timer_queue_status();

#endif