//UART Functions 
//Roger Traylor 11.l6.11
//For controlling the UART and sending debug data to a terminal as an aid 
//in debugging. Note that RX and TX lines are marked relative to the device
//they are located on.

#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

//F_CPU should be set in Makefile, don't set it here.

#define USART_BAUDRATE 9600  
#define BAUDVALUE  ((F_CPU/(USART_BAUDRATE * 16UL)) - 1 )

#define USART1_BAUDRATE 9600  
#define BAUDVALUE_1  ((F_CPU/(USART1_BAUDRATE * 16UL)) - 1 )

#include <string.h>

char uart_tx_buf[40];      //holds string to send to crt
char uart_rx_buf[40];      //holds string that recieves data from uart

char uart1_tx_buf[40];     //holds string to send to crt
char uart1_rx_buf[40];     //holds string that recieves data from uart
//******************************************************************
//                        uart_putc
//
// Takes a character and sends it to USART0
//
void uart_putc(char data) {
    while (!(UCSR0A&(1<<UDRE0)));    // Wait for previous transmissions
    UDR0 = data;    // Send data byte
    while (!(UCSR0A&(1<<UDRE0)));    // Wait for previous transmissions
}
//******************************************************************

//******************************************************************
//                        uart1_putc
//
// Takes a character and sends it to USART1
//
void uart1_putc(char data) {
    while (!(UCSR1A&(1<<UDRE1)));    // Wait for previous transmissions
    UDR1 = data;    // Send data byte
    while (!(UCSR1A&(1<<UDRE1)));    // Wait for previous transmissions
}
//******************************************************************

//******************************************************************
//                        uart_puts
// Takes a string and sends each charater to be sent to USART0
//void uart_puts(unsigned char *str) {
void uart_puts(char *str) {
    int i = 0;               
    // Loop through string, sending each character
    while(str[i] != '\0') { 
        uart_putc(str[i]);
        i++;
    }
}
//******************************************************************

//******************************************************************
//                        uart1_puts
// Takes a string and sends each charater to be sent to USART0
//void uart_puts(unsigned char *str) {
void uart1_puts(char *str) {
    int i = 0;               
    // Loop through string, sending each character
    while(str[i] != '\0') { 
        uart1_putc(str[i]);
        i++;
    }
}
//******************************************************************

//******************************************************************
//                        uart_puts_p
// Takes a string in flash memory and sends each charater to USART0
//void uart_puts(unsigned char *str) {
void uart_puts_p(const char *str) {      
    // Loop through string, sending each character
    while(pgm_read_byte(str) != 0x00) { 
        uart_putc(pgm_read_byte(str++));
    }
}
//******************************************************************

//******************************************************************
//                            uart_init
//
//RXD0 is PORT E bit 0
//TXD0 is PORT E bit 1
//Jumpers J14 and J16 (mega128.1) or Jumpers J7 and J9 (mega128.2)
//must be in place for the MAX232 chip to get data.

void uart_init(){
//rx and tx enable, receive interrupt enabled, 8 bit characters
UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0); //INTERRUPTS ENABLED
//  UCSR0B |= (1<<RXEN0) | (1<<TXEN0);               //INTERRUPS DISABLED

//async operation, no parity,  one stop bit, 8-bit characters
  UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);
  UBRR0H = (BAUDVALUE >>8 ); //load upper byte of the baud rate into UBRR 
  UBRR0L =  BAUDVALUE;       //load lower byte of the baud rate into UBRR 

}
//******************************************************************

//******************************************************************
//                            uart1_init
//
//RXD0 is PORT E bit 0
//TXD0 is PORT E bit 1
//Jumpers J14 and J16 (mega128.1) or Jumpers J7 and J9 (mega128.2)
//must be in place for the MAX232 chip to get data.

void uart1_init(){
//rx and tx enable, receive interrupt enabled, 8 bit characters
//UCSR1B |= (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1); //INTERRUPTS ENABLED
  UCSR1B |= (1<<RXEN1) | (1<<TXEN1);               //INTERRUPS DISABLED

//async operation, no parity,  one stop bit, 8-bit characters
  UCSR1C |= (1<<UCSZ11) | (1<<UCSZ10);
  UBRR1H = (BAUDVALUE_1 >>8 ); //load upper byte of the baud rate into UBRR 
  UBRR1L =  BAUDVALUE_1;       //load lower byte of the baud rate into UBRR 

}
//******************************************************************

//******************************************************************
//                             uart_getc
//Modified to not block indefinately in the case of a lost byte
//
char uart_getc(void) {
  uint16_t timer = 0;

  while (!(UCSR0A & (1<<RXC0))) {
  timer++;
  if(timer >= 16000){ return(0);}
  //what should we return if nothing comes in?
  //return the data into a global variable
  //give uart_getc the address of the variable
  //return a -1 if no data comes back.
  } // Wait for byte to arrive
  return(UDR0); //return the received data
}
//******************************************************************

//******************************************************************
//                             uart1_getc
//Modified to not block indefinately in the case of a lost byte
//
char uart1_getc(void) {
  uint16_t timer = 0;

  while (!(UCSR1A & (1<<RXC1))) {
  timer++;
  if(timer >= 16000){ return(0);}
  //what should we return if nothing comes in?
  //return the data into a global variable
  //give uart_getc the address of the variable
  //return a -1 if no data comes back.
  } // Wait for byte to arrive
  return(UDR1); //return the received data
}
//usage examples:
//uart_puts(".");
//uart_puts("   ");
//uart_puts("strength = ");
//itoa((int)strength, str, 10);
//uart_puts(str);
//uart_puts("   ");
//uart_init();
//uart_putc('\n');
//uart_puts("*****************\n");
//uart_puts("wrote first byte: ");
//uart_puts(str);
//uart_putc('\n');
  
