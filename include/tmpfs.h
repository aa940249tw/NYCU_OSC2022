#ifndef __TMPFS_H__
#define __TMPFS_H__

#include "utils.h"
#include "vfs.h"
#define MAX_ENTRY   16

struct tmpfs_data {
    char name[256];
    int size;
    char *type;
    void *data; //pointer to the vnode it represented
    struct vnode *vnode;
};

struct tmpfs_dirdata {
    struct {
        char name[256]; // img1
        struct tmpfs_data *next; // img1 tmpfs_data
    } entry[MAX_ENTRY];
};

struct tmpfs_filedata {
    char *data;
    int capacity;
};

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_close(struct file *file);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_set_mount(struct filesystem *fs, struct mount *mount);

#endif