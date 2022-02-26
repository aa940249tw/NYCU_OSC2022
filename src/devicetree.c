#include "devicetree.h"
#include "uart.h"
#include "utils.h"

void dtb_info() {
    // get devicetree info
    struct fdt_header **header = (struct fdt_header **)&__devicetree;
    unsigned int totalsize, off_dt_strings, off_dt_struct, size_dt_strings, size_dt_struct;
    totalsize        = convert_big_to_small_endian((*header)->totalsize);
    off_dt_strings   = convert_big_to_small_endian((*header)->off_dt_strings);
    off_dt_struct    = convert_big_to_small_endian((*header)->off_dt_struct);
    size_dt_strings  = convert_big_to_small_endian((*header)->size_dt_strings);
    size_dt_struct   = convert_big_to_small_endian((*header)->size_dt_struct);

    printf("=============================================\n");
    printf("Devicetree Info:\n");
    printf("Totalsize: %d\n", totalsize);
    printf("Off_dt_strings: %d\n", off_dt_strings);
    printf("Off_dt_struct: %d\n", off_dt_struct);
    printf("Size_dt_strings: %d\n", size_dt_strings);
    printf("Size_dt_struct: %d\n", size_dt_struct);
    printf("=============================================\n");

    // parse devicetree
    unsigned long struct_address = (unsigned long)*header + (unsigned long) off_dt_struct;
    unsigned long struct_end = struct_address + (unsigned long) size_dt_struct;
    unsigned char *cur_address = (unsigned char *)struct_address;

    while((unsigned long)cur_address < struct_end) {
        unsigned int token = convert_big_to_small_endian(*((unsigned int *)cur_address));
        cur_address += 4;
        switch(token) {
            case FDT_BEGIN_NODE:
                printf("**********************************\n");

                printf("Device name: %s\n", cur_address);
                cur_address += strlen((char *)cur_address);
                if((unsigned long)cur_address % 4) cur_address +=  (4 - (unsigned long)cur_address % 4); 
                break;
            case FDT_END_NODE:
                break;
            case FDT_PROP: 
                ;
                unsigned int len, nameoff;
                len = convert_big_to_small_endian(*((unsigned int *)cur_address));
                cur_address += 4;
                nameoff = convert_big_to_small_endian(*((unsigned int *)cur_address));
                cur_address += 4;
                if(len > 0) {
                    printf("Property Name:  %s\n", (char *)((unsigned long)*header + (unsigned long) off_dt_strings + nameoff));
                    printf("Property Value: %s\n", (char *)cur_address);
                    cur_address += len;
                    if((unsigned long)cur_address % 4) cur_address +=  (4 - (unsigned long)cur_address % 4);
                }
                break;
            case FDT_NOP:
                break;
            case FDT_END:
                break;
            default: break;
        }

    }
    printf("=============================================\n");
}  

unsigned long get_initramfs(char *key) {
    struct fdt_header **header = (struct fdt_header **)&__devicetree;
    unsigned int off_dt_strings, off_dt_struct, size_dt_struct;
    off_dt_strings   = convert_big_to_small_endian((*header)->off_dt_strings);
    off_dt_struct    = convert_big_to_small_endian((*header)->off_dt_struct);
    size_dt_struct   = convert_big_to_small_endian((*header)->size_dt_struct);

    unsigned long struct_address = (unsigned long)*header + (unsigned long) off_dt_struct;
    unsigned long struct_end = struct_address + (unsigned long) size_dt_struct;
    unsigned char *cur_address = (unsigned char *)struct_address;

    while((unsigned long)cur_address < struct_end) {
        unsigned int token = convert_big_to_small_endian(*((unsigned int *)cur_address));
        cur_address += 4;
        switch(token) {
            case FDT_BEGIN_NODE:
                cur_address += strlen((char *)cur_address);
                if((unsigned long)cur_address % 4) cur_address +=  (4 - (unsigned long)cur_address % 4); 
                break;
            case FDT_END_NODE:
                break;
            case FDT_PROP: 
                ;
                unsigned int len, nameoff;
                len = convert_big_to_small_endian(*((unsigned int *)cur_address));
                cur_address += 4;
                nameoff = convert_big_to_small_endian(*((unsigned int *)cur_address));
                cur_address += 4;
                if(len > 0) {
                    char *tmp = (char *)((unsigned long)*header + (unsigned long) off_dt_strings + nameoff);
                    if(!strncmp(tmp, key, 18)) return convert_big_to_small_endian(*((unsigned int *)cur_address));
                    cur_address += len;
                    if((unsigned long)cur_address % 4) cur_address +=  (4 - (unsigned long)cur_address % 4);
                }
                break;
            case FDT_NOP:
                break;
            case FDT_END:
                break;
            default: break;
        }
    }
    return 0;
}