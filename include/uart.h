void uart_init();
void uart_send(unsigned int);
char uart_getc();
char uart_getc_raw();
void uart_puts(char *);
void uart_flush();
void uart_hex(unsigned int);
void printf(char *fmt, ...);
