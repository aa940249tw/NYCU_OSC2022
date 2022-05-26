#ifndef __DEV_H__
#define __DEV_H__

#include "utils.h"
#include "vfs.h"
#include "thread.h"

#define UARTFILE 001
#define FRAMEBUF 010
#define SEEK_SET 0

void dev_init();
int mknod(const char *, int, uint32_t);
void open_uart_fds(struct thread_t *cur);
long __lseek64(struct file* file, long offset, int whence);
int __ioctl(struct file* file, unsigned long request, ...); 

struct special_fops {
    int (*init)(struct vnode *target_file);
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*open)(struct vnode* file_node, struct file **target);
    int (*close)(struct file* file);
};

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
    void *lfb;
};

// uart file ops
int uart_dev_init(struct vnode *target_file);
int uart_dev_read(struct file *file, void *buf, size_t len);
int uart_dev_write(struct file *file, const void *buf, size_t len);
int uart_dev_open(struct vnode* file_node, struct file **target);
int uart_dev_close(struct file* file);
// framebuffer file ops
int frame_dev_init(struct vnode *target_file);
int frame_dev_read(struct file *file, void *buf, size_t len);
int frame_dev_write(struct file *file, const void *buf, size_t len);
int frame_dev_open(struct vnode* file_node, struct file **target);
int frame_dev_close(struct file* file);
#endif