// twi_master.c  
// R. Traylor
// 11.07.2011
// twi_master code   

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/twi.h>
#include <stdlib.h>
#include "twi_master.h"

#define ZERO  0x00
#define ONE   0x01

volatile uint8_t  *twi_buf;      //pointer to the buffer we are xferred from/to
volatile uint8_t  twi_msg_size;  //number of bytes to be xferred
volatile uint8_t  twi_bus_addr;  //address of device on TWI bus 
volatile uint8_t  twi_state;     //status of transaction  

//****************************************************************************
//This is the TWI ISR. Different actions are taken depending upon the value
//of the TWI status register TWSR.
//****************************************************************************/
ISR(TWI_vect){
  static uint8_t twi_buf_ptr;  //index into the buffer being used 

  switch (TWSR) {
    case TW_START:          //START has been xmitted, fall thorough
    case TW_REP_START:      //Repeated START was xmitted
      TWDR = twi_bus_addr;  //load up the twi bus address
      twi_buf_ptr = 0;      //initalize buffer pointer 
      TWCR = TWCR_SEND;     //send SLA+RW
      break;
    case TW_MT_SLA_ACK:     //SLA+W was xmitted and ACK rcvd, fall through 
    case TW_MT_DATA_ACK:                //Data byte was xmitted and ACK rcvd
      if (twi_buf_ptr < twi_msg_size){  //send data till done
        TWDR = twi_buf[twi_buf_ptr++];  //load next and postincrement index
        TWCR = TWCR_SEND;               //send next byte 
      }
      else{TWCR = TWCR_STOP;}           //last byte sent, send STOP 
      break;
    case TW_MR_DATA_ACK:                //Data byte has been rcvd, ACK xmitted, fall through
      twi_buf[twi_buf_ptr++] = TWDR;    //fill buffer with rcvd data
    case TW_MR_SLA_ACK:                 //SLA+R xmitted and ACK rcvd
      if (twi_buf_ptr < (twi_msg_size-1)){TWCR = TWCR_RACK;}  //ACK each byte
      else                               {TWCR = TWCR_RNACK;} //NACK last byte 
      break; 
    case TW_MR_DATA_NACK: //Data byte was rcvd and NACK xmitted
      twi_buf[twi_buf_ptr] = TWDR;      //save last byte to buffer
      TWCR = TWCR_STOP;                 //initiate a STOP
      break;      
    case TW_MT_ARB_LOST:                //Arbitration lost 
      TWCR = TWCR_START;                //initiate RESTART 
      break;
    default:                            //Error occured, save TWSR 
      twi_state = TWSR;         
      TWCR = TWCR_RST;                  //Reset TWI, disable interupts 
  }//switch
}//TWI_isr
//****************************************************************************

//*****************************************************************************
//Call this function to test if the TWI unit is busy transferring data. The TWI
//code uses the the interrupt enable bit (TWIE) to indicate if the TWI unit
//is busy or not.  This protocol must be maintained for correct operation.
//*****************************************************************************
uint8_t twi_busy(void){
  return (bit_is_set(TWCR,TWIE)); //if interrupt is enabled, twi is busy
}
//*****************************************************************************

//****************************************************************************
//Initiates a write transfer. Loads global variables. Sends START. ISR handles
//the rest.
//****************************************************************************
void twi_start_wr(uint8_t twi_addr, uint8_t *twi_data, uint8_t byte_cnt){

  while(twi_busy());                    //wait till TWI rdy for next xfer
  twi_bus_addr = (twi_addr & ~TW_READ); //set twi bus address, mark as write 
  twi_buf = twi_data;                   //load pointer to write buffer
  twi_msg_size = byte_cnt;              //load size of xfer 
  TWCR = TWCR_START;                    //initiate START
}

//****************************************************************************
//Initiates a read transfer. Loads global variables. Sends START. ISR handles
//the rest.
//****************************************************************************
void twi_start_rd(uint8_t twi_addr, uint8_t *twi_data, uint8_t byte_cnt){

  while(twi_busy());                   //wait till TWI rdy for next xfer
  twi_bus_addr = (twi_addr | TW_READ); //set twi bus address, mark as read  
  twi_buf = twi_data;                  //load pointer to write buffer
  twi_msg_size = byte_cnt;             //load size of xfer 
  TWCR = TWCR_START;                   //initiate START
}
//******************************************************************************
//                            init_twi                               
//
//Uses PD1 as SDA and PD0 as SCL
//10K pullups are present on the board
//For the alarm clock, an additional 4.7K resistor is also there for pullup
//******************************************************************************

void init_twi(){
  TWDR = 0xFF;     //release SDA, default contents
  TWSR = 0x00;     //prescaler value = 1
  TWBR = TWI_TWBR; //defined in twi_master.h 
}

