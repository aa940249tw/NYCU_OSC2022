#ifndef __THREAD_H__
#define __THREAD_H__

#include "list.h"
#include "utils.h"
#include "posix.h"
#include "vfs.h"

#define THREAD_SIZE 4096
#define fd_size     8

struct task_context {
    uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
    uint64_t fp;  
    uint64_t lr;  
    uint64_t spel0;
    uint64_t spel1;
};

struct trapframe {
    uint64_t x[31];
    uint64_t spel0;
    uint64_t elrel1;
    uint64_t spsrel1;
};

typedef enum {IDLE, RUN, ZONBIE} STATUS;

struct thread_t {
    struct task_context context;
    int id;
    STATUS status;
    uint64_t user_stack;
    uint64_t kernel_stack;
    uint64_t m_stack;
    uint64_t ksp;
    struct posix_t posix;
    struct mm_struct *mm;
    struct list_head list;
    struct file *fd[fd_size];
};

extern void switch_to(struct task_context *, struct task_context *);
extern void load_context(struct task_context *);
extern void switch_ttbr0(unsigned long);
extern uint64_t get_current(); 
struct thread_t *Thread(void (*));
void init_thread();
void kill_zombies();
void schedule();
void idle();
void check_posix(struct thread_t *);
void save_sp(uint64_t);
int get_empty_fd(struct thread_t *);

int __getpid();
void __fork(uint64_t);
void __exit();
void __sigreturn();
void __signal(int, void (*));
void __kill(int, int);
void __exec(char *, char **);

void thread_test();
void fork_test();
void play_video();
void posix_test();

#endif