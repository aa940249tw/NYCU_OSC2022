#include "timer.h"
#include "uart.h"
#include "utils.h"

void timer_init() {
    timeout_head = 0;
}

void core_timer_print_message_callback(struct core_timeout *q) {
    printf("Message: %s\t", q->buf);
    get_coretime();
    uart_puts("\r# ");
}

void get_coretime() {
    unsigned int count, freq;
    int time = 0;
    asm volatile("mrs  %[result], cntpct_el0": [result]"=r"(count));
    asm volatile("mrs  %[result], cntfrq_el0": [result]"=r"(freq));
    time = (1000 * count) / freq;
    printf("[%2d] core timer interrupt\n", time);
}

void core_timer_handler() {
    struct core_timeout *q = timeout_head;
    q->callback(q);
    if(timeout_head->next != 0) {
        timeout_head = timeout_head->next;
        update_all_timeout(q->timeout);
        set_expired_time(timeout_head->timeout);
    }
    else {
        timeout_head = 0;
        core_timer_disable();
    }
}

void core_timer_handler_test() {
    asm volatile("mrs x0, cntfrq_el0\n" "msr cntp_tval_el0, x0\n");
    asm volatile("mrs     x0, cntfrq_el0\n"
                 "lsl     x0, x0, #1\n"
                 "msr     cntp_tval_el0, x0\n");
    get_coretime();
}

void set_expired_time(unsigned int duration) {
    unsigned long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    asm volatile("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0 * duration));
}

void update_all_timeout(unsigned int time) {
    struct core_timeout *q = timeout_head;
    while(q != 0) {
        q->timeout = q->timeout - time;
        q = q->next;
    }
}

void sort_timer() {
    struct core_timeout *ptr, *prev_ptr, *next_ptr;
    ptr = timeout_head;
    prev_ptr = 0;

    while(ptr != 0) {
        if(ptr->next != 0) {
            next_ptr = ptr->next;
            if(ptr->timeout > next_ptr->timeout) {
                ptr->next = next_ptr->next;
                next_ptr->next = ptr;
                if(prev_ptr != 0) 
                    prev_ptr->next = next_ptr;
                else 
                    timeout_head = next_ptr;
                ptr = next_ptr;
            }
        }
        prev_ptr = ptr;
        ptr = ptr->next;  
    }
}

void add_timer(void (*callback)(), unsigned int timeout, char *message, unsigned int size) {
    struct core_timeout *q = (struct core_timeout *)simple_malloc((unsigned long)sizeof(struct core_timeout));
    q->callback = callback;
    q->timeout = timeout;
    q->argc = 0;
    q->argv = 0;
    q->size = size;
    strncpy(q->buf, message, size);
    if(timeout_head == 0) {
        timeout_head = q;
        set_expired_time(timeout);
        core_timer_enable();
    }
    else {
        q->next = timeout_head;
        timeout_head = q;
        sort_timer(timeout_head);
        if(timeout_head == q) set_expired_time(timeout);
    }
    //core_timer_queue_status();
}

void core_timer_queue_status() {
    struct core_timeout *q = timeout_head;
    if(q != 0) {
        while(q != 0) {
            printf("mes: %s -> ", q->buf);   
            q = q->next;
        }
    }
    printf("null\n");
}
