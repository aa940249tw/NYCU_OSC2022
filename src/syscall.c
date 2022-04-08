#include "syscall.h"
#include "utils.h"

typedef enum {SYS_GET_PID, SYS_UART_READ, SYS_UART_WRITE, 
              SYS_EXEC, SYS_FORK, SYS_EXIT, SYS_MBOX, 
              SYS_SIGRET, SYS_SIGREG, SYS_SIGNAL} SYS_ID;

inline int getpid() {
    /*
    SYS_ID id = SYS_GET_PID;
    asm volatile("mov x8, %0"::"r"(id));
    asm volatile("svc 2");
    int tid;
    asm volatile("mov %0, x0":"=r"(tid));
    return tid;
    */
    register unsigned long x8 asm("x8") = SYS_GET_PID;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x8));
    return ret;
}

inline size_t uart_read(char *buf, size_t size) {
    /*
    SYS_ID id = SYS_UART_READ;
    asm volatile("mov x8, %0"::"g"(id));
    asm volatile("mov x8, #1");
    asm volatile("svc 2");
    asm volatile("mov %0, x0":"=r"(size));
    return size;
    */
    register char *x0 asm("x0") = buf;
    register unsigned long x1 asm("r1") = size;
    register unsigned long x8 asm("x8") = SYS_UART_READ;
    register unsigned long ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x8));
    return ret;
}

inline size_t uart_write(const char *buf, size_t size) {
    /*
    SYS_ID id = SYS_UART_WRITE;
    asm volatile("mov x8, %0"::"r"(id));
    asm volatile("mov x8, #2");
    asm volatile("svc 2");
    asm volatile("mov %0, x0":"=r"(size));
    return size;
    */
    register const char *x0 asm("x0") = buf;
    register unsigned long x1 asm("r1") = size;
    register unsigned long x8 asm("x8") = SYS_UART_WRITE;
    register unsigned long ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x8));
    return ret;
}

inline void exec(const char* name, char **const argv) {
    /*
    SYS_ID id = SYS_EXEC;
    asm volatile("mov x8, %0"::"r"(id));
    */
    register const char *x0 asm("x0") = name;
    register char **const x1 asm("r1") = argv;
    register unsigned long x8 asm("x8") = SYS_EXEC;
    __asm volatile("svc #0" :: "r"(x0), "r"(x1), "r"(x8));
    return;
}

void exit() {
    SYS_ID id = SYS_EXIT;
    asm volatile("mov x8, %0"::"r"(id));
    asm volatile("svc 0");
    return;
}

inline int fork() {
    /*
    SYS_ID id = SYS_FORK;
    asm volatile("mov x8, %0"::"r"(id));
    asm volatile("svc 2");
    int tid;
    asm volatile("mov %0, x0":"=r"(tid));
    return tid;
    */
    register unsigned long x8 asm("x8") = SYS_FORK;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x8));
    return ret;
}

inline int mbox_call(unsigned char ch, unsigned int *mbox) {
    register unsigned char x0 asm("x0") = ch;
    register unsigned int *x1 asm("r1") = mbox;
    register unsigned long x8 asm("x8") = SYS_MBOX;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x8));
    return ret;
}

inline void sigreturn() {
    register unsigned long x8 asm("x8") = SYS_SIGRET;
    __asm volatile("svc #0" :: "r"(x8));
}

inline void register_posix(int SIG, void (*func)) {
    register int x0 asm("x0") = SIG;
    register unsigned long x1 asm("r1") = (uint64_t)func;
    register unsigned long x8 asm("x8") = SYS_SIGREG;
    __asm volatile("svc #0" :: "r"(x0), "r"(x1), "r"(x8));
}

inline void p_signal(int SIG, int tid) {
    register int x0 asm("x0") = SIG;
    register int x1 asm("r1") = tid;
    register unsigned long x8 asm("x8") = SYS_SIGNAL;
    __asm volatile("svc #0" :: "r"(x0), "r"(x1), "r"(x8));
}