#include "posix.h"
#include "utils.h"
#include "mm.h"
#include "uart.h"

extern void exit();

void init_posix(struct posix_t *p) {
    p->signal = 0;
    p->sig_sp = 0;
    p->masked = false;
    //p->sig_sp = (uint64_t) kmalloc(4096) + 4096;
    memset((char *)p->signal_handler, sizeof(p->signal_handler), 0);
    //p->signal_handler[SIGINT] = (uint64_t)exit;
    p->signal_handler[SIGKILL] = (uint64_t)exit;
}

void reset_posix(struct posix_t *p) {
    p->signal = 0;
}

void free_posix_stack(struct posix_t *p) {
    kfree((void *)p->sig_sp);
}

void copy_posix(struct posix_t *p, struct posix_t *c) {
    for(int i = 0; i < MAX_SIG; i++) c->signal_handler[i] = p->signal_handler[i];
}