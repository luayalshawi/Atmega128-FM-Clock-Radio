// lab5.c
// Luay Alshawi
// Nov. 27.16
// 48uC 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "lm73_functions_skel.h"
#include "twi_master.h"
#include "uart_functions_m48.h"


//uart
volatile uint8_t  rcv_rdy;
char              rx_char;
uint8_t           send_seq=0;         //transmit sequence number

char received_chars[2];
char    string_array[16];  //holds a string to send to 128uC

//delclare the 2 byte TWI read and write buffers (lm73_functions_skel.c)
extern uint8_t lm73_wr_buf[2];//
extern uint8_t lm73_rd_buf[2];//

ISR(USART_RX_vect){
  //USART RX complete interrupt
  static  uint8_t  i;
  rx_char = UDR0;              //get character
  received_chars[i++]=rx_char;  //store in array
 //if entire string has arrived, set flag, reset index
  if(rx_char == '\0'){
    rcv_rdy=1;
    i=0;
  }
}

/***********************************************************************/
/*                                main                                 */
/***********************************************************************/
int main ()
{
  uint16_t lm73_temp;  //a place to assemble the temperature from the lm73

  uart_init();
  init_twi();//initalize TWI (twi_master.h)

  //set LM73 mode for reading temperature by loading pointer register
  //this is done outside of the normal interrupt mode of operation
  lm73_wr_buf[0] = LM73_PTR_TEMP;//load lm73_wr_buf[0] with temperature pointer address
  twi_start_wr(0x90,lm73_wr_buf,2);//start the TWI write process (twi_start_wr())
  sei();//enable interrupts to allow start_wr to finish

  while(1){          //main while loop
    if(rcv_rdy==1 )// if command recieved from uc128
    {
      twi_start_rd(0x90,lm73_rd_buf,10);//read temperature data from LM73 (2 bytes)  (twi_start_rd())
      _delay_us(200);    //wait for it to finish
      //now assemble the two bytes read back into one 16-bit value
      lm73_temp = lm73_rd_buf[0];//save high temperature byte into lm73_temp
      lm73_temp = lm73_temp << 8;//shift it into upper byte
      lm73_temp |= lm73_rd_buf[1]; //"OR" in the low temp byte to lm73_temp
      lm73_temp = lm73_temp >> 7; // shift right by 7 bits to convert binary to decimal value
      itoa(lm73_temp,string_array,10);//convert to string in array with itoa() from avr-libc
      //send result to the 128uC
      uart_puts(string_array);
      uart_putc('\0');
      //reset recived flag
      rcv_rdy = 0;
    }
  } //while
} //main
