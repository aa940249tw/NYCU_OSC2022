#ifndef __VFS_H__
#define __VFS_H__

#include "utils.h"

#define O_CREAT 0100

#define PATH_LEN    16
#define PATH_NOT_FOUND  -1
#define FILE_FOUND      -2

#define RO  0
#define RW  1
// Mount object
struct mount {
    struct vnode *root;
    struct filesystem *fs;
};

struct filesystem {
    const char *name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount);
    struct filesystem *next;
};

// Inode + dentry
struct vnode {  
    struct mount *mount;
    struct vnode_operations *v_ops;
    struct file_operations *f_ops;
    void *internal;
};

struct file {
    struct vnode *vnode;
    size_t f_pos; // The next read/write position of this opened file
    struct file_operations *f_ops;
    int flags;
};

struct file_operations {
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*open) (struct vnode* file_node, struct file **target);
    int (*close) (struct file* file);
};

struct vnode_operations {
    int (*lookup)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int (*create)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode** target, const char* component_name);
};

int register_filesystem(struct filesystem *);
struct file *vfs_open(const char *, int);
void rootfs_init();
struct filesystem *get_filesystem(const char *);
int register_filesystem(struct filesystem *);
int get_vnode(struct vnode **, const char *);
char *get_filename(const char *);
struct file *vfs_open(const char *, int);
int vfs_close(struct file *);
int vfs_write(struct file *, const void *, size_t);
int vfs_read(struct file *, void *, size_t);
int vfs_mkdir(const char *);
int vfs_mount(const char *, const char *);
void initramfs();


#endif