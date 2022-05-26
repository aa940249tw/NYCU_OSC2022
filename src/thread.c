#include "thread.h"
#include "mm.h"
#include "uart.h"
#include "syscall.h"
#include "posix.h"
#include "utils.h"
#include "mem.h"
#include "initrd.h"
#include "dev.h"

extern void return_to_user();
extern unsigned char kernel_virt;

static int thread_cnt;
struct thread_t *run_queue;
struct list_head *kill_queue;

extern void core_timer_enable();

void init_thread() {
    thread_cnt = 0;
    run_queue = (struct thread_t *)kmalloc(sizeof(struct thread_t));
    run_queue->status = IDLE;
    kill_queue = (struct list_head *)kmalloc(sizeof(struct list_head));
    INIT_LIST_HEAD(&(run_queue->list));
    INIT_LIST_HEAD(kill_queue);
    struct thread_t *thread_i = (struct thread_t *)kmalloc(sizeof(struct thread_t));
    thread_i->status = RUN;
    thread_i->id = thread_cnt++;
    thread_i->id = 100;
    thread_i->kernel_stack = (uint64_t)&kernel_virt + 0x80000;
    //thread_i->user_stack = (uint64_t)&kernel_virt + 0x60000;
    thread_i->mm = (struct mm_struct *)kmalloc(sizeof(struct mm_struct));
    thread_i->context.spel1 = thread_i->kernel_stack;
    thread_i->context.spel0 = thread_i->user_stack;
    init_mm(thread_i->mm);
    init_posix(&(thread_i->posix));
    for(int i = 0; i < fd_size; i++) thread_i->fd[i] = NULL;
    open_uart_fds(thread_i);
    // Mailbox
    mappages((pagetable_t)thread_i->mm->pgd, 0x3c100000, 0x200000, PA2KA(0x3c100000), PT_AF | PT_USER | PT_MEM | PT_RW);
    asm volatile("msr tpidr_el1, %0"::"r"(thread_i));
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    core_timer_enable();
    // Do exec, jump to user shell
    __exec("syscall.img", NULL);
}

struct thread_t *Thread (void (*func)) {
    struct thread_t *new = (struct thread_t *)kmalloc(sizeof(struct thread_t));
    new->kernel_stack = (uint64_t)kmalloc(THREAD_SIZE) + THREAD_SIZE;
    new->mm = (struct mm_struct *)kmalloc(sizeof(struct mm_struct));
    init_mm(new->mm);
    // Mailbox
    mappages((pagetable_t)new->mm->pgd, 0x3c100000, 0x200000, PA2KA(0x3c100000), PT_AF | PT_USER | PT_MEM | PT_RW);
    new->context.spel1 = new->kernel_stack;
    new->context.spel0 = new->user_stack;
    new->context.fp = new->context.spel0;
    new->context.lr = (uint64_t)func;
    new->id = thread_cnt++;
    new->status = RUN;
    init_posix(&(new->posix));
    for(int i = 0; i < fd_size; i++) new->fd[i] = NULL;
    open_uart_fds(new);
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
        //kfree((void *)t->user_stack - THREAD_SIZE);
        kfree((void *)t->kernel_stack - THREAD_SIZE);
        //free_posix_stack(&(t->posix));
        kfree(t);
        thread_cnt--;
    }
}

int get_empty_fd(struct thread_t *t) {
    for(int i = 0; i < fd_size; i++) {
        if(t->fd[i] == NULL) return i;
    }
    return -1;
}

void print_con() {
    struct trapframe *tmp = (struct trapframe *)((struct thread_t *)get_current())->ksp;
    //for(int i = 0; i < 34; i++) printf("%x\n", *((char *)((uint64_t)tmp + i)));
    printf("%x %x\n", tmp, tmp->elrel1);
    printf("--------------------------------\n");
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
    switch_ttbr0(t->mm->pgd);
    switch_to((struct task_context *)cur, (struct task_context *)t);
    check_posix((struct thread_t *)get_current());
}

extern void exit();
extern void run_posix(uint64_t, uint64_t, uint64_t);
extern void sigreturn();

void check_posix(struct thread_t *t) {
    struct posix_t *p = &(t->posix);
    for(int i = 0; i < MAX_SIG; i++) {
        if(p->signal & (1 << i)) {
            p->signal &= ~(1 << i);
            if(p->signal_handler[i] && p->signal_handler[i] != (uint64_t)exit) {
                p->masked = true;
                // Map posix stack
                p->sig_sp = (uint64_t) kmalloc(4096);
                mappages((pagetable_t)t->mm->pgd, POSIX_SP, 4096, t->posix.sig_sp, PT_AF | PT_USER | PT_MEM | PT_RW);
                run_posix(p->signal_handler[i], POSIX_SP + 0x1000, (uint64_t)sigreturn);
            }
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
    struct thread_t *cur = (struct thread_t *)get_current();
    if(!cur->posix.masked) cur->ksp = k_sp;
}

int __getpid() {
    return ((struct thread_t *)get_current())->id;
}

void __fork(uint64_t p_trapframe) {
    struct thread_t *parent = (struct thread_t *)get_current();
    struct thread_t *child = Thread((void *)(0));
    // Copy context
    for(int i = 0; i < sizeof(struct task_context); i++) *((char *)&(child->context) + i) = *((char *)&(parent->context) + i);
    // Copy user stacks
    //memcpy((void *)(child->user_stack - THREAD_SIZE), (void *)(parent->user_stack - THREAD_SIZE), THREAD_SIZE);
    // Copy kernel stack
    memcpy((void *)(child->kernel_stack - THREAD_SIZE), (void *)(parent->kernel_stack - THREAD_SIZE), THREAD_SIZE);
    // Copy POSIX
    copy_posix(&(parent->posix), &(child->posix));
    // Set Child's lr
    child->context.lr = (uint64_t)return_to_user;
    // Use the same code section 
    //child->mm->start_code = parent->mm->start_code;
    //child->mm->end_code = parent->mm->end_code;
    //mappages((pagetable_t)child->mm->pgd, USER_TEXT, child->mm->end_code - child->mm->start_code, child->mm->start_code, PT_AF | PT_USER | PT_MEM | PT_RW);
    // Set Child's sp to right place
    uint64_t k_offset = parent->kernel_stack - p_trapframe;
    //uint64_t u_offset = parent->user_stack - ((struct trapframe*)p_trapframe)->spel0;
    //child->context.spel0 = child->user_stack - u_offset;
    child->context.spel1 = child->kernel_stack - k_offset;
    // Change return value
    ((struct trapframe*)p_trapframe)->x[0] = child->id;
    ((struct trapframe*)(child->kernel_stack - k_offset))->x[0] = 0;
    //((struct trapframe*)(child->kernel_stack - k_offset))->spel0 = child->context.spel0;

    /* Copy on write */
    copy_vma(parent->mm, child->mm);
    copy_page_table((pagetable_t)child->mm->pgd, (pagetable_t)parent->mm->pgd, 3, child->mm);
    // Mailbox
    mappages((pagetable_t)child->mm->pgd, 0x3c100000, 0x200000, PA2KA(0x3c100000), PT_AF | PT_USER | PT_MEM | PT_RW);
}

extern void to_el0();
void __exec(char *filename, char **argv) {
    struct thread_t *cur = (struct thread_t *)get_current();
    struct exec_t *execute = cpio_find(filename);
    /*
    void *user_program = kmalloc(execute->len);
    memcpy(user_program, execute->filecontext, execute->len);

    cur->mm->start_code = (unsigned long)user_program;
    cur->mm->end_code = (unsigned long)user_program + execute->len;
    mappages((pagetable_t)cur->mm->pgd, USER_TEXT, execute->len, (uint64_t)user_program, PT_AF | PT_USER | PT_MEM | PT_RW);
    */
    if(cur->mm->mmap) clear_vma(cur->mm);
    //clear_pgd(cur->mm);
    // Map text sections
    create_vma(cur->mm, USER_TEXT, PGROUNDUP(execute->len), 0, 0);   // Text vma
    int u_size = execute->len;
    for(int i = 0; i <= (execute->len / PAGE_SIZE); i++) {
        void *user_program = alloc_pages(0);
        u_size -= PAGE_SIZE;
        int m_size = (u_size > 0) ? PAGE_SIZE : (u_size + PAGE_SIZE);
        memcpy(user_program, (execute->filecontext + PAGE_SIZE * i), m_size);
        mappages((pagetable_t)cur->mm->pgd, (USER_TEXT + PAGE_SIZE * i), m_size, (uint64_t)user_program, PT_AF | PT_USER | PT_MEM | PT_RW);
    }
    // Map User stack
    create_vma(cur->mm, USER_STACK, THREAD_SIZE * 4, PROT_READ | PROT_WRITE, 0);   // Stack vma
    cur->user_stack = (uint64_t)kmalloc(THREAD_SIZE * 4) + THREAD_SIZE * 4;
    //mappages((pagetable_t)cur->mm->pgd, USER_STACK, THREAD_SIZE, cur->user_stack - THREAD_SIZE, PT_AF | PT_USER | PT_MEM | PT_RW);
    // TODO: Pass Args
    to_el0(0, 0x0000fffffffff000, cur->mm->pgd);
}

void __exit() {
    struct thread_t *cur = (struct thread_t *)get_current();
    cur->status = ZONBIE;
    schedule();
}

extern void return_posix(uint64_t);

void __sigreturn() {
    struct thread_t *cur = (struct thread_t *)get_current();
    cur->posix.masked = false;
    free_posix_stack(&cur->posix);
    //check_posix(cur);
    return_posix(cur->ksp);
}

void __signal(int SIG, void (*func)) {
    struct thread_t *cur = (struct thread_t *)get_current();
    cur->posix.signal_handler[SIG] = (uint64_t)func;
}

void __kill(int tid, int SIG) {
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
    delay(100000000);
    printf("Hi\n");
}

void posix_test() {
    int ret = 0;
    if((ret = fork()) == 0) {
        if((ret = fork()) == 0) {
            signal(SIGKILL, kill_print);
            while(1);
        }
        else {
            kill(2, SIGKILL);
            //printf("%d\n", getpid());
        }
    }
    else {
        idle();
    }
}