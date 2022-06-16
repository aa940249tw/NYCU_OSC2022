#ifndef __FAT32_H__
#define __FAT32_H__

#include "utils.h"
#include "vfs.h"
#include "list.h"

#define BLOCK_SIZE                  512
#define MAX_ENTRY                   8
#define MAX_FILE_BLK                8
#define DIR_ENTRY_LAST_AND_UNUSED   0x0
#define DIR_ENTRY_UNUSED            0xE5
#define DIR_ENTRY_ATTR_DIRECTORY    0x10
#define DIR_ENTRY_ATTR_ARCHIVE      0x20
#define FAT_ENTRY_PER_BLK           (BLOCK_SIZE / sizeof(uint32_t))
#define FAT_ENTRY_MASK              0x0fffffff
#define FAT_ENTRY_FREE              0
#define FAT_ENTRY_RESERVED_TO_END   0x0ffffff8
#define FAT_ENTRY_END_OF_FILE       0x0fffffff

struct fat32_meta {
    int Number_of_Reserved_Sectors;
    int Number_of_FATs;
    int Sectors_Per_FAT;
    unsigned long fat_begin_lba;
    unsigned long cluster_begin_lba;
    unsigned char sectors_per_cluster;
    unsigned long root_dir_first_cluster;
};

struct MBR_partition {
    uint8_t     status;
    uint8_t     StartAddrHead;
    uint16_t    StartAddrCylSec;
    uint8_t     PartType;
    uint8_t     EndAddrHead;
    uint16_t    EndAddrCylSec;
    uint32_t    StartLBA;
    uint32_t    EndLBA;
}__attribute__((packed));

struct MBR {
    unsigned char code[446];
    struct MBR_partition part_table[4];     // start from 0x1BE
    unsigned char boot_signature[2];        // 0x01FE, 0x01FF
}__attribute__((packed));

struct fat32_bootsector {
    uint8_t     jump_instruction[3];        // 0x000 - 0x002
    uint8_t     oem_name[8];                // 0x003 - 0x00A 
    uint16_t    bytes_per_sector;           // 0x00B - 0x00C
    uint8_t     sectors_per_cluster;        // 0x00D
    uint16_t    num_of_reserved_sector;     // 0x00E - 0x00F
    uint8_t     num_of_fat;                 // 0x010
    uint16_t    num_of_root_dir_entries;    // 0x011 - 0x012
    uint16_t    num_of_sectors;             // 0x013 - 0x014
    uint8_t     media_descriptor;           // 0x015
    uint16_t    sectors_per_fat;            // 0x016 - 0x017
    uint16_t    sectors_per_track;          // 0x018 - 0x019
    uint16_t    num_of_heads;               // 0x01A - 0x01B
    uint32_t    num_of_hidden_sectors;      // 0x01C - 0x01F
    uint32_t    num_of_sectors_fat32;       // 0x020 - 0x023
    uint32_t    sectors_per_fat_fat32;      // 0x024 - 0x027
    uint16_t    mirror_flags;               // 0x028 - 0x029
    uint16_t    version;                    // 0x02A - 0x02B
    uint32_t    first_cluster_of_root_dir;  // 0x02C - 0x02F
    uint16_t    info_sector;                // 0x030 - 0x031
    uint16_t    back_up_boot_sectors;       // 0x032 - 0x033
    uint32_t    reserved[3];                // 0x034 - 0x03F
    uint8_t     physical_drive_number;      // 0x040
    uint8_t     for_various_purpose;        // 0x041
    uint8_t     extended_signature;         // 0x042
    uint32_t    volume_id;                  // 0x043 - 0x046
    uint8_t     volume_label[11];           // 0x047 - 0x051
    uint64_t    filesystem_type;            // 0x052 - 0x059
}__attribute__((packed));

struct dir_entry_8_3 {
    uint8_t DIR_Name[11];           /* Offset 0 */
    uint8_t DIR_Attr;               /* Offset 11 */
    uint8_t DIR_NTRes;              /* Offset 12 */
    uint8_t DIR_CrtTimeHundth;      /* Offset 13 */
    uint16_t DIR_CrtTime;           /* Offset 14 */
    uint16_t DIR_CrtDate;           /* Offset 16 */
    uint16_t DIR_LstAccDate;        /* Offset 18 */
    uint16_t DIR_FstClusHI;         /* Offset 20 */
    uint16_t DIR_WrtTime;           /* Offset 22 */
    uint16_t DIR_WrtDate;           /* Offset 24 */
    uint16_t DIR_FstClusLO;         /* Offset 26 */
    uint32_t DIR_FileSize;          /* Offset 28 */
}__attribute__((packed));

struct dir_entry_long {
    uint8_t LDIR_Ord;               /* Offset 0 */
    uint16_t LDIR_Name1[5];         /* Offset 1 */
    uint8_t LDIR_Attr;              /* Offset 11 */
    uint8_t LDIR_Type;              /* Offset 12 */
    uint8_t LDIR_Chksum;            /* Offset 13 */
    uint16_t LDIR_Name2[6];         /* Offset 14 */
    uint16_t LDIR_FstClusLO;        /* Offset 26 */
    uint16_t LDIR_Name3[2];         /* Offset 28 */
}__attribute__((packed));

// Assume that the name won't use more than two LFNs
struct dir_name {
    struct dir_entry_long LFN;
    struct dir_entry_8_3 SFN;
}__attribute__((packed));

struct cache_block {
    struct list_head list;
    bool dirty;
    bool cached;
    unsigned int blk_id;
    char block[BLOCK_SIZE];
};

struct fat32_data {
    char name[256];
    int size;
    char *type;
    unsigned int first_cluster;
    void *data;
    struct vnode *vnode;
    struct fat32_data *parent;
};

struct fat32_dircache {
    struct cache_block cb;
    struct {
        char name[256];
        struct fat32_data *next;
    } entry[MAX_ENTRY];
};

struct fat32_filecache {
    struct cache_block data[MAX_FILE_BLK];
};

struct FAT {
    struct cache_block table;
};

void fat32_init();

int fat32_write(struct file *file, const void *buf, size_t len);
int fat32_read(struct file *file, void *buf, size_t len);
int fat32_close(struct file *file);
int fat32_open(struct vnode *file_node, struct file **target);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_set_mount(struct filesystem *fs, struct mount *mount);
void fat32_sync();

#endif