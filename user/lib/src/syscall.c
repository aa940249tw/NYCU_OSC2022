#include "syscall.h"
#include <stdint.h>

int getpid() {
  asm volatile("mov x8, #0");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

size_t uartread(char buf[], size_t size) {
  asm volatile("mov x8, #1");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

size_t uartwrite(const char buf[], size_t size) {
  asm volatile("mov x8, #2");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

int exec(const char *name, char *const argv[]) {
  asm volatile("mov x8, #3");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

int fork() {
  asm volatile("mov x8, #4");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

void exit(int status) {
  asm volatile("mov x8, #5");
  asm volatile("svc 0");
  __builtin_unreachable();
}

int mbox_call(unsigned char ch, unsigned int *mbox) {
  asm volatile("mov x8, #6");
  asm volatile("svc 0");
  uint64_t result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}

void kill(int pid) {
  asm volatile("mov x8, #7");
  asm volatile("svc 0");
}

void register_posix(int, void(*)) {
  asm volatile("mov x8, #8");
  asm volatile("svc 0");
}

void p_signal(int, int) {
  asm volatile("mov x8, #9");
  asm volatile("svc 0");
}

void *mmap(void* addr, unsigned long len, int prot, int flags) {
  asm volatile("mov x8, #10");
  asm volatile("svc 0");
  void *result;
  asm volatile("mov %0, x0" : "=r"(result)::);
  return result;
}
