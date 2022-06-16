#include "vfs.h"
#include "mm.h"
#include "utils.h"
#include "tmpfs.h"
#include "uart.h"
#include "initrd.h"

struct mount rootfs;
struct filesystem *filesystem_list;

struct filesystem *get_filesystem(const char *name) {
    struct filesystem *fs = filesystem_list->next;
    while(fs) {
        if(!strcmp(fs->name, name)) return fs;
        fs = fs->next;
    }
    return NULL;
}

int register_filesystem(struct filesystem *fs) {
    // register the file system to the kernel.
    // you can also initialize memory pool of the file system here.
    if(get_filesystem(fs->name)) return -1;
    struct filesystem *tmp = filesystem_list->next;
    filesystem_list->next = fs;
    fs->next = tmp;
    return 0;
}

int get_vnode(struct vnode **target, const char* pathname) {
    struct vnode *itr = rootfs.root;
    char *path_tmp = (char *)kmalloc(sizeof(char) * 256);
    strcpy(path_tmp, pathname);
    char *path = strtok(path_tmp, '/');
    *target = itr;
    path = strtok(NULL, '/');
    while(path != NULL) {
        struct vnode *next_vnode;
        int ret = itr->v_ops->lookup(itr, &next_vnode, path);
        if(ret != 0) return ret;
        else {
            while(next_vnode->mount != NULL) 
                next_vnode = next_vnode->mount->root;
            itr = next_vnode;
            *target = itr;
            path = strtok(NULL, '/');
        }
    }
    return 0;
}

char *get_filename(const char *pathname) {
    char *path_tmp = (char *)kmalloc(sizeof(char) * 256);
    strcpy(path_tmp, pathname);
    char *path = strtok(path_tmp, '/');
    char *ret = NULL;
    while(path != NULL) {
        ret = path;
        path = strtok(NULL, '/');
    }
    return ret;
}

struct file *vfs_open(const char* pathname, int flags) {
    // 1. Lookup pathname from the root vnode.
    // 2. Create a new file descriptor for this vnode if found.
    // 3. Create a new file if O_CREAT is specified in flags.
    struct vnode *target_dir;
    struct file *file;
    int ret = get_vnode(&target_dir, pathname);
    if(ret != 0) {
        if((ret == PATH_NOT_FOUND && !strtok(NULL, '/')) && (flags & O_CREAT)) {
            struct vnode *target_file;
            int ret = target_dir->v_ops->create(target_dir, &target_file, get_filename(pathname));
            if(ret < 0) return NULL;
            target_file->f_ops->open(target_file, &file);
        }
        else return NULL;
    }
    else target_dir->f_ops->open(target_dir, &file);
    return file;
}

int vfs_close(struct file* file) {
    // 1. release the file descriptor
    return file->f_ops->close(file);
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len) {
    // 1. read min(len, readable file data size) byte to buf from the opened file.
    // 2. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname) {
    struct vnode *target_dir;
    int ret = get_vnode(&target_dir, pathname);
    if(ret == PATH_NOT_FOUND && !strtok(NULL, '/')) {
        struct vnode *create_dir;
        char *dir_name = get_filename(pathname);
        int ret = target_dir->v_ops->mkdir(target_dir, &create_dir, dir_name);
        if(ret < 0) {
            printf("MKDIR: CREATE ERROR!!!\n");
            return -1;
        }
        return ret;
    }
    else if(ret == 0) {
        printf("Path already exsist!!!\n");
        return 0;
    }
    else {
        printf("MKDIR: PATH ERROR!!!\n");
        return -1;
    }
}

int vfs_mount(const char *target, const char *filesystem) {
    struct filesystem *fs = get_filesystem(filesystem);
    struct vnode *target_dir;
    get_vnode(&target_dir, target);
    struct mount *m = (struct mount *)kmalloc(sizeof(struct mount));
    target_dir->mount = m;
    fs->setup_mount(fs, m);
    return 0;
}

void rootfs_init() {
    // Init filesystem_list
    filesystem_list = (struct filesystem *)kmalloc(sizeof(struct filesystem));
    filesystem_list->name = "head";
    filesystem_list->next = NULL;
    // Create tmpfs filesystem
    struct filesystem *tmpfs = (struct filesystem *)kmalloc(sizeof(struct filesystem)); 
    tmpfs->name = "tmpfs";
    tmpfs->setup_mount = &tmpfs_set_mount;
    register_filesystem(tmpfs);
    tmpfs_set_mount(tmpfs, &rootfs);
}

extern struct cpio_newc_header* Header;
void initramfs() {
    vfs_mkdir("/initramfs");
    struct cpio_newc_header *header = Header;
    char new_path[64];
    while(1) {
        unsigned long file_size = hex_to_int(header->c_filesize, 8);
        unsigned long name_size = hex_to_int(header->c_namesize, 8);
        unsigned long mode = hex_to_int(header->c_mode, 8);
        char *pathname = (char *)((char *)header + CPIO_SIZE);
        if(!strcmp(pathname, "TRAILER!!!")) break;
        strcpy(new_path, "/initramfs/");
        strcat(new_path, pathname);
        if(mode & 0040000) vfs_mkdir(new_path);
        else if(mode & 0100000) {
            struct file *f = vfs_open(new_path, O_CREAT);
            vfs_write(f, (void *)((char *)header + align(CPIO_SIZE + name_size, 4)), file_size);
            vfs_close(f);
        }
        header = (struct cpio_newc_header*)((char *)header + align(CPIO_SIZE + name_size, 4) + align(file_size, 4));
    }
}
