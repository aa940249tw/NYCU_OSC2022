#include "vfs.h"
#include "mm.h"
#include "tmpfs.h"
#include "utils.h"
#include "uart.h"

int tmpfs_write(struct file *file, const void *buf, size_t len) {
    struct tmpfs_filedata *fd = ((struct tmpfs_data *)file->vnode->internal)->data;
    if(!fd) return -1;
    if(fd->capacity == 0) {
        fd->data = kmalloc(file->f_pos + len);
        fd->capacity = file->f_pos + len;
    }
    else if(fd->capacity < file->f_pos + len) {
        char *data = fd->data;
        fd->data = kmalloc(file->f_pos + len);
        memcpy(fd->data, data, fd->capacity);
        kfree(data);
        fd->capacity = file->f_pos + len;
    }
    memcpy((void *)(fd->data + file->f_pos), buf, len);
    file->f_pos += len; 
    return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len) {
    struct tmpfs_filedata *fd = ((struct tmpfs_data *)file->vnode->internal)->data;
    if(!fd) return -1;
    size_t read_len = (len < fd->capacity) ? len : fd->capacity;
    memcpy((void *)buf, (fd->data + file->f_pos), read_len);
    file->f_pos += len;
    return read_len;
}

int tmpfs_close(struct file* file) {
    kfree(file);
    return 0;
}

struct tmpfs_data *new_tmpfs_data(char *type, const char *name, struct vnode *self, int size) {
    struct tmpfs_data *td = (struct tmpfs_data *)kmalloc(sizeof(struct tmpfs_data));
    td->type = type;
    td->vnode = self;
    strcpy(td->name, name);
    td->size = size;
    if(!strcmp(type, "dir")) {
        struct tmpfs_dirdata *tdd = (struct tmpfs_dirdata *)kmalloc(sizeof(struct tmpfs_dirdata));
        for(int i = 0; i < MAX_ENTRY; i++) {
            tdd->entry[i].name[0] = '\0';
            tdd->entry[i].next = NULL;
        }
        td->data = tdd;
    }
    else {
        struct tmpfs_filedata *tdd = (struct tmpfs_filedata *)kmalloc(sizeof(struct tmpfs_filedata));
        tdd->capacity = 0;
        td->data = tdd;
    }
    return td;
}

struct vnode *tmpfs_new_vnode(struct tmpfs_data *td) {
    struct vnode *vtmp = (struct vnode *)kmalloc(sizeof(struct vnode));
    struct vnode_operations *tmpfs_vn_op = (struct vnode_operations *)kmalloc(sizeof(struct vnode_operations));
    struct file_operations *tmpfs_f_op = (struct file_operations *)kmalloc(sizeof(struct file_operations));
    tmpfs_vn_op->create = &tmpfs_create;
    tmpfs_vn_op->lookup = &tmpfs_lookup;
    tmpfs_vn_op->mkdir = &tmpfs_mkdir;
    tmpfs_f_op->read = &tmpfs_read;
    tmpfs_f_op->write = &tmpfs_write;
    tmpfs_f_op->open = &tmpfs_open;
    tmpfs_f_op->close = &tmpfs_close;
    vtmp->f_ops = tmpfs_f_op;
    vtmp->v_ops = tmpfs_vn_op;
    if(!td) {
        td->vnode = vtmp;
        vtmp->internal = td;
    }
    return vtmp;
}

int tmpfs_mkdir(struct vnode* dir_node, struct vnode **target, const char *component_name) {
    struct tmpfs_dirdata *tdd = ((struct tmpfs_data *)(dir_node->internal))->data;
    int entry = -1;
    for(int i = 0; i < MAX_ENTRY; i++) {
        if(!tdd->entry[i].next) {
            entry = i;
            break;
        }
    }
    if(entry == -1) return -1;
    *target = tmpfs_new_vnode(NULL);
    (*target)->internal = new_tmpfs_data("dir", component_name, *target, 0);
    strcpy(tdd->entry[entry].name, component_name);
    tdd->entry[entry].next = (*target)->internal;
    return 0;
}

int tmpfs_open(struct vnode *file_node, struct file **target) {
    *target = (struct file *)kmalloc(sizeof(struct file));
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    (*target)->flags = RW;
    (*target)->vnode = file_node;
    return 0;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    if(!strcmp(((struct tmpfs_data *)(dir_node->internal))->type, "dir")) {
        struct tmpfs_dirdata *tdd = ((struct tmpfs_data *)(dir_node->internal))->data;
        for (int i = 0; i < MAX_ENTRY; i++) {
            if(!strcmp(tdd->entry[i].name, component_name)) {
                if(tdd->entry[i].next->vnode) *target = tdd->entry[i].next->vnode;
                else *target = tmpfs_new_vnode(tdd->entry[i].next);
                return 0;
            }
        }
        return PATH_NOT_FOUND;
    }
    return FILE_FOUND;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    struct tmpfs_dirdata *tdd = ((struct tmpfs_data *)(dir_node->internal))->data;
    int entry = -1;
    for (int i = 0; i < MAX_ENTRY; i++) {
        if(!strcmp(tdd->entry[i].name, component_name)) return -1;
        else if(!tdd->entry[i].next) entry = i; 
    }
    if(entry == -1) return -1;
    *target = tmpfs_new_vnode(NULL);
    (*target)->internal = new_tmpfs_data("file", component_name, *target, 0);
    strcpy(tdd->entry[entry].name, component_name);
    tdd->entry[entry].next = (*target)->internal;
    return 0;
}

int tmpfs_set_mount(struct filesystem *fs, struct mount *mount) {
    mount->fs = fs;
    struct vnode *root = tmpfs_new_vnode(NULL);
    root->internal = new_tmpfs_data("dir", "/", root, 0);
    mount->root = root;
    return 0;   
}
