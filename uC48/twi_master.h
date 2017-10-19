//twi master functions header file: twi_master.h 
//Roger Traylor 11.7.2011
//special defines and functions for twi bus in interrupt mode 

//using status codes in: usr/local/AVRMacPack/avr-3/include/util/twi.h
//use my own defines for actions that are to be taken

#define TWI_TWBR 0x0C  //400khz TWI clock

//TODO: try these some day...
//set bit rate for TWI
//TWBR must be > 10 for correct operation
//TWPS must be 0, for prescaler = 1
//#define SCL_CLOCK  100000L
//#define TWI_TWBR ( ((F_CPU/SCL_CLOCK) -16) / 2  ) 

#define NO_INTERRUPTS  0

#if (NO_INTERRUPTS)

#define TWCR_START  0xA4   //send start condition  
#define TWCR_STOP   0x94   //send stop condition   
#define TWCR_RACK   0xC4   //receive byte and return ack to slave  
#define TWCR_RNACK  0x84   //receive byte and return nack to slave
#define TWCR_SEND   0x84   //pokes the TWINT flag in TWCR and TWEN

#else
#define TWCR_START  0xA5 //send START 
#define TWCR_SEND   0x85 //poke TWINT flag to send another byte 
#define TWCR_RACK   0xC5 //receive byte and return ACK to slave  
#define TWCR_RNACK  0x85 //receive byte and return NACK to slave
#define TWCR_RST    0x04 //reset TWI
#define TWCR_STOP   0x94 //send STOP,interrupt off, signals completion

#endif

#define TWI_BUFFER_SIZE 17  //SLA+RW (1 byte) +  16 data bytes (message size)

uint8_t twi_busy(void);
void    twi_start_wr(uint8_t twi_addr, uint8_t *twi_data, uint8_t byte_cnt);
void    twi_start_rd(uint8_t twi_addr, uint8_t *twi_data, uint8_t byte_cnt);
void    init_twi();




