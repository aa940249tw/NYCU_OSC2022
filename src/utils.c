#include "utils.h"

unsigned char *allocator_address = (unsigned char *)allocator_init;

int strcmp(const char *X, const char *Y) {
    while (*X) {
        if (*X != *Y)
            break;
        X++;
        Y++;
    }
    return *(const unsigned char *)X - *(const unsigned char *)Y;
}

int strncmp(const char *str1, const char *str2, unsigned int n){
    while ((*str1 || *str2) && n > 0){
        if (*str1 == *str2){
            str1++;
            str2++;
            n--;
        }
        else return *str1 - *str2;
    }
    return 0;
}

char *strncpy(char *dest, const char *src, unsigned int n){
    char *addr = dest;

    while (*src && n > 0){
        *dest++ = *src++;
        n--;
    }
    *dest = '\0';
    return addr;
}

static int to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    return 0;
}

int hex_to_int(char *s, int len) {
    int sum = 0;
    for(int i = 0; i < len; ++i) {
        sum <<= 4;
        sum += to_int(*(s+i));
    }
    return sum;
}

char *simple_malloc(unsigned long size) {
    char *tmp = (char *)allocator_address;
    allocator_address += size;
    return tmp;
}

unsigned int convert_big_to_small_endian(unsigned int num){
    return (num>>24 & 0xff) | (num<<8 & 0xff0000) | (num>>8 & 0xff00) | (num<<24 & 0xff000000);
}

int strlen(const char* s){
    int len = 0;
    while(s[len] != '\0'){
        len++;
    }
    return len;
}

void memset(char *buf, int size, char c) {
    for(int i = 0; i < size; i++) {
        buf[i] = c;
    }
}

void* memcpy (void *dest, const void *src, int len) {
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}