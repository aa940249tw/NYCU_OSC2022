#ifndef SYSCALL_H
#define SYSCALL_H
#include <stddef.h>
#include <stdnoreturn.h>

int getpid();
size_t uartread(char buf[], size_t size);
size_t uartwrite(const char buf[], size_t size);
int exec(const char *name, char *const argv[]);
int fork();
void exit(int status);
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);
void register_posix(int, void(*));
void p_signal(int, int);
void *mmap(void* addr, unsigned long len, int prot, int flags);
#define O_CREAT 0100
int open(const char *pathname, int flags);
int close(int fd);
long write(int fd, const void *buf, size_t count);
long read(int fd, void *buf, size_t count);
int mkdir(const char *pathname, unsigned mode);
int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);
int chdir(const char *path);
#define SEEK_SET 0
long lseek64(int fd, long offset, int whence);
int ioctl(int fd, unsigned long request, ...);

#endif
