#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "utils.h"
#include "thread.h"
#include "initrd.h"
#include "mbox.h"
#include "posix.h"
#include "mm.h"

char *exception_type[] = {
    "synchronous_sp_el0",
    "irq_sp_el0        ",
    "fiq_sp_el0        ",
    "serror_sp_el0     ",
    "synchronous_sp_elx",
    "irq_sp_elx        ",
    "fiq_sp_elx        ",
    "serror_sp_elx     ",
    "synchronous_el64  ",
    "irq_el64          ",
    "fiq_el64          ",
    "serror_el64       ",
    "synchronous_el32  ",
    "irq_el32          ",
    "fiq_el32          ",
    "serror_el32       "
};

void exception_entry(int type, unsigned long esr, unsigned long elr)
{
    printf("Exception eccurs!!!\n");
    printf("Excpetion type:  %s\tESR:  0x%x\tELR:  0x%x\n", exception_type[type], esr, elr);
}

void svc_handler(int type, unsigned long esr, unsigned long elr, uint64_t trapframe) {
    int ecr = (esr >> 26);
    if(ecr == 0x15) {   // svc system_call
        int svc_type = esr & ((1 << 24) - 1);
        switch (svc_type) {
            case 2:
                printf("Exception eccurs!!!\n");
                printf("Excpetion type:  %s\tESR:  0x%x\tELR:  0x%x\n", exception_type[type], esr, elr);
                break;
            case 1: 
                ;
                int time = 0;
                unsigned int count, freq;
                asm volatile("mrs  %[result], cntpct_el0": [result]"=r"(count));
                asm volatile("mrs  %[result], cntfrq_el0": [result]"=r"(freq));
                time = (1000 * count) / freq;
                printf("[%2d] core timer interrupt\n", time);
                break;
            // syscall
            case 0:
                ;
                uint64_t syscall;
                asm volatile("mov %0, x8" : "=r"(syscall));
                switch (syscall) {
                    case 0:
                        ;
                        ((struct trapframe *)trapframe)->x[0] = __getpid();
                        break;
                    case 1: //uart_read
                        ;
                        char* buf0 = (char *)((struct trapframe *)trapframe)->x[0];
                        size_t size0 = (size_t)((struct trapframe *)trapframe)->x[1];
                        asm volatile("msr DAIFClr, 0xf"); 
                        for (size_t i = 0; i < size0; i++) buf0[i] = uart_getc();
                        buf0[size0] = '\0';
                        ((struct trapframe *)trapframe)->x[0] = size0;
                        asm volatile("msr DAIFSet, 0xf"); 
                        break;
                    case 2: //uart_write
                        ;
                        char* buf = (char *)((struct trapframe *)trapframe)->x[0];
                        size_t size = (size_t)((struct trapframe *)trapframe)->x[1];
                        asm volatile("msr DAIFClr, 0xf"); 
                        for (size_t i = 0; i < size; i++) uart_send(buf[i]);
                        ((struct trapframe *)trapframe)->x[0] = size;
                        asm volatile("msr DAIFSet, 0xf"); 
                        break;
                    case 3:
                        __exec((char *)((struct trapframe *)trapframe)->x[0], (char **)((struct trapframe *)trapframe)->x[1]);
                        break;
                    case 4:
                        __fork(trapframe);
                        break;
                    case 5:
                        __exit();
                        break;
                    case 6:
                        struct thread_t *cur = (struct thread_t *)get_current();
                        unsigned int *mbox_addr = (unsigned int *)(((struct trapframe *)trapframe)->x[1] - 0xffffffffe000 + cur->m_stack);
                        ((struct trapframe *)trapframe)->x[0] = __mbox_call((unsigned char)((struct trapframe *)trapframe)->x[0], mbox_addr);
                        break;
                    case 7:
                        __sigreturn();
                        break;
                    case 8:
                        __signal(((struct trapframe *)trapframe)->x[0], (void (*))((struct trapframe *)trapframe)->x[1]);
                        break;
                    case 9:
                        __kill(((struct trapframe *)trapframe)->x[0], ((struct trapframe *)trapframe)->x[1]);
                        break;
                    case 10:
                        struct trapframe *t = (struct trapframe *)trapframe;
                        uint64_t ret = __mmap(t->x[0], t->x[1], t->x[2], t->x[3]); 
                        t->x[0] = ret;
                        break;
            }
        }
    }
    else if(ecr == 0x20 || ecr == 0x21 || ecr == 0x24 || ecr == 0x25) { //data abort in EL0, ELX; instruction abort in EL0, ELX
        unsigned long fault_addr;
        asm volatile("mrs %0, far_el1":"=r"(fault_addr));
        mem_abort_handler(esr, fault_addr);
    }
    else {
        printf("Not Handled Exception eccurs!!!\n");
        printf("Excpetion type:  %s\tESR:  0x%x\tELR:  0x%x\n", exception_type[type], esr, elr);
    }
}