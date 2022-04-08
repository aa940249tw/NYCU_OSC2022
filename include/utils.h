#ifndef __UTILS_H__
#define __UTILS_H__

#define NULL 0
#define allocator_init 0x1000000

typedef enum { false, true } bool;
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long int   uint64_t;
typedef unsigned long       size_t;

int strcmp(const char *, const char *);
int strncmp(const char *, const char *, unsigned int);
char *strncpy(char *, const char *, unsigned int);
int hex_to_int(char *, int);
char *simple_malloc(unsigned long); 
unsigned int convert_big_to_small_endian(unsigned int);
int strlen(const char *);
void* memcpy (void *, const void *, int);
void memset(char *, int, char);
void delay(size_t);

#endif