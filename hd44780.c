//hd44780.c
//A set of useful functions for writing LCD characer displays
//that utilize the Hitachi HD44780 or equivalent LCD controller.
//This code also includes adaptation for 4 bit LCD interface.
//Right now support only exists or 2x16 displays.


//Original source by R. Traylor ~2006
//Refactored by R. Traylor 10.7.09
//New features added by Kirby Headrick 11.20.09
//lcd_send with delay added by William Dillon 10.8.10
//Refactored, and cleaned up again by R. Traylor 12.28.2011, 12.30.2014

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "hd44780.h"

#define NUM_LCD_CHARS 16

//If using an interrupt driven, one char at a time style lcd refresh
//for a 2x16 display, use this array to place anything to be displayed
char lcd_string_array[32];  //holds two strings to refresh the LCD

char  lcd_str[16];  //holds string to send to lcd

//-----------------------------------------------------------------------------
//                               send_lcd
//
// Sends a command or character data to the lcd. First argument of 0x00 indicates
// a command transfer while 0x01 indicates data transfer.  The next byte is the
// command or character byte.
//
// This is a low-level function usually called by the other functions but may
// be called directly to provide more control. Most commonly controlled
// commands require 37us to complete. Thus, this routine should be called no more
// often than every 37us.
//
// Commnads that require more time have delays built in for them.
//
void send_lcd(uint8_t cmd_or_char, uint8_t byte){

#if SPI_MODE==1
  SPDR = (cmd_or_char)? 0x01 : 0x00;  //send the proper value for intent
  while (bit_is_clear(SPSR,SPIF)){}   //wait till byte is sent out
  SPDR = byte;                        //send payload
  while (bit_is_clear(SPSR,SPIF)){}   //wait till byte is sent out
  strobe_lcd();                       //strobe the LCD enable pin
#else //4-bit mode
  if(cmd_or_char==0x01){LCD_PORT |=  (1<<LCD_CMD_DATA_BIT);}
  else                 {LCD_PORT &= ~(1<<LCD_CMD_DATA_BIT);}
  uint8_t temp = LCD_PORT & 0x0F;             //perserve lower nibble, clear top nibble
  LCD_PORT   = temp | (byte & 0xF0);  //output upper nibble first
  strobe_lcd();                       //send to LCD
  LCD_PORT   = temp | (byte << 4);    //output lower nibble second
  strobe_lcd();                       //send to LCD
#endif
}

//------------------------------------------------------------------
//                          refresh_lcd
//
//When called, writes one character to the LCD.  On each call it
//increments a pointer so that after 32 calls, the entire lcd is
//written. The calling program is responsible for not calling this
//function more often than every 80us which is the maximum amount
//of time it might take for any operation to take in this function.
//This includes the possibility for a movement to the next line.
//These functions are homeline() or homeline2.
//
//The array is organized as one array of 32 char locations to make
//the index handling easier.  To external functions that write into
//the array it will appear as two separate 16 location arrays by
//using an offset into the array address.
//
//        LCD display character index postions (2x16 display)
//  -----------------------------------------------------------------
//  |  0|  1|  2|  3|  4|  5|  6|  7|  8|  9| 10| 11| 12| 13| 14| 15|
//  -----------------------------------------------------------------
//  | 16| 17| 18| 19| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31|
//  -----------------------------------------------------------------
//
void refresh_lcd(char lcd_string_array[]) {

  static uint8_t i=0;           // index into string array
  static uint8_t null_flag=0;   // end of string flag

  if(lcd_string_array[i] == '\0') null_flag = 1;

  // if a null terminator is found clear the rest of the display
  if(null_flag) send_lcd(CHAR_BYTE, ' ');
  else send_lcd(CHAR_BYTE,lcd_string_array[i]);

  i++;   //increment to next character

  //delays are inserted to allow character to be written before moving
  //the cursor to the next line.
  if(i == 16)
  {
      // goto line2, 1st char
      _delay_us(40);
      line2_col1();
  }
  else if(i == 32)
  {
      // goto line1, 1st char
      _delay_us(40);
      line1_col1();
      null_flag=0;
      i=0;
  }
}//refresh_lcd
/***********************************************************************/

//-----------------------------------------------------------------------------
//                          set_custom_character
//
//Loads a custom character into the CGRAM (character generation RAM) of the LCD
//data is an 8 element array of uint8_t, the last 5 digits of each element
//define the on/off pattern of each row starting at the top, 1 is on (dark) and
//0 is off (light)
//Example of a music quarter note
//uint8_t custom_LCD0[8] = {
//    0x04,             |  #  |
//    0x06,             |  ## |
//    0x05,  Generates  |  # #|
//    0x05,  -------->  |  # #|
//    0x04,             |  #  |
//    0x1C,             |###  |
//    0x1C,             |###  |
//    0x00,             |     |
//    };
//
//Variable "address" is the location to store the character where valid
//addresses are 0x00 - 0x07 for a total of 8 custom characters (0x08 - 0x0F
//map to 0x00 - 0x07) to display a custom character just refer to it's
//address, such as send_lcd(CHAR_BYTE, 0x01, 1) or it can be insteretd into a string
//via a forward slash escape as in string2lcd("this is my music note \1")

void set_custom_character(uint8_t data[], uint8_t address){
    uint8_t i;
    send_lcd(CMD_BYTE, 0x40 + (address << 3)); _delay_us(40);  //only needs 37uS
    for(i=0; i<8; i++){
      send_lcd(CHAR_BYTE, data[i]); _delay_us(40); //each char byte takes 37us to execute
    }
}

//-----------------------------------------------------------------------------
//                          set_cursor
//
//Sets the cursor to an arbitrary potition on the screen, row is either 1 or 2
//col is a number form 0-15, counting from left to right
void set_cursor(uint8_t row, uint8_t col){
    send_lcd(CMD_BYTE, 0x80 + col + ((row-1)*0x40));
}
//TODO: use this method of moving the cursor in the other cursor moving routines

//-----------------------------------------------------------------------------
//                          uint2lcd
//
//Takes a 8bit unsigned and displays it in base ten on the LCD. Leading 0's are
//not displayed.
//TODO: optimize by removing the mod operators
//TODO: Should be renamed uint8_2lcd(). Also, implement a uint16_2lcd() function

void uint2lcd(uint8_t number){
    if  (number == 0)  {send_lcd(CHAR_BYTE, 0x30                ); }
    else{
      if(number >= 100){send_lcd(CHAR_BYTE, 0x30+number/100     ); }
      if(number >= 10) {send_lcd(CHAR_BYTE, 0x30+(number%100)/10); }
      if(number >= 1)  {send_lcd(CHAR_BYTE, 0x30+(number%10)    ); }
    }
}

//-----------------------------------------------------------------------------
//                          int2lcd
//
//Takes a 8bit signed and displays it in base ten on the LCD. Leading 0's are
//not displayed.
//
void int2lcd(int8_t number){
    //if < 0, print minus sign, then take 2's complement of number and display
    if(number < 0){send_lcd(CHAR_BYTE, '-'); _delay_us(40); uint2lcd(~number+1);}
    else          {uint2lcd(number);                                            }
}

//-----------------------------------------------------------------------------
//                          cursor_on
//
//Sets the cursor to display
void cursor_on(void){send_lcd(CMD_BYTE, 0x0E);}

//-----------------------------------------------------------------------------
//                          cursor_off
//
//Turns the cursor display off
void cursor_off(void){send_lcd(CMD_BYTE, 0x0C);}

//-----------------------------------------------------------------------------
//                          shift_right
//
//shifts the display right one character
void shift_right(void){send_lcd(CMD_BYTE, 0x1E);}

//-----------------------------------------------------------------------------
//                          shift_left
//
//shifts the display left one character
void shift_left(void){send_lcd(CMD_BYTE, 0x18);}

//-----------------------------------------------------------------------------
//                          strobe_lcd
//Strobes the "E" pin on the LCD module. How this is done depends on the interface
//style. The 4-bit mode is presently kludged with nop statements to make a suitable
//pulse width for a 4 Mhz clock.
//TODO: make number of nops executed dependent on F_CPU, not hardcoded
//
void strobe_lcd(void){
#if SPI_MODE==1
 PORTF |=  0x08; PORTF &= ~0x08; //toggle port F bit 3, LCD strobe trigger
#else
//4-bit mode below
 LCD_PORT |= (1<<LCD_STROBE_BIT);           //set strobe bit
 asm("nop"); asm("nop"); asm("nop"); asm("nop"); //4 cycle wait
 LCD_PORT &= ~(1<<LCD_STROBE_BIT);          //clear strobe bit
 asm("nop"); asm("nop"); asm("nop"); asm("nop"); //4 cycle wait
#endif
}

//-----------------------------------------------------------------------------
//                          clear_display
//
//Clears entire display and sets DDRAM address 0 in address counter. Requires
//1.8ms for execution. Use only if you can withstand the big delay.
//
void clear_display(void){
  send_lcd(CMD_BYTE, CLEAR_DISPLAY);
  _delay_us(1800);   //1.8ms wait for LCD execution
}

//-----------------------------------------------------------------------------
//                          cursor_home()
//
//Sets DDRAM address 0 in address counter. Also returns display from being
//shifted to original position.  DDRAM contents remain unchanged. Requires
//1.5ms to execute. Use only if you can withstand the big delay. Consider
//using line1_col1().
//
void cursor_home(void){
  send_lcd(CMD_BYTE, RETURN_HOME);
  _delay_us(1500);  //1.5ms wait for LCD execution
  }

//-----------------------------------------------------------------------------
//                          line2_col1()
//
//Put cursor at line 2, column 0 by directly maniuplating the DDRAM address
//pointer. 37us required for execution.
//
void line2_col1(void){
  //change DDRAM address to 40, first char in second row, executes in 37us
  send_lcd(CMD_BYTE, (SET_DDRAM_ADDR | 0x40));
}

//-----------------------------------------------------------------------------
//                          line1_col1()
//
//Put cursor at line 1, column 0 by directly maniuplating the DDRAM address
//pointer. 37us required for execution.
//
void line1_col1(void){
  //change DDRAM address to 0, first char in first row, executes in 37us
  send_lcd(CMD_BYTE,(SET_DDRAM_ADDR | 0x00));
}

//-----------------------------------------------------------------------------
//                          fill_spaces
//
//Fill an entire line with spaces.
void fill_spaces(void){
	uint8_t i;
	for (i=0; i<=(NUM_LCD_CHARS-1); i++){
		send_lcd(CHAR_BYTE, ' ');
                _delay_us(40);  //40us wait between characters
	}
}

//----------------------------------------------------------------------------
//                            char2lcd
//
//Send a single char to the LCD.
//usage: char2lcd('H');  // send an H to the LCD
//
void char2lcd(char a_char){send_lcd(CHAR_BYTE, a_char);}

//----------------------------------------------------------------------------
//                            string2lcd
//
//Send a ascii string to the LCD.
void string2lcd(char *lcd_str){
  uint8_t i;
  for (i=0; i<=(strlen(lcd_str)-1); i++){send_lcd(CHAR_BYTE, lcd_str[i]);
  _delay_us(40);  //execution takes 37us per character
  }
}

//----------------------------------------------------------------------------
//                            lcd_init
//
//Initalize the LCD
//
void lcd_init(void){
  _delay_ms(16);      //power up delay
#if SPI_MODE==1       //assumption is that the SPI port is intialized
  //TODO: kludge alert! setting of DDRF should not be here, but is probably harmless.
  DDRF=0x08;          //port F bit 3 is enable for LCD in SPI mode
  send_lcd(CMD_BYTE, 0x30); _delay_ms(7); //send cmd sequence 3 times
  send_lcd(CMD_BYTE, 0x30); _delay_ms(7);
  send_lcd(CMD_BYTE, 0x30); _delay_ms(7);
  send_lcd(CMD_BYTE, 0x38); _delay_ms(5);
  send_lcd(CMD_BYTE, 0x08); _delay_ms(5);
  send_lcd(CMD_BYTE, 0x01); _delay_ms(5);
  send_lcd(CMD_BYTE, 0x06); _delay_ms(5);
  send_lcd(CMD_BYTE, 0x0C + (CURSOR_VISIBLE<<1) + CURSOR_BLINK); _delay_ms(5);
#else //4-bit mode
  LCD_PORT_DDR = 0xF0                    | //initalize data pins
                 ((1<<LCD_CMD_DATA_BIT)  | //initalize control pins
                  (1<<LCD_STROBE_BIT  )  |
                  (1<<LCD_RDWR_BIT)    );
  //do first four writes in 8-bit mode assuming reset by instruction
  //command and write are asserted as they are initalized to zero
  LCD_PORT = 0x30; strobe_lcd(); _delay_ms(8);  //function set,   write lcd, delay > 4.1ms
  LCD_PORT = 0x30; strobe_lcd(); _delay_us(200);//function set,   write lcd, delay > 100us
  LCD_PORT = 0x30; strobe_lcd(); _delay_us(80); //function set,   write lcd, delay > 37us
  LCD_PORT = 0x20; strobe_lcd(); _delay_us(80); //set 4-bit mode, write lcd, delay > 37us
  //continue initalizing the LCD, but in 4-bit mode
  send_lcd(CMD_BYTE, 0x28); _delay_ms(7); //function set: 4-bit, 2 lines, 5x8 font
  //send_lcd(CMD_BYTE, 0x08, 5000);
  send_lcd(CMD_BYTE, 0x01); _delay_ms(7)  //clear display
  send_lcd(CMD_BYTE, 0x06);  _delay_ms(5) //cursor moves to right, don't shift display
  send_lcd(CMD_BYTE, 0x0C | (CURSOR_VISIBLE<<1) | CURSOR_BLINK); _delay_ms(5);
#endif
}


//************************************************************************
//                                lcd_int32
// Displays a 32-bit value at the current position on the display. If a
// fieldwidth is specified, the field will be cleared and the number right
// justified within the field.  If a decimal position is specified, the
// data divided by 10^decpos will be displayed with decpos decimal positions
// displayed.  If bSigned is YES, the data will be treated as signed data.
// If bZeroFill and fieldwidth are specified, the number will be displayed
// right justified in the field and all positions in the field to the left
// of the number will be filled with zeros.
// This code courtesy of David Naviaux.
//************************************************************************
// TODO: not yet tested
void  lcd_int32(int32_t l,          //number to display
                uint8_t fieldwidth, //width of the field for display
                uint8_t decpos,     //0 if no decimal point, otherwise
                uint8_t bSigned,    //non-zero if the number should be treated as signed
                uint8_t bZeroFill)  //non-zero if a specified fieldwidth is to be zero filled
{
      char    sline[NUM_LCD_CHARS+1];
      uint8_t i=0;
      char    fillch;
      ldiv_t  qr;

      qr.quot = l; // initialize the quotient

      if (bSigned){
        bSigned = (qr.quot<0);
        qr.quot = labs(qr.quot);
      }

      // convert the digits to the right of the decimal point
      if (decpos){
        for (; decpos ; decpos--){
          qr = ldiv(qr.quot, 10);
          sline[i++] = qr.rem + '0';
        }
        sline[i++] = '.';
      }

      // convert the digits to the left of the decimal point
      do{
          qr = ldiv(qr.quot, 10);
          sline[i++] = qr.rem + '0';
        }while(qr.quot);

      // fill the whole field if a width was specified
      if (fieldwidth){
        fillch = bZeroFill? '0': ' '; // determine the fill character
        for (; i<fieldwidth ; ){sline[i++] = fillch;}
      }

      // output the sign, if we need to
      if (bSigned){sline[i++] = '-';}

      // now output the formatted number
      do{send_lcd(CHAR_BYTE, sline[--i]); _delay_us(40);} while(i);

}

//***************************************************************************************
//                                        lcd_int16
//
// Displays a 16-bit value at the current position on the display.  If a fieldwidth
// is specified, the field will be cleard and the number right justified within the
// field.  If a decimal position is specified, the data divided by 10^decpos will be
// displayed with decpos decimal positions displayed.  If bZeroFill and fieldwidth
// are specified, the number will be displayed right justified in the field and all
// positions in the field to the left of the number will be filled with zeros.
// TODO: not yet tested
//
// IN:           l               - number to display
//               fieldwidth      - width of the field
//               decpos          - 0 if no decimal point, otherwise
//               bSigned         - non-zero if the number should be treated as signed
//               bZeroFill       - non-zero if a specified fieldwidth is to be zero filled
//
//**************************************************************************************
void    lcd_int16(int16_t l,
                  uint8_t fieldwidth,
                  uint8_t decpos,
                  uint8_t bZeroFill)
{
        char    sline[NUM_LCD_CHARS+1];
        uint8_t i=0;
        char    fillch;
        div_t   qr;
        uint8_t bSigned;

        // initialize the quotient
        qr.quot = l;

        if ( (bSigned=(qr.quot<0)) )
                qr.quot = -qr.quot;

        // convert the digits to the right of the decimal point
        if (decpos){
          for (; decpos ; decpos--){
            qr = div(qr.quot, 10);
            sline[i++] = qr.rem + '0';
          }
          sline[i++] = '.';
        }

        // convert the digits to the left of the decimal point
        do
        {
                qr = div(qr.quot, 10);
                sline[i++] = qr.rem + '0';
        }
        while(qr.quot);

        // add the sign now if we don't pad the number with zeros
        if (!bZeroFill && bSigned)
        {
                sline[i++] = '-';
                bSigned = 0;
        }

        // fill the whole field if a width was specified
        if (fieldwidth){
          // determine the fill character
          fillch = bZeroFill? '0': ' ';
          for (; i<(fieldwidth-bSigned) ; ){ sline[i++] = fillch;}
        }

        // output the sign, if we need to
        if (bSigned){sline[i++] = '-';}

        // now output the formatted number
            do{send_lcd(CHAR_BYTE, sline[--i]); _delay_us(40);} while(i);
}
