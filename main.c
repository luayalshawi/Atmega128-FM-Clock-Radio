// lab6_Radio.c
// Luay Alshawi
// Nov. 27.16
// Eaxtra Credit - Change radio Freq
// Eaxtra Credit - Volume Control
// Eaxtra Credit - Signal Strength

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "hd44780.h"
#include "twi_master.h"
#include "lm73_functions_skel.h"
#include "uart_functions.h"
#include "si4734.h"

#define TRUE 1
#define FALSE 0
#define CW 1
#define CCW 2

//http://www.avrfreaks.net/forum/there-c-nop-function
#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)

// array to hold a value of segmentsToDigits value to be sent to LED
volatile uint8_t segments_arr[5];

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];

//array holds segments value
// index: 0 -> 0 in decimal shape
// index: 1 -> 1 in decimal shape
// ...
uint8_t segmentsToDigits[10] = {
  0b11000000,
  0b11001111,
  0b10100100,
  0b10110000,
  0b10011001,
  0b10010010,
  0b10000010,
  0b11111000,
  0b10000000,
  0b10011000
};
static uint16_t counter = 0x0000;
static uint8_t digitsON = 0x01;
static uint8_t i = 0x00;
static uint8_t ReadData = 0x00;
static uint8_t SentData = 0x00;
static uint8_t multiplier = 0x00;
static uint8_t temp = 0x00;
static uint8_t temp1 = 0x00;
static uint8_t temp2 = 0x00;
static uint8_t mode1 = 0x00;
static uint8_t mode2 = 0x00;

static uint8_t clockCounter = 0x00;
static uint8_t colon = 0x00;//0 oof, 1 enabled
static uint8_t seconds = 0x00;
static uint8_t minutes = 0x00;
static uint8_t hours = 0x00;

static uint8_t minutes_alarm = 0x00;
static uint8_t hours_alarm = 0x00;
static uint8_t snooze_counter = 0x00;
volatile uint8_t s_counter = 0x00;

//Display to lcd modes
//This is just to keep track whether to send a message to the lcd or not
//Values -- 0: not to send, 1: send, 2: clear it from screen
#define setAlrm 0
#define clockSetting 1
#define adc_result 2
#define alarm_on 3
#define temp 4
static uint8_t displayToLed[5];


//MODES and their index
#define m_clockTime 0 //mode to change clock timer
#define m_setAlarmTime 1 // mode to change alarm time
#define m_alarmStatus 2 // 0: disabled, 1: eneabled, 2: on beebing, 3: snooze
#define m_snooze 3 // keep track of snooze 0: no snooze, 1: snooze
#define m_hrs 4 // keep track of snooze 0: no snooze, 1: snooze
#define m_volume 5 // on: control volume mode
#define m_radio 6 // radio mode         //in main command will turn off a radio
#define m_alarm_tune_source 7 // radio mode         //in main command will turn off a radio
static uint8_t modes[8];



//ADC
static uint16_t adc_value = 0x0000;

//delclare the 2 byte TWI read and write buffers (lm73_functions_skel.c)
volatile uint8_t lm73_wr_buf[2];//................
volatile uint8_t lm73_rd_buf[2];//................
uint16_t lm73_temp;  //a place to assemble the temperature from the lm73
char    lcd_string_array[32];  //holds a string to refresh the LCD

//uart
volatile uint8_t  rcv_rdy;
char              rx_char;
uint8_t           send_seq=0;         //transmit sequence number
uint8_t           send_temp_command = 0;
char              lcd_str_array[16];  //holds string to send to lcd
char              tmp_string[16];


//radio
volatile uint16_t eeprom_fm_freq;//= 9990;
volatile uint16_t eeprom_am_freq;
volatile uint16_t eeprom_sw_freq;
volatile uint8_t  eeprom_volume; //=100;

volatile uint16_t current_fm_freq = 9990; //arg2, arg3: 99.9Mhz, 200khz steps
volatile uint16_t current_am_freq;
volatile uint16_t current_sw_freq;
volatile uint8_t  current_volume; //=100;
volatile current_radio_band;// = 0;
volatile uint8_t si4734_tune_status_buf[8];
volatile uint8_t STC_interrupt;
static uint8_t radio_flag = FALSE;
static uint8_t update_radio_freq = FALSE;
static uint16_t VolumeValue = 30000;
static uint16_t radio_freq = 9990;
static uint8_t read_radio_strenght_flag;
//******************************************************************************
// External interrupt 7 is on Port E bit 7. The interrupt is triggered on the
// rising edge of Port E bit 7.  The i/o clock must be running to detect the
// edge (not asynchronouslly triggered)
//******************************************************************************
ISR(INT7_vect){STC_interrupt = TRUE;}
/***********************************************************************/

//******************************************************************************
//                            chk_buttons
//Checks the state of the button number passed to it. It shifts in ones till
//the button is pushed. Function returns a 1 only once per debounced button
//push so a debounce and toggle function can be implemented at the same time.
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"
//Expects active low pushbuttons on PINA port.  Debounce time is determined by
//external loop delay times 12.
//http://www.avrfreaks.net/forum/issues-using-stepper-motor-encoder-avr
uint8_t chk_buttons(uint8_t button) {

  static uint16_t state[8] = {0,0,0,0,0,0,0,0};
    state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
    if (state[button] == 0xF000) return 1;
   return 0;
}
//***********************************************************************
//                            spi_init
//**********************************************************************
void spi_init(void){
  /* Run this code before attempting to write to the LCD.*/
  DDRF  |= 0x08;  //port F bit 3 is enable for LCD
  PORTF &= 0xF7;  //port F bit 3 is initially low

  //Port B data direction
  DDRB   = 0xF7;//output mode for SS, MOSI, SCLK

  //SPI control register
  SPCR   = (1<<SPE) | (1<<MSTR) | (0<<CPOL) | (0<<CPHA);//enable SPI,master mode, clk low on idle, leading edge sample

  //SPI status register
  SPSR   = (1<<SPI2X);//choose double speed operation
}//spi_init

void readTemperature()
{
  twi_start_rd(0x90,lm73_rd_buf,10);//................ //read temperature data from LM73 (2 bytes)  (twi_start_rd())
  //_delay_ms(2);    //wait for it to finish
//now assemble the two bytes read back into one 16-bit value
  lm73_temp = lm73_rd_buf[0];//................ //save high temperature byte into lm73_temp
  lm73_temp = lm73_temp << 8;//................ //shift it into upper byte
  lm73_temp |= lm73_rd_buf[1];//................ //"OR" in the low temp byte to lm73_temp
  lm73_temp = lm73_temp >> 7;
}
 //***********************************************************************
 //                            Read from Encoder
 //return 0 CC
 //return 1 CCW
 //return 2 neither
 //**********************************************************************
uint8_t endoderRotation(uint8_t encoder2)
{
   static uint8_t sw_table[] = {0,1,2,0,2,0,0,1,1,0,0,2,0,2,1,0};
   static uint8_t previous_encoder2;
   uint8_t sw_index = 0 ;
   uint8_t direction = 0;
   static uint8_t count = 0;
   sw_index = (previous_encoder2 << 2) | encoder2;
   direction = sw_table[sw_index];
   if(direction==CW){count++;}
   if(direction==CCW){count--;}
   if(encoder2==3)
   {
     if( (count > 1) && (count < 100) )
     {
       count = 0;

       return 0;
     }
     if( (count <= 0xFF) && (count > 0x90) )
     {
       count = 0;

       return 1;
     }

     count = 0;
   }
 previous_encoder2 = encoder2;
 return 2;
}
uint8_t endoderRotation2(uint8_t encoder2)
{
   static uint8_t sw_table[] = {0,1,2,0,2,0,0,1,1,0,0,2,0,2,1,0};
   static uint8_t previous_encoder2;
   uint8_t sw_index = 0 ;
   uint8_t direction = 0;
   static uint8_t count = 0;
   sw_index = (previous_encoder2 << 2) | encoder2;
   direction = sw_table[sw_index];
   if(direction==CW){count++;}
   if(direction==CCW){count--;}
   if(encoder2==3)
   {
     if( (count > 1) && (count < 100) )
     {
       count = 0;

       return 0;
     }
     if( (count <= 0xFF) && (count > 0x90) )
     {
       count = 0;

       return 1;
     }

     count = 0;
   }
 previous_encoder2 = encoder2;
 return 2;
}

//**********************************************************************
//                           setMode
// Based on button click the function will disable and enable modes
// buttton 0: disable and enable change time mode
// buttton 1: disable and enable change clock time mode
// buttton 2: disable and enable alarm mode
// buttton 3: disable and enable snooze
// buttton 4: change clock display from 24hrs to 12 hrs
//**********************************************************************
void setModeOnButtonClick()
{
  if( chk_buttons(0) )
  {
    //set clock time
    if(modes[m_clockTime] == 0)
    {
      if(modes[m_setAlarmTime] == 0)
      {
        modes[m_clockTime] = 1;
        displayToLed[clockSetting] = TRUE;
        SentData |= 0x01;
      }

    }else
    {
      modes[m_clockTime] = 0;
      displayToLed[clockSetting] = 2;//clear
      SentData ^= 0x01;
    }
  }

 if(chk_buttons(1))
  {
    // set alarm time
    if(modes[m_setAlarmTime] == 0)
    {
      if(modes[m_clockTime] == 0)
      {
        modes[m_setAlarmTime] = 1;
        SentData |= 0x02;
      }
    }else
    {
      modes[m_setAlarmTime] = 0;
      SentData ^= 0x02;
    }
  }

  if(chk_buttons(2))
   {
     // activate and deactivate alarm
     if(modes[m_alarmStatus] == 0)
     {

       modes[m_alarmStatus] = 1;//alarm is active
       SentData |= 0x04;

     }
     else
     {
       modes[m_alarmStatus] = 0;
       SentData ^= 0x04;
       displayToLed[alarm_on] = 2;//remove mesage from display

     }
   }
   if(chk_buttons(3))
    {
      // 2 means the alarm is beebing
      if(modes[m_alarmStatus] == 2)
      {
          modes[m_snooze] = 1;
          //SentData |= 0x08;
          //turn off tune of the alaram it's on. but keep it enable
          modes[m_alarmStatus] = 3;
          //clear alarm message
          displayToLed[alarm_on] = 2;
      }

    }
    if(chk_buttons(4))
     {
       // 2 means the alarm is beebing
       if(modes[m_hrs] == 1)
       {
         modes[m_hrs] = 0;
         SentData ^= 128;
       }
       else
       {
        modes[m_hrs] = 1;
        SentData ^= 128;
       }

     }
     if(chk_buttons(5))
     {
       //turn on radio
       if(modes[m_radio] == 0)
       {
         //in main command will turn on a radio
         modes[m_radio] = 1;
       }
       else if(modes[m_radio]==2)//means radios is active
       {
         //in main command will turn off a radio
         modes[m_radio]=3;
       }
     }
     if(chk_buttons(6))
     {
       if(modes[m_alarm_tune_source]==0)
       {
         modes[m_alarm_tune_source] = 1;
       }
       else
       {
         modes[m_alarm_tune_source] = 0;
       }
     }
     if(chk_buttons(7))
     {
       //modes[m_volume] = TRUE;
       if(modes[m_volume] == FALSE)
       {
         modes[m_volume] = TRUE;
       }
       else
       {
         modes[m_volume] = FALSE;
       }
     }
}
//**********************************************************************
//                           setupMode
// Read Encoders and buttons. Also setup the clock timer
//**********************************************************************

ISR(TIMER0_OVF_vect)
{

  //make PORTA an input port with pullups
  DDRA = 0x00;
  PORTA = 0xFF;
  //enable tristate buffer for pushbutton switches
  uint8_t tempB = PORTB;
  PORTB = 0x70;
  //delay to read button press
  _NOP();
  _NOP();

  //now check each button and increment the count as needed
  setModeOnButtonClick();


  //Start Sending to HC595 and Read from 165 using SPDR
  // DDRD = 1<<PD7;
  PORTC =1<<PC6; //send falling edge to regclk on HC595
  PORTC = 0<<PC6; //send falling edge to regclk on HC595


  // togle port e bit 6 for 165 to read from incoders
  PORTE = 0<<PE6;//0x00;
  PORTE = 1<<PE6;

  SPDR = SentData;// send to 590
  while (bit_is_clear(SPSR, SPIF)){}	// wait until its done sending
  ReadData = ~SPDR;// read from 165
  temp1 = ReadData>>2;// read from 165
  //End Sending to HC595 and Read from 165 using SPDR

  // Rotating the encoders will take effect here
  // Right rotation
  if(endoderRotation(ReadData)==0)
  {
    if(modes[m_clockTime])
    {
      minutes++;
      if(minutes >= 60)
      {
        minutes = 0;
      }
    }

    if(modes[m_setAlarmTime])
    {
      minutes_alarm ++;
      if(minutes_alarm  >= 60)
      {
        minutes_alarm  = 0;
      }
    }
    if(modes[m_volume] == TRUE)
    {
      VolumeValue -=1000;
      OCR3A =VolumeValue;
    }

  }
  // Left Rotation
  if(endoderRotation(ReadData)==1)
  {
    if(modes[m_clockTime])
    {
    minutes--;
    if(minutes >=  60)
      minutes = 0;
    }

    if(modes[m_setAlarmTime])
    {
      minutes_alarm --;
      if(minutes_alarm  >=  60)
        minutes_alarm  = 0;
    }

    if(modes[m_volume] == TRUE)
    {
      VolumeValue +=1000;
      OCR3A =VolumeValue;
    }



  }
  if(endoderRotation2(temp1)==0)
  {
    if(modes[m_clockTime])
    {
      hours++;
      if(hours > 23)
      {
        hours = 0;
      }
    }

    if(modes[m_setAlarmTime])
    {
      hours_alarm ++;
      if(hours_alarm  > 23)
      {
        hours_alarm  = 0;
      }
    }
    if(modes[m_radio] == 2)
    {
      radio_freq += 20;
      update_radio_freq = TRUE;

    }


  }
  // Left Rotation
  if(endoderRotation2(temp1)==1)
  {
    if(modes[m_clockTime])
    {
      hours--;
      if(hours >  23)
        hours = 0;
    }

    if(modes[m_setAlarmTime])
    {
      hours_alarm --;
      if(hours_alarm  >  23)
        hours_alarm  = 0;
    }
    if(modes[m_radio] == 2)
    {
      radio_freq -= 20;
      update_radio_freq = TRUE;
    }
  }

  //Clock setup
  //Since this interupts happen 128 times per second
  if(clockCounter >= 128)
  {
    clockCounter = 0;
    seconds = 1 + seconds;
    //we should toggle : on LED segments
    colon ^= 0x01;
    //snooze counter
    // if snooze enabled then let's increment this every second
    if(modes[m_snooze]==1)
    {
      s_counter = 1 + s_counter;
    }

    readTemperature();// read temperature
    displayToLed[temp] = 1;

    //we want to set True to send a command to uC48
    send_temp_command = TRUE;

    //current_fm_freq += 20;
    read_radio_strenght_flag = TRUE;
  }

  if(seconds >= 60)
  {
    seconds = 0;
    minutes = 1 + minutes;
  }
  if(minutes >= 60)
  {
    minutes = 0;
    hours = 1 + hours;
  }
  if(hours > 23)
  {
    hours = 0;
    minutes = 0;
    seconds = 0;
  }

  //check for the alarm
  // if alar mode is active then check if the alarm should go on
  if(modes[m_alarmStatus]==TRUE)
  {
    if( (minutes == minutes_alarm)  && (hours == hours_alarm) )
    {
      modes[m_alarmStatus] = 2;// alarm is on and is beebing
      displayToLed[alarm_on] = TRUE;
    }
  }


  //snooze
  if(s_counter ==10)
  {
    //snooze period is over
    //passed 10 seconds you can turn on the alarm again
    if(modes[m_snooze]==1 && modes[m_alarmStatus] == 3)
    {
      //snooze is over. need to click the snooze button again
      modes[m_snooze] = 0;

      //run the alarm
      modes[m_alarmStatus] = 2;
      //turnon_timer1A();
      displayToLed[alarm_on] = TRUE;
    }
    s_counter = 0;
  }


  //disable tristate buffer for pushbutton switches
  //tempB = PORTB;
  PORTB = tempB & 0x0F;
  //PORTB = 0x0F;

  refresh_lcd(lcd_string_array);

  //Clock incrementing
  clockCounter = 1 + clockCounter;



}

void setRadioSegments(uint16_t frequency)
{
  uint8_t zertotonine = frequency % 10;
  uint8_t tens = (frequency/10) % 10;
  uint8_t hundereds = (frequency/100) %10;
  uint8_t thousands = (frequency/1000);
  segments_arr[0] = segmentsToDigits[zertotonine];
  segments_arr[1] = segmentsToDigits[tens];
  segments_arr[2] = 0;
  segments_arr[3] = segmentsToDigits[hundereds];
  segments_arr[4] = segmentsToDigits[thousands];
}
void setTimeSegments(h,m)
{
  if(m < 10)
  {
    segments_arr[0] = segmentsToDigits[m];
    segments_arr[1] = segmentsToDigits[0];
  }else
  {
    uint8_t m_digit1 = m % 10;
    uint8_t m_digit2 = (m/10) % 10;
    segments_arr[0] = segmentsToDigits[m_digit1];
    segments_arr[1] = segmentsToDigits[m_digit2];
  }
  if(h < 10)
  {
    if( modes[m_hrs] == 1)
    {
      if(h == 0)
      {
        //display 12 Am
        segments_arr[3] = segmentsToDigits[2];
        segments_arr[4] = segmentsToDigits[1];

      }
    }else
    {
      uint8_t h_digit1 = h % 10;
      segments_arr[3] = segmentsToDigits[h_digit1];
      segments_arr[4] = segmentsToDigits[0];
    }

  }else
  {
    if(h >12 && modes[m_hrs])
    {
      //display 1-12
      // PM mode
      uint8_t h_digit1 = (h-12) % 10;
      uint8_t h_digit2 = ((h-12)/10) % 10;
      segments_arr[3] = segmentsToDigits[h_digit1];
      segments_arr[4] = segmentsToDigits[h_digit2];
    }else
    {
      uint8_t h_digit1 = h % 10;
      uint8_t h_digit2 = (h/10) % 10;
      segments_arr[3] = segmentsToDigits[h_digit1];
      segments_arr[4] = segmentsToDigits[h_digit2];
    }

  }
}
void timer2_pwm_init()
{
  //setup timer counter 2 as the pwm source
  TCCR2 = (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<COM20 |(1<<CS20));
  //fast pwm, inverting mode, no prescaling
  OCR2 = 128;
}
//Volume timer
void timer3A_init()
{
  // Fast pwm, set on compare match
  TCCR3A |= (1<<COM3A1) | (1<<COM3A0) | (1<<WGM31);
  TCCR3C = 0x00;
  OCR3A = VolumeValue;
  ICR3 = 0xFFF0;
}
ISR(TIMER1_COMPA_vect)
{
  PORTC ^= 1<<PC1;
}
void timer1A_init()
{
  //portc pin1 is the tune -- oscillator
  DDRC   |= 1<<DDC1;
  TIMSK |= (1<<OCIE1A);  //enable timer interrupt 1 on compare
  TCCR1A = 0x00;         //TCNT1, normal port operation
  TCCR1B |= (1<<WGM12);  //CTC, OCR1A = top, clk/64 (250kHz)
  TCCR1C = 0x00;         //no forced compare
  OCR1A = 0x00FF;        //(use to vary alarm frequency)
}
void turnoff_timer1A()
{
  TCCR1B =0x00;
  TCCR3B = 0x00;
  PORTC = 0x00;
  OCR1A = 0x0000;
  OCR3A = 0x0000;
}
void turnon_timer1A()
{
  TCCR1B |= (1<<CS11)|(1<<CS10);
  TCCR1B |= (1<<WGM12);
  TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS30);
  PORTC = 1<<PC1;
  OCR1A = 0x00FF;
  OCR3A = VolumeValue;
}
void turnon_timer3_volume()
{
  TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS30);
  OCR3A = VolumeValue;
}

//***********************************************************************
//                            adc_init
//**********************************************************************
void adc_init()
{
  //Initalize ADC and its ports
  DDRF  &= ~(_BV(DDF7)); //make port F bit 7 is ADC input
  PORTF &= ~(_BV(PF7));  //port F bit 7 pullups must be off

  ADMUX |= ( (1<<REFS0) | (0<<ADLAR) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0)); //single-ended, input PORTF bit 7, right adjusted, 10 bits

  ADCSRA |=( (1<<ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2) | (1<<ADIE) );
}
ISR(ADC_vect)
{
  adc_value = ADC;
  if(adc_value < 680)
  {
    OCR2 = 20;
  }
  else if(adc_value < 720)
  {
    OCR2 = 30;
  }
  else if(adc_value < 760)
  {
    OCR2 = 40;
  }
  else if(adc_value < 830)
  {
    OCR2 = 60;

  }
  else if(adc_value < 870)
  {
    OCR2 = 80;

  }
  else if(adc_value < 900)
  {
    OCR2 = 100;

  }
  else if(adc_value < 940)
  {
    OCR2 = 150;

  }
  else if(adc_value < 980)
  {
    OCR2 = 200;

  }
  else if(adc_value < 1023)
  {
    OCR2 = 255;

  }
}
ISR(USART0_RX_vect){
  //USART0 RX complete interrupt
  static  uint8_t  i;
  rx_char = UDR0;              //get character
  lcd_str_array[i++]=rx_char;  //store in array
 //if entire string has arrived, set flag, reset index
  if(rx_char == '\0'){
    rcv_rdy=1;
    i=0;
  }
}
void radio_init()
{
  DDRE  |= 0x04; //Port E bit 2 is active high reset for radio
  PORTE |= 0x04; //radio reset is on at powerup (active high)

  // hardware reset of Si4734
  PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
  DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
  PORTE |=  (1<<PE2); //hardware reset Si4734
  _delay_us(200);     //hold for 200us, 100us by spec
  PORTE &= ~(1<<PE2); //release reset
  _delay_us(30);      //5us required because of my slow I2C translators I suspect
                      //Si code in "low" has 30us delay...no explaination
  DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt

  EIMSK |= (1 << INT7);  //initialize interrupt pin
	EICRB |= (1 << ISC70);
}
//***********************************************************************************
//Main
//***********************************************************************************
uint8_t main()
{

  //timer counter 0 setup, running off i/o clock
  TCCR0 = (1<<CS00);//normal mode, no prescaling
  ASSR  =  (1<<AS0);//external osc TOSC 32HZ
  TIMSK |= (1<<TOIE0);//enable interrupts
  DDRC   |= 1<<DDC6 ;// used for HC595
  DDRC   |= 1<<DDC7 ;//used for HC595
  DDRE = 0xFF;//set direction to output
  DDRD = 0xFF;
  spi_init();  //initalize SPI port
  adc_init();//initalize ADC bit 7 in PORTF
  timer2_pwm_init();//pwm setup for bit 7
  // using timer counter 1 to toggle portc pin2 to make occilator thing for the tune
  timer1A_init();
  timer3A_init();//pwm volume PORTE bit 3

  char lcd_str_h[16];  //holds string to send to lcd
  char lcd_str_l[16];  //holds string to send to lcd
  lcd_init();
  uart_init();
  init_twi();//initalize TWI (twi_master.h)
  //set LM73 mode for reading temperature by loading pointer register
  //this is done outside of the normal interrupt mode of operation
  lm73_wr_buf[0] = LM73_PTR_TEMP;//load lm73_wr_buf[0] with temperature pointer address
  twi_start_wr(0x90,lm73_wr_buf,2);//start the TWI write process (twi_start_wr())
  clear_display();

  //enable five digits
  digitsON = 0x05;
  int i = 0;
  for (i = 0; i < 32; i++)
  {
    lcd_string_array[i]= ' ';
  }

  // radio_init();
  /*
  Radio setup
  */
  radio_init();
  turnon_timer3_volume();
  sei();//set enable interupt

  while(1){

      if(displayToLed[alarm_on]==TRUE)
      {
        //set_cursor(1,0);
        lcd_string_array[0] = 'A';
        lcd_string_array[1] = 'l';
        lcd_string_array[2] = 'a';
        lcd_string_array[3] = 'r';
        lcd_string_array[4] = 'm';
        lcd_string_array[5] = ' ';
        lcd_string_array[6] = 'A';
        lcd_string_array[7] = 'c';
        lcd_string_array[8] = 't';
        lcd_string_array[9] = 'i';
        lcd_string_array[10] = 'v';
        lcd_string_array[11] = 'e';
        //string2lcd("Alarm Active");
        displayToLed[alarm_on] = FALSE;
        if(modes[m_alarm_tune_source] ==0)
        {
          turnon_timer1A();//turn on alaram tune
        }else
        {
          turnon_timer3_volume();
          //_delay_ms(300);
          // //radio_pwr_dwn();
          fm_pwr_up(); //powerup the radio as appropriate
          current_fm_freq = radio_freq;
          fm_tune_freq();
        }
      }
      if(displayToLed[alarm_on] == 2)
      {
        lcd_string_array[0] = ' ';
        lcd_string_array[1] = ' ';
        lcd_string_array[2] = ' ';
        lcd_string_array[3] = ' ';
        lcd_string_array[4] = ' ';
        lcd_string_array[5] = ' ';
        lcd_string_array[6] = ' ';
        lcd_string_array[7] = ' ';
        lcd_string_array[8] = ' ';
        lcd_string_array[9] = ' ';
        lcd_string_array[10] = ' ';
        lcd_string_array[11] = ' ';
        //clear_display();
        displayToLed[alarm_on] = FALSE;
        if(modes[m_alarm_tune_source] ==0)
        {
          turnoff_timer1A();//turn off alarm tune
        }else
        {
          radio_pwr_dwn();
        }
      }

      if(displayToLed[temp] == 1)
      {
        //local temperature

        //set_cursor(2,0);
        itoa(lm73_temp,tmp_string,10);//convert to string in array with itoa() from avr-libc
        lcd_string_array[25] = tmp_string[0];
        lcd_string_array[26] = tmp_string[1];
        lcd_string_array[27] = 'C';
        lcd_string_array[28] = '-';
        // lcd_string_array[18] = tmp_string[2];
        //string2lcd(lcd_string_array);//send the string to LCD (lcd_functions)
        //char2lcd('C');
        displayToLed[temp] = 0;
      }

      // Display the recieved value from 48uc
      //**************  start rcv portion ***************
      if(rcv_rdy==1){
        //reset recieved ready flag
        lcd_string_array[29] = lcd_str_array[0];
        lcd_string_array[30] = lcd_str_array[1];
        lcd_string_array[31] = 'C';
        rcv_rdy=0;
      }

      if(send_temp_command)
      {
        //send to 48uC asking to send tempertutr value
        uart_putc('A');
        uart_putc('\0');
        send_temp_command = FALSE;


      }

      //bound a counter (0-5) to keep track of digit to display
      if(i ==digitsON)
      {
        i = 0;
      }

      // call a function to set hours and minutes
      if(modes[m_setAlarmTime])
      {
        setTimeSegments(hours_alarm,minutes_alarm);

      }else if(modes[m_radio]==2)
      {
        setRadioSegments(current_fm_freq);
      }else
      {
        setTimeSegments(hours,minutes);
      }
      PORTA = 0xFF;
      if(i == 0 )
      {
        PORTB = 0x00;
      }
      if(i==1 && digitsON > 0)
      {
        PORTB = 0x10;
      }
      if(i==2 && digitsON > 0)
      {
        PORTB = 0x20;
        if(colon==1)
        {
          segments_arr[i] = 0x00;
        }else
        {
          segments_arr[i] = 0xFF;
        }
      }
      if(i==3 && digitsON > 0)
      {
        PORTB = 0x30;

      }
      if(i==4 && digitsON > 0)
      {
        PORTB = 0x40;
      }
      //make PORTA an output
      DDRA = 0xFF;
      //send 7 segment code to LED segments
      PORTA = segments_arr[i];
      //update digit to display

      ADCSRA |= (1<<ADSC); //poke ADSC and start conversion


      i++;

      //Radio Modes
      if(modes[m_radio] == 1)
      {
        turnon_timer3_volume();
        fm_pwr_up(); //powerup the radio as appropriate
        current_fm_freq = radio_freq;
        fm_tune_freq();
        modes[m_radio] = 2;//means radio is on and active
      }
      if(modes[m_radio]==2)
      {
        if(update_radio_freq)
        {
          current_fm_freq = radio_freq;
          fm_tune_freq();
          update_radio_freq = FALSE;
        }

      }
      if(modes[m_radio]==3)
      {
        radio_pwr_dwn();
        modes[m_radio]=0;
      }

      if(read_radio_strenght_flag)
      {
        read_radio_strenght_flag = FALSE;
        //to read signal strength...
        while(twi_busy()){} //make sure TWI is not busy
        fm_rsq_status();
        //save tune status
        uint8_t rssi =  si4734_tune_status_buf[4];
        itoa(rssi,tmp_string,10);//convert to string in array with itoa() from avr-libc
        lcd_string_array[16] = tmp_string[0];
      }

  }//while
}//main
