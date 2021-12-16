extern unsigned char __allocator_init;
unsigned char *allocator_address = (unsigned char *)&__allocator_init;

int strcmp(const char *X, const char *Y) {
    while (*X) {
        if (*X != *Y)
            break;
        X++;
        Y++;
    }
    return *(const unsigned char *)X - *(const unsigned char *)Y;
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

void *simple_malloc(unsigned long size) {
    allocator_address += size;
}