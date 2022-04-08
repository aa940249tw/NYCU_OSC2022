#include "irq.h"
#include "uart.h"
#include "timer.h"
#include "utils.h"

struct irq_task *pending_head = 0;

void enable_interrupt() { 
    asm volatile("msr DAIFClr, 0xf"); 
}

void disable_interrupt() { 
    asm volatile("msr DAIFSet, 0xf"); 
}

void irq_handler(int type, unsigned long esr, unsigned long elr) {
    if(*IRQ_BASIC_PENDING & AUX_INT) {
        uart_handler();
    }
    else if(*CORE0_INTERRUPT_SOURCE & 0x2) {
        context_sw_timer();
        //core_timer_handler();
    }
    else {
        printf("Exception eccurs!!!\n");
        if(type == 9)
            printf("Excpetion type:  irq_el64\tESR:  0x%x\tELR:  0x%x\n", esr, elr);
        else
            printf("Excpetion type:  irq_sp_elx\tESR:  0x%x\tELR:  0x%x\n", esr, elr);
    }
}

void run_task(struct irq_task *t) {
    while(t) {
        t->task(t->data);
        pending_head = pending_head->list;
        if(t->preempt) break;
        t = t->list;
    }
}

void add_task(void (*task)(void *), void *data, int priority) {
    struct irq_task *new = (struct irq_task *)simple_malloc((unsigned long)sizeof(struct irq_task));
    new->task = task;
    new->data = data;
    new->priority = priority;
    new->list = 0;
    new->preempt = 0;
    if(pending_head == 0) {
        pending_head = new;
        enable_interrupt();
        run_task(new);
        disable_interrupt();
    }
    else if(new->priority < pending_head->priority) {
        new->preempt = 1;
        new->list = pending_head;
        pending_head = new;
        enable_interrupt();
        run_task(new);
        disable_interrupt();
    }
    else {
        struct irq_task *tmp = pending_head;
        while(tmp->list) {
            if(tmp->list->priority > new->priority) break;
            tmp = tmp->list;
        }
        new->list = tmp->list;
        tmp->list = new;
    }
}

