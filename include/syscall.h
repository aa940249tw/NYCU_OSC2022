#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "utils.h"

int getpid();
size_t uart_read(char *, size_t);
size_t uart_write(const char *, size_t);
void exec(const char* name, char **const argv);
void exit();
int fork();
int mbox_call(unsigned char, unsigned int *);
void sigreturn();
void kill(int, int);
void signal(int, void (*));

int open(const char *pathname, int flags);
int close(int fd);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
int mkdir(const char *pathname, int mode);
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);

#endif