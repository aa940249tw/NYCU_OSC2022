#include "syscall.h"
#include "utils.h"

typedef enum {SYS_GET_PID, SYS_UART_READ, SYS_UART_WRITE, 
              SYS_EXEC, SYS_FORK, SYS_EXIT, SYS_MBOX, 
              SYS_SIGRET, SYS_SIGREG, SYS_SIGNAL, SYS_MMAP,
              SYS_OPEN, SYS_CLOSE, SYS_WRITE, SYS_READ,
              SYS_MKDIR, SYS_MOUNT, SYS_CHDIR, SYS_LSEEK} SYS_ID;

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

inline void signal(int SIG, void (*func)) {
    register int x0 asm("x0") = SIG;
    register unsigned long x1 asm("r1") = (uint64_t)func;
    register unsigned long x8 asm("x8") = SYS_SIGREG;
    __asm volatile("svc #0" :: "r"(x0), "r"(x1), "r"(x8));
}

inline void kill(int tid, int SIG) {
    register int x0 asm("x0") = tid;
    register int x1 asm("r1") = SIG;
    register unsigned long x8 asm("x8") = SYS_SIGNAL;
    __asm volatile("svc #0" :: "r"(x0), "r"(x1), "r"(x8));
}

inline void *mmap(void* addr, size_t len, int prot, int flags) {
    register void *x0 asm("x0") = addr;
    register size_t x1 asm("x1") = len;
    register int x2 asm("x2") = prot;
    register int x3 asm("x3") = flags;
    register void *ret asm("x0");
    register unsigned long x8 asm("x8") = SYS_MMAP;
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x8));
}

inline int open(const char *pathname, int flags) {
    register const char *x0 asm("x0") = pathname;
    register int x1 asm("x1") = flags;
    register unsigned long x8 asm("x8") = SYS_OPEN;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x8));
    return ret;
}

inline int close(int fd) {
    register int x0 asm("x0") = fd;
    register unsigned long x8 asm("x8") = SYS_CLOSE;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x8));
    return ret;
}

inline int write(int fd, const void *buf, int count) {
    register int x0 asm("x0") = fd;
    register const void *x1 asm("x1") = buf;
    register int x2 asm("x2") = count;
    register int ret asm("w0");
    register unsigned long x8 asm("x8") = SYS_WRITE;
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x2), "r"(x8));
    return ret;
}

inline int read(int fd, void *buf, int count) {
    register int x0 asm("x0") = fd;
    register void *x1 asm("x1") = buf;
    register int x2 asm("x2") = count;
    register int ret asm("w0");
    register unsigned long x8 asm("x8") = SYS_READ;
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x2), "r"(x8));
    return ret;
}

inline int mkdir(const char *pathname, int mode) {
    register const char *x0 asm("x0") = pathname;
    register int x1 asm("x1") = mode;
    register unsigned long x8 asm("x8") = SYS_MKDIR;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x8));
    return ret;
}

inline int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data) {
    register const char *x0 asm("x0") = src;
    register const char *x1 asm("x1") = target;
    register const char *x2 asm("x2") = filesystem;
    register unsigned long x3 asm("x3") = flags;
    register const void *x4 asm("x4") = data;
    register unsigned long x8 asm("x8") = SYS_MKDIR;
    register int ret asm("w0");
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8));
    return ret;
}

inline long lseek64(int fd, long offset, int whence) {
    register int x0 asm("x0") = fd;
    register long x1 asm("x1") = offset;
    register int x2 asm("x2") = whence;
    register long ret asm("x0");
    register unsigned long x8 asm("x8") = SYS_LSEEK;
    __asm volatile("svc #0" : "=r"(ret) : "r"(x0), "r"(x1), "r"(x2), "r"(x8));
    return ret;
}