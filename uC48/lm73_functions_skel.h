// lm73_functions.h 
// Roger Traylor 11.27.10
//special defines and functions for the lm73 temperature sensor

//#define F_CPU 16000000UL
#define TRUE  1
#define FALSE 0

#include "twi_master.h"  //my TWCR_START, STOP, NACK, RNACK, SEND
//use the status codes in: usr/local/AVRMacPack/avr-3/include/util/twi.h

#define LM73_ADDRESS 0x90                    //LM73-0, address pin floating
#define LM73_WRITE (LM73_ADDRESS | TW_WRITE) //LSB is a zero to write
#define LM73_READ  (LM73_ADDRESS | TW_READ)  //LSB is a one to read
#define LM73_PTR_TEMP          0x00          //LM73 temperature address
#define LM73_PTR_CONFIG        0x01          //LM73 configuration address
#define LM73_PTR_CTRL_STATUS   0x04          //LM73 control and status register
#define LM73_CONFIG_VALUE0     0x60          //no pwr dwn, disbl alert, no one shot: config reg
#define LM73_CONFIG_VALUE1     0xE0          //no timeout, max resolution: for ctl/status reg

//special functions for lm73 temperature sensor
uint8_t  lm73_temp_convert(char temp_digits[], uint16_t lm73_temp, uint8_t f_not_c);
  
