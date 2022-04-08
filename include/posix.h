#ifndef __POSIX_H__
#define __POSIX_H__

#include "utils.h"

#define MAX_SIG 16
#define SIGKILL 9
#define SIGINT  2


struct posix_t {
    uint16_t signal;
    uint64_t signal_handler[MAX_SIG];
    uint64_t sig_sp;
};

void init_posix(struct posix_t *);
void reset_posix(struct posix_t *);
void free_posix_stack(struct posix_t *);

#endif

