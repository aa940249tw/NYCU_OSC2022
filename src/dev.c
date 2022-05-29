#include "dev.h"
#include "uart.h"
#include "mbox.h"
#include "tmpfs.h"
#include "thread.h"

struct framebuffer_info fb;

static const struct special_fops uartfops = {
    .init = &uart_dev_init,
    .write = &uart_dev_write,
    .read = &uart_dev_read,
    .open = &uart_dev_open,
    .close = &uart_dev_close,
};

static const struct special_fops framefops = {
    .init = &frame_dev_init,
    .write = &frame_dev_write,
    .read = &frame_dev_read,
    .open = &frame_dev_open,
    .close = &frame_dev_close,
};

void dev_init() {
    vfs_mkdir("/dev");
    mknod("/dev/uart", 0, UARTFILE);
    mknod("/dev/framebuffer", 0, FRAMEBUF);
}

void init_dev(struct vnode *target_file, uint32_t dev) {
    if(dev & UARTFILE) {
        //uart_dev_init(target_file);
        target_file->f_ops->write = uartfops.write;
        target_file->f_ops->read = uartfops.read;
        //target_file->f_ops->open = uartfops.open;
        //target_file->f_ops->close = uartfops.close;
    }
    else if(dev & FRAMEBUF) {
        frame_dev_init(target_file);
        target_file->f_ops->write = framefops.write;
        //target_file->f_ops->read = framefops.read;
        //target_file->f_ops->open = framefops.open;
        //target_file->f_ops->close = framefops.close;
    }
}

int mknod(const char *pathname, int mode, uint32_t dev) {
    struct vnode *target_dir;
    int ret = get_vnode(&target_dir, pathname);
    if(ret != 0) {
        if((ret == PATH_NOT_FOUND && !strtok(NULL, '/'))) {
            struct vnode *target_file;
            int ret = target_dir->v_ops->create(target_dir, &target_file, get_filename(pathname));
            if(ret < 0) return ret;
            init_dev(target_file, dev);
        }
    }
    return ret;
}

// uart file ops
void open_uart_fds(struct thread_t *cur) {
    struct file *f = vfs_open("/dev/uart", 0);
    cur->fd[0] = f;
    cur->fd[1] = f;
    cur->fd[2] = f;
}

int uart_dev_init(struct vnode *target_file) {
    uart_init();
	uart_flush();
    return 0;
}

int uart_dev_read(struct file *file, void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) ((char *)buf)[i] = uart_getc();
    //int ret = tmpfs_read(file, buf, len);
    return len;
}

int uart_dev_write(struct file *file, const void *buf, size_t len) {
    //int ret = tmpfs_write(file, buf, len);
    printf("%s", buf);
    return len;
}

int uart_dev_open(struct vnode* file_node, struct file **target) {
    return 0;
}

int uart_dev_close(struct file* file) {
    return 0;
}
// framebuffer file ops
#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8

int frame_dev_init(struct vnode *target_file) {
    unsigned int __attribute__((aligned(16))) mbox[36];

    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024; // FrameBufferInfo.width
    mbox[6] = 768;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // FrameBufferInfo.pitch

    mbox[34] = MBOX_TAG_LAST;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (__mbox_call(MBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        fb.width = mbox[5];        // get actual physical width
        fb.height = mbox[6];       // get actual physical height
        fb.pitch = mbox[33];       // get number of bytes per line
        fb.isrgb = mbox[24];       // get the actual channel order
        fb.lfb = (void *)((unsigned long)mbox[28]);
    } else {
        printf("Unable to set screen resolution to 1024x768x32\n");
    }
    return 0;
}

int frame_dev_read(struct file *file, void *buf, size_t len) {
    return 0;
}

int frame_dev_write(struct file *file, const void *buf, size_t len) {
    memcpy((void *)((char *)fb.lfb + file->f_pos), buf, len);
    file->f_pos += len;
    return 0;
}

int frame_dev_open(struct vnode* file_node, struct file **target) {
    return 0;
}

int frame_dev_close(struct file* file) {
    return 0;
}

long __lseek64(struct file* file, long offset, int whence) {
    long off = -1, pos;
    switch (whence)
    {
    case SEEK_SET:
        off = 0;
        break;
    default:
        break;
    }
    pos = off + offset;
    if ((offset < 0 && -offset > off) || (offset > 0 && pos <= off)) return -1;
    file->f_pos = pos;
    return pos;
}

int __ioctl(struct file* file, unsigned long request, ...) {
    return 0;
}

