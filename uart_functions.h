//UART Functions header file 
//Roger Traylor 11.l6.11
//For controlling the UART and sending debug data to a terminal
//as an aid in debugging.

void uart_putc(char data);
void uart_puts(char *str);
void uart_puts_p(const char *str);
void uart_init();
char uart_getc(void);

void uart1_putc(char data);
void uart1_puts(char *str);
void uart1_init();
char uart1_getc(void);
