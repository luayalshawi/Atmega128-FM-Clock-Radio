//hd44780.h

//HD44780 command summary
/*
clear display 	                                0x01
Return cursor to home, and un-shift display 	0x02
move cursor right, don’t shift display 	        0x04
move cursor right, do shift display (left) 	0x05
move cursor right, don’t shift display  	0x06
move cursor right, do shift display (left) 	0x07
turn display off 	                        0x08
display on, cursor off, 	                0x0C
display on, cursor on, steady cursor 	        0x0E
display on, cursor on, blinking cursor 	        0x0F
shift cursor left 	                        0x10
shift cursor right 	                        0x14
shift display left 	                        0x18
shift display right 	                        0x1C
set cursor position                             0x80+position
*/

//some commands that are defined
#define SET_DDRAM_ADDR  0x80    // must "OR" in the address
#define RETURN_HOME     0x02
#define CLEAR_DISPLAY   0x01

//assumes timing specified at LCD Vdd=4.4-5.5v

#define CURSOR_VISIBLE 0 //Set 1 for visible cursor,  0 for invisible
#define CURSOR_BLINK   0 //Set 1 for blinking cursor, 0 for continuous

#define CMD_BYTE  0x00
#define CHAR_BYTE 0x01

//The hardware port configuration for 4-bit operation is assumed
//to be all on one port.  Control lines are assumed to be in the
//lower nibble.  The control lines may be in another order but
//must still be within the lower nibble on the same port. The data
//lines are asuumed to be in the upper nibble with MSB of LCD
//aligned with the port MSB. (bit 7 is MSB on both) Bit zero of
//LCD_PORT is unused.
#define LCD_PORT           PORTD
#define LCD_PORT_DDR       DDRD
#define LCD_CMD_DATA_BIT   1    //zero is command, one is data
#define LCD_RDWR_BIT       2    //zero is write,   one is read
#define LCD_STROBE_BIT     3    //active high strobe

//Set to the width of the display, assumption is a two line display
#define NUM_LCD_CHARS 16

//Set the the lcd interface mode. One indicates SPI mode, zero
//indicates 4-bit mode. SPI mode assumes lcd is write only and
//that the SPI port is already initalized. 4-bit mode initalizes
//the RW bit as write. Busy flag reads will set it to read for
//as long as the Busy bit is read.
#define SPI_MODE          1

void send_lcd(uint8_t cnd_or_char, uint8_t data);
void send_lcd_8bit(uint8_t cnd_or_char, uint8_t data, uint16_t wait);
void set_custom_character(uint8_t data[], uint8_t address);
void set_cursor(uint8_t row, uint8_t col);
void uint2lcd(uint8_t number);
void int2lcd(int8_t number);
void cursor_on(void);
void cursor_off(void);
void shift_right(void);
void shift_left(void);
void cursor_home(void);
void line1_col1(void);
void line2_col1(void);
void fill_spaces(void);
void string2lcd(char *lcd_str);
void strobe_lcd(void);
void clear_display(void);
void char2lcd(char a_char);
void lcd_init(void);
void refresh_lcd(char lcd_string_array[]);
void lcd_int32(int32_t l, uint8_t fieldwidth, uint8_t decpos, uint8_t bSigned, uint8_t bZeroFill);
void lcd_int16(int16_t l, uint8_t fieldwidth, uint8_t decpos, uint8_t bZeroFill);
void set_DDRAM_addr16(void);
void set_DDRAM_addr0(void);
