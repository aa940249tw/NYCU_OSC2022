#ifndef __INITRD_H__
#define __INITRD_H__

#define CPIO_ADDR 0x20000000 // Rpi3
//#define CPIO_ADDR 0x8000000 // QEMU
#define CPIO_SIZE 110

#include "utils.h"

struct cpio_newc_header {
	char	c_magic[6];
	char	c_ino[8];
	char	c_mode[8];
	char	c_uid[8];
	char	c_gid[8];
	char	c_nlink[8];
	char	c_mtime[8];
	char	c_filesize[8];
	char	c_devmajor[8];
	char	c_devminor[8];
	char	c_rdevmajor[8];
	char	c_rdevminor[8];
	char	c_namesize[8];
	char	c_check[8];
};

struct exec_t {
	struct cpio_newc_header *head;
	char *filecontext;
	size_t len;
};

void cpio_init();
void cpio_ls();
void cpio_cat(char *);
struct exec_t *cpio_find(char *);
unsigned long align(unsigned long, unsigned long);

#endif