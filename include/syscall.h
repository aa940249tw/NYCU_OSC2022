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
void p_signal(int, int);
void register_posix(int, void (*));

#endif