#include "thread.h"
#include "mm.h"
#include "uart.h"
#include "syscall.h"
#include "posix.h"
#include "utils.h"

extern void return_to_user();

static int thread_cnt;
struct thread_t *run_queue;
struct list_head *kill_queue;

extern void core_timer_enable();

void init_thread() {
    thread_cnt = 0;
    run_queue = (struct thread_t *)kmalloc(sizeof(struct thread_t));
    run_queue->status = IDLE;
    run_queue->id = thread_cnt++;
    run_queue->id = 100;
    run_queue->kernel_stack = 0x80000;
    run_queue->user_stack = 0x60000;
    run_queue->context.spel1 = run_queue->kernel_stack;
    run_queue->context.spel0 = run_queue->user_stack;
    init_posix(&(run_queue->posix));
    INIT_LIST_HEAD(&(run_queue->list));
    INIT_LIST_HEAD(kill_queue);
    asm volatile("msr tpidr_el1, %0"::"r"(run_queue));
    core_timer_enable();
}

struct thread_t *Thread (void (*func)) {
    struct thread_t *new = (struct thread_t *)kmalloc(sizeof(struct thread_t));
    new->user_stack = (uint64_t)kmalloc(THREAD_SIZE) + THREAD_SIZE;
    new->kernel_stack = (uint64_t)kmalloc(THREAD_SIZE) + THREAD_SIZE;
    new->context.spel1 = new->kernel_stack;
    new->context.spel0 = new->user_stack;
    new->context.fp = new->context.spel0;
    new->context.lr = (uint64_t)func;
    new->id = thread_cnt++;
    new->status = RUN;
    init_posix(&(new->posix));
    list_add_tail(&(new->list), &(run_queue->list));
    return new;
}

void idle() {
    kill_zombies();
    schedule(0);
}

void kill_zombies() {
    struct list_head *h = kill_queue, *tmp;
    while(h->next != h) {
        tmp = h->next;
        list_del(tmp);
        struct thread_t *t = container_of(tmp, struct thread_t, list);
        kfree((void *)t->user_stack - THREAD_SIZE);
        kfree((void *)t->kernel_stack - THREAD_SIZE);
        free_posix_stack(&(t->posix));
        kfree(t);
        thread_cnt--;
    }
}

void schedule() {
    uint64_t cur = get_current();
    if(((struct thread_t *)cur)->status == RUN) list_add_tail(&(((struct thread_t *)cur)->list), &(run_queue->list));
    else if(((struct thread_t *)cur)->status == ZONBIE) list_add_tail(&(((struct thread_t *)cur)->list), kill_queue);
    struct list_head *tmp = NULL;
    list_for_each(tmp, &(run_queue->list)) {
        if(container_of(tmp, struct thread_t, list)->status == RUN) break;
    } 
    if(tmp == &(run_queue->list));
    else list_del(tmp);
    struct thread_t *t = container_of(tmp, struct thread_t, list);
    switch_to((struct task_context *)cur, (struct task_context *)t);
    //check_posix(t);
}

extern void exit();
extern void run_posix(uint64_t, uint64_t, uint64_t);
extern void sigreturn();

void check_posix(struct thread_t *t) {
    struct posix_t *p = &(t->posix);
    for(int i = 0; i < MAX_SIG; i++) {
        if(p->signal & (1 << i)) {
            p->signal &= ~(1 << i);
            if(p->signal_handler[i] && p->signal_handler[i] != (uint64_t)exit) 
                run_posix(p->signal_handler[i], p->sig_sp, (uint64_t)sigreturn);
            else if(p->signal_handler[i] == (uint64_t)exit) {
                printf("Kill %d %d: Using default handler.\n", t->id, getpid());
                exit();
            }
        } 
    }
    // No signal left
    //reset_posix(p);
    //load_context((struct task_context *)get_current());
}

void save_sp(uint64_t k_sp) {
    ((struct thread_t *)get_current())->ksp = k_sp;
}

int __getpid() {
    return ((struct thread_t *)get_current())->id;
}

void __fork(uint64_t p_trapframe) {
    struct thread_t *parent = (struct thread_t *)get_current();
    struct thread_t *child = Thread((void *)(0));
    // Copy context
    for(int i = 0; i < sizeof(struct task_context); i++) *((char *)&(child->context) + i) = *((char *)&(parent->context) + i);
    // Copy user stack
    for(int i = 0; i < THREAD_SIZE; i++) *((char *)child->user_stack - i) = *((char *)parent->user_stack - i);
    // Copy kernel stack
    for(int i = 0; i < THREAD_SIZE; i++) *((char *)child->kernel_stack - i) = *((char *)parent->kernel_stack - i);
    // Set Child's lr
    child->context.lr = (uint64_t)return_to_user;
    // Set Child's sp to right place
    uint64_t k_offset = parent->kernel_stack - p_trapframe;
    uint64_t u_offset = parent->user_stack - ((struct trapframe*)p_trapframe)->spel0;
    child->context.spel0 = child->user_stack - u_offset;
    child->context.spel1 = child->kernel_stack - k_offset;
    // Change return value
    ((struct trapframe*)p_trapframe)->x[0] = child->id;
    ((struct trapframe*)(child->kernel_stack - k_offset))->x[0] = 0;
    ((struct trapframe*)(child->kernel_stack - k_offset))->spel0 = child->context.spel0;
}

void __exit() {
    struct thread_t *cur = (struct thread_t *)get_current();
    cur->status = ZONBIE;
    schedule();
}

extern void return_posix(uint64_t);

void __sigreturn() {
    struct thread_t *cur = (struct thread_t *)get_current();
    //check_posix(cur);
    return_posix(cur->ksp);
}

void __register_posix(int SIG, void (*func)) {
    struct thread_t *cur = (struct thread_t *)get_current();
    cur->posix.signal_handler[SIG] = (uint64_t)func;
}

void __p_signal(int SIG, int tid) {
    struct list_head *tmp = NULL;
    list_for_each(tmp, &(run_queue->list)) {
        if(container_of(tmp, struct thread_t, list)->id == tid) break;
    } 
    if(tmp == &(run_queue->list)) printf("No such process.\n");
    else container_of(tmp, struct thread_t, list)->posix.signal |= (1 << SIG);
} 

void foo() {
    for(int i = 0; i < 10; ++i) {
        struct thread_t *cur = (struct thread_t *)get_current();
        printf("Thread id: %d %d\n", cur->id, i);
        //delay(1000000);
        if(i == 6) exit();
        if(i == 9) cur->status = ZONBIE;
        schedule();
    }
}

void thread_test() {
    for(int i = 0; i < 5; ++i) { 
        Thread(foo);
    }
    idle();
    kill_zombies();
}

void fork_test() {
    printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    uint64_t tmp;
    if ((ret = fork()) == 0) { // child
        asm volatile("mov %0, sp": "=r"(tmp));
        printf("child1, sp: %x, pid: %d, cnt: %d, ptr: %x\n", tmp, getpid(), cnt, &cnt);
        ++cnt;
        if(ret == fork()) {
            asm volatile("mov %0, sp": "=r"(tmp));
            while (cnt < 5) {
                printf("child2, sp: %x, pid: %d, cnt: %d, ptr: %x\n", tmp, getpid(), cnt, &cnt);
                delay(1000);
                ++cnt;
            }
        }
        else {
            asm volatile("mov %0, sp": "=r"(tmp));
            printf("child1, sp: %x, pid: %d, cnt: %d, ptr: %x\n", tmp, getpid(), cnt, &cnt);
        }
        exit();
    } else {
        asm volatile("mov %0, sp": "=r"(tmp));
        printf("sp: %x\n", tmp);
        printf("parent here, pid %d, child %d, cnt: %d, ptr: %x\n", getpid(), ret, ++cnt, &cnt);
    }
}

void play_video() {
    int ret = 0;
    if((ret = fork()) == 0) {
        char *argv[] = {"syscall.img"};
        exec("syscall.img", argv);
    }
    else {
        printf("\nFork Test, pid %d\n", getpid());
    }
}

void kill_print() {
    printf("Kill me if you can BITCH!!!!\n");

}

void posix_test() {
    int ret = 0;
    if((ret = fork()) == 0) {
        if((ret = fork()) == 0) {
            //register_posix(SIGKILL, kill_print);
            while(1) ;
        }
        else {
            while(1) {
                //p_signal(SIGKILL, 2);
                //printf("%d\n", getpid());
            }
        }
    }
    else {
        idle();
    }
}