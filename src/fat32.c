#include "fat32.h"
#include "sdhost.h"
#include "uart.h"
#include "mm.h"

struct fat32_meta fat32_metadata;
struct list_head *dirty_data;
struct FAT *fat;

static struct file_operations fat32_fops = {
    .write = &fat32_write,
    .read = &fat32_read,
    .open = &fat32_open,
    .close = &fat32_close
};

static struct vnode_operations fat32_vops = {
    .lookup = &fat32_lookup,
    .create = &fat32_create,
    .mkdir = &fat32_mkdir
};

void string_to_lower(const char *array, char *target) {
    int i = 0;
    while(array[i] != '\0') {
        target[i] = (array[i] >='A' && array[i] <= 'Z') ? (array[i] + 32) : (array[i]);
        i++;
    }
    target[i] = '\0';
}

unsigned long get_cluster_blkid(unsigned long cluster) {
    return fat32_metadata.cluster_begin_lba 
           + (cluster - fat32_metadata.root_dir_first_cluster) * fat32_metadata.sectors_per_cluster;
}

struct fat32_data *new_fat32_data(char *type, const char *name, struct vnode *self, int size, int cluster_id, struct fat32_data *parent) {
    struct fat32_data *fd = (struct fat32_data *)kmalloc(sizeof(struct fat32_data));
    fd->type = type;
    fd->vnode = self;
    strcpy(fd->name, name);
    fd->size = size;
    fd->first_cluster = cluster_id;
    fd->parent = parent;
    if(!strcmp(type, "dir")) {
        struct fat32_dircache *fdc = (struct fat32_dircache *)kmalloc(sizeof(struct fat32_dircache));
        fdc->cb.cached = false;
        fdc->cb.dirty = false;
        for(int i = 0; i < MAX_ENTRY; i++) {
            fdc->entry[i].name[0] = '\0';
            fdc->entry[i].next = NULL;
        }
        fd->data = fdc;
    }
    else {
        struct fat32_filecache *ffc = (struct fat32_filecache *)kmalloc(sizeof(struct fat32_filecache));
        for(int i = 0; i < MAX_FILE_BLK; i++) {
            ffc->data[i].cached = false;
            ffc->data[i].dirty = false;
            ffc->data[i].blk_id = 0;
        }
        fd->data = ffc;
    }
    return fd;
}

struct vnode *fat32_new_vnode(struct fat32_data *fd) {
    struct vnode *ret = (struct vnode *)kmalloc(sizeof(struct vnode));
    ret->f_ops = &fat32_fops;
    ret->v_ops = &fat32_vops;
    if(!fd) {
        fd->vnode = ret;
        ret->internal = fd;
    }
    return ret;
}

void update_dirinfo(struct fat32_data *fd) {
    struct fat32_dircache *fdc = (struct fat32_dircache *)fd->data;
    // Assume that dir only use one cluster
    // TODO: There will be cases that mixed using SFN and (SFN + LFN)
    unsigned long cluster_id = get_cluster_blkid(fd->first_cluster);
    readblock(cluster_id, fdc->cb.block);
    fdc->cb.cached = true;
    fdc->cb.blk_id = cluster_id;
    // parse
    for(int i = 0; i < MAX_ENTRY; i++) {
        struct dir_name *dentry = (struct dir_name *)fdc->cb.block + i;
        if(dentry->SFN.DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED) break;
        else if(dentry->SFN.DIR_Name[0] == DIR_ENTRY_UNUSED) continue;

        // Name1
        for(int j = 0 ; j < 5; j++) fdc->entry[i].name[j] = (char)dentry->LFN.LDIR_Name1[j];
        // Name2
        for(int j = 5 ; j < 11; j++) fdc->entry[i].name[j] = (char)dentry->LFN.LDIR_Name2[j - 5];
        // Name3
        for(int j = 11 ; j < 13; j++) fdc->entry[i].name[j] = (char)dentry->LFN.LDIR_Name3[j - 11];

        struct vnode *target = fat32_new_vnode(NULL);
        if(dentry->SFN.DIR_Attr == DIR_ENTRY_ATTR_DIRECTORY)
            fdc->entry[i].next = new_fat32_data("dir", fdc->entry[i].name, target, dentry->SFN.DIR_FileSize, 
                                                (dentry->SFN.DIR_FstClusHI << 16) | (dentry->SFN.DIR_FstClusLO), fd);
        else
            fdc->entry[i].next = new_fat32_data("file", fdc->entry[i].name, target, dentry->SFN.DIR_FileSize,
                                                (dentry->SFN.DIR_FstClusHI << 16) | (dentry->SFN.DIR_FstClusLO), fd);
        target->internal = fdc->entry[i].next;
    }
}

void read_FAT(int fat_id) {
    unsigned int blk_id = fat32_metadata.fat_begin_lba + fat_id;
    fat[fat_id].table.blk_id = blk_id;
    fat[fat_id].table.cached = true;
    readblock(blk_id, fat[fat_id].table.block);
}

int find_free_FAT_ENTRY() {
    int cluster_id = -1;
    // Find Not Used Cluster
    for(int i = 0; i < fat32_metadata.Sectors_Per_FAT / fat32_metadata.sectors_per_cluster; i++) {
        if(!fat[i].table.cached) read_FAT(i);
        for(int j = 2; j < FAT_ENTRY_PER_BLK; j++) {
            if(((unsigned int *)fat[i].table.block)[j] == FAT_ENTRY_FREE) {
                cluster_id = j;
                break;
            }
        }
        if(cluster_id >= 0) {
            cluster_id += BLOCK_SIZE * i;
            break;
        }
    }
    return cluster_id;
}

void update_fileinfo(struct fat32_data *fd) {
    struct fat32_filecache *fdc = (struct fat32_filecache *)fd->data;
    unsigned long cluster_id = get_cluster_blkid(fd->first_cluster);

    int data_id = 0;
    while(cluster_id < FAT_ENTRY_RESERVED_TO_END) {
        fdc->data[data_id].blk_id = cluster_id;
        fdc->data[data_id].cached = true;
        readblock(cluster_id, fdc->data[data_id].block);
        data_id++;

        int fat_id = cluster_id / FAT_ENTRY_PER_BLK;
        if(!fat[fat_id].table.cached) read_FAT(fat_id);
        cluster_id = ((unsigned int *)fat[fat_id].table.block)[cluster_id % FAT_ENTRY_PER_BLK];
    }
}

int fat32_write(struct file *file, const void *buf, size_t len) {
    struct fat32_data *fd = (struct fat32_data *)file->vnode->internal;
    struct fat32_data *parent = ((struct fat32_data *)file->vnode->internal)->parent;
    struct fat32_filecache *fdc = (struct fat32_filecache *)fd->data;
    struct fat32_dircache *pfdc = (struct fat32_dircache *)parent->data;
    if(!fdc || !pfdc) return -1;

    // Assume that only write one block, write from the begining, simple implementation
    // TODO: what if writing many blocks
    int data_id = file->f_pos / BLOCK_SIZE;
    if(!fdc->data[data_id].cached) update_fileinfo(fd);
    memcpy(fdc->data[data_id].block + file->f_pos, buf, len);
    file->f_pos += len;
    fd->size += len;
    if(!fdc->data[data_id].dirty) {
        fdc->data[data_id].dirty = true;
        list_add_tail(&fdc->data[data_id].list, dirty_data);
    }

    // Modify parent's dir data
    int entry = -1;
    for (int i = 0; i < MAX_ENTRY; i++) {
        if(!strcmp(pfdc->entry[i].name, fd->name)) {
            entry = i;
            break;
        }
    }
    struct dir_name *tmp = (struct dir_name *)pfdc->cb.block + entry;
    tmp->SFN.DIR_FileSize = fd->size;
    if(!pfdc->cb.dirty) {
        pfdc->cb.dirty = true;
        list_add_tail(&pfdc->cb.list, dirty_data);
    }
    return len;
}

int fat32_read(struct file *file, void *buf, size_t len) {
    struct fat32_data *fd = (struct fat32_data *)file->vnode->internal;
    struct fat32_filecache *fdc = (struct fat32_filecache *)fd->data;
    if(!fd->data) return -1;
    size_t read_len = (len < fd->size - file->f_pos) ? len : (fd->size - file->f_pos);
    
    size_t remain = read_len;
    while(remain > 0) {
        int data_id = file->f_pos / BLOCK_SIZE;
        size_t reads = (remain < BLOCK_SIZE - file->f_pos % BLOCK_SIZE) ? remain : BLOCK_SIZE - file->f_pos % BLOCK_SIZE;
        if(!fdc->data[data_id].cached) update_fileinfo(fd);
        memcpy(buf + (read_len - remain), fdc->data[data_id].block + file->f_pos % BLOCK_SIZE, reads);
        remain -= reads; 
        file->f_pos += reads;
    }    
    return read_len;
}

int fat32_close(struct file *file) {
    kfree(file);
    return 0;
}

int fat32_open(struct vnode *file_node, struct file **target) {
    *target = (struct file *)kmalloc(sizeof(struct file));
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    (*target)->flags = RW;
    (*target)->vnode = file_node;
    return 0;
}   

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    // Not Implemented
    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    struct fat32_dircache *fdc = (struct fat32_dircache *)((struct fat32_data *)(dir_node->internal))->data;
    char cmp[128];
    string_to_lower(component_name, cmp);  // Only used to fit LFN testcase
    if(!strcmp(((struct fat32_data *)(dir_node->internal))->type, "dir")) {
        if(!fdc->cb.cached) update_dirinfo(((struct fat32_data *)(dir_node->internal)));
        for (int i = 0; i < MAX_ENTRY; i++) {
            if(!strcmp(fdc->entry[i].name, cmp)) {
                if(fdc->entry[i].next->vnode) *target = fdc->entry[i].next->vnode;
                else *target = fat32_new_vnode(fdc->entry[i].next);
                return 0;
            }
        }
        return PATH_NOT_FOUND;
    }
    return FILE_FOUND;
}

unsigned char lfn_checksum(const unsigned char *pFCBName) {
   int i;
   unsigned char sum = 0;

   for (i = 11; i; i--)
      sum = ((sum & 1) << 7) + (sum >> 1) + *pFCBName++;

   return sum;
}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    struct fat32_data *fd = (struct fat32_data *)dir_node->internal;
    struct fat32_dircache *fdc = (struct fat32_dircache *)fd->data;
    char cmp[128];
    string_to_lower(component_name, cmp);  // Only used to fit LFN testcase
    // Find dir empty entry
    if(!fdc->cb.cached) update_dirinfo(fd);
    int entry = -1;
    for(int i = 0; i < MAX_ENTRY; i++) {
        if(!strcmp(fdc->entry[i].name, cmp)) return -1;
        else if(!fdc->entry[i].next) {
            entry = i;
            break;
        }
    }
    // Find Not Used Cluster
    int cluster_id = find_free_FAT_ENTRY();

    if(entry == -1 || cluster_id == -1) return -1;
    *target = fat32_new_vnode(NULL);
    (*target)->internal = new_fat32_data("file", cmp, *target, 0, cluster_id, fd);
    strcpy(fdc->entry[entry].name, cmp);
    fdc->entry[entry].next = (*target)->internal;
    // Modify dir block
    struct dir_name tmp;
    char *pFCBName = (char *)kmalloc(sizeof(char) * 11);
    memset(pFCBName, 11, 0x20);
    int cnt = 0;
    for(int i = 0; i < 8; i++, cnt++) {
        if(component_name[cnt] == '.') {
            cnt++;
            break;
        }
        pFCBName[i] = component_name[cnt];
    }
    while(component_name[cnt-1] != '.') ++cnt;
    for(int i = 8; i < 11; i++, cnt++) pFCBName[i] = component_name[cnt];
    // SFN
    strcpy((char *)tmp.SFN.DIR_Name, pFCBName);
    tmp.SFN.DIR_Attr = DIR_ENTRY_ATTR_ARCHIVE;
    tmp.SFN.DIR_FstClusHI = (uint16_t)(cluster_id >> 16);
    tmp.SFN.DIR_FstClusLO = (uint16_t)(cluster_id & 0xFF);
    tmp.SFN.DIR_FileSize = 0;
    // LFN
    tmp.LFN.LDIR_Ord = 0x41;    // Assume Using only one entry
    tmp.LFN.LDIR_Attr = 0x0F;
    tmp.LFN.LDIR_Type = 0x00;
    tmp.LFN.LDIR_FstClusLO = 0x0;
    tmp.LFN.LDIR_Chksum = lfn_checksum((const unsigned char *)pFCBName);
    cnt = 0;
    int len = strlen(cmp);
    // Name1
    for(int j = 0; j < 5; j++) 
        tmp.LFN.LDIR_Name1[j] = cnt < len ? (uint16_t)cmp[cnt++] : (cnt == len ? 0x0 : 0xFFFF);
    // Name2
    for(int j = 0; j < 6; j++) 
        tmp.LFN.LDIR_Name2[j] = cnt < len ? (uint16_t)cmp[cnt++] : (cnt == len ? 0x0 : 0xFFFF);
    // Name3
    for(int j = 0; j < 2; j++) 
        tmp.LFN.LDIR_Name3[j] = cnt < len ? (uint16_t)cmp[cnt++] : (cnt == len ? 0x0 : 0xFFFF);

    memcpy((void *)((struct dir_name *)fdc->cb.block + entry), &tmp, sizeof(tmp));

    if(!fdc->cb.dirty) {
        fdc->cb.dirty = true;
        list_add_tail(&fdc->cb.list, dirty_data);
    }

    int fat_id = cluster_id / FAT_ENTRY_PER_BLK;
    ((unsigned int *)fat[fat_id].table.block)[cluster_id % FAT_ENTRY_PER_BLK] = FAT_ENTRY_END_OF_FILE;
    if(!fat[fat_id].table.dirty) {
        fat[fat_id].table.dirty = true;
        list_add_tail(&fat[fat_id].table.list, dirty_data);
    }
    return 0;
}

int fat32_set_mount(struct filesystem *fs, struct mount *mount) {
    mount->fs = fs;
    struct vnode *root = fat32_new_vnode(NULL);
    root->internal = new_fat32_data("dir", "/", root, 0, fat32_metadata.root_dir_first_cluster, NULL);
    mount->root = root;
    return 0;
}

void fat32_init() {
    struct MBR mbr;
    readblock(0, (void *)&mbr);

    if(mbr.boot_signature[0] != 0x55 || mbr.boot_signature[1] != 0xAA) {
        printf("MBR signature error !!!\n");
        while(1);
    }

    if(mbr.part_table[0].PartType != 0x0B && mbr.part_table[0].PartType != 0x0C) {
        printf("MBR part type error !!!\n");
        while(1);
    }

    char buf[BLOCK_SIZE];
    readblock(mbr.part_table[0].StartLBA, buf);
    struct fat32_bootsector *fat32_bs = (struct fat32_bootsector *)buf;
    fat32_metadata.Number_of_Reserved_Sectors = fat32_bs->num_of_reserved_sector;
    fat32_metadata.Number_of_FATs = fat32_bs->num_of_fat;
    fat32_metadata.Sectors_Per_FAT = fat32_bs->sectors_per_fat_fat32;
    fat32_metadata.fat_begin_lba = fat32_metadata.Number_of_Reserved_Sectors + mbr.part_table[0].StartLBA;
    fat32_metadata.cluster_begin_lba = fat32_metadata.fat_begin_lba + (fat32_metadata.Number_of_FATs * fat32_metadata.Sectors_Per_FAT);
    fat32_metadata.sectors_per_cluster = fat32_bs->sectors_per_cluster;
    fat32_metadata.root_dir_first_cluster = fat32_bs->first_cluster_of_root_dir;
    
    // printf("StartLBA: %x\n", mbr.part_table[0].StartLBA);
    // printf("Number_of_Reserved_Sectors: %d\n", fat32_metadata.Number_of_Reserved_Sectors);
    // printf("Number_of_FATs: %d\n", fat32_metadata.Number_of_FATs);
    // printf("Sectors_Per_FAT: %d\n", fat32_metadata.Sectors_Per_FAT);
    // printf("fat_begin_lba: %x\n", fat32_metadata.fat_begin_lba);
    // printf("cluster_begin_lba: %x\n", fat32_metadata.cluster_begin_lba); 
    // printf("sectors_per_cluster: %d\n", fat32_metadata.sectors_per_cluster);
    // printf("root_dir_first_cluster %d\n", fat32_metadata.root_dir_first_cluster);
    
    dirty_data = (struct list_head *)kmalloc(sizeof(struct list_head));
    INIT_LIST_HEAD(dirty_data);
    fat = (struct FAT *)kmalloc(sizeof(struct FAT) * (fat32_metadata.Sectors_Per_FAT / fat32_metadata.sectors_per_cluster));

    struct filesystem *fs = (struct filesystem *)kmalloc(sizeof(struct filesystem));
    fs->name = "fat32";
    fs->setup_mount = &fat32_set_mount;
    register_filesystem(fs);
    vfs_mkdir("/boot");
    vfs_mount("/boot", "fat32");
}

void fat32_sync() {
    struct list_head *h = dirty_data;
    while(h->next != h) {
        struct list_head *tmp = h->next;
        struct cache_block *cb = container_of(tmp, struct cache_block, list);
        list_del(tmp);
        // printf("blockid: %d\n", cb->blk_id);
        writeblock(cb->blk_id, cb->block);
        cb->dirty = false;
    }
}