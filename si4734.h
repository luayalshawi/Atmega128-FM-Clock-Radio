//Si4734 Addresses on the I2C bus
#define SI4734_ADDRESS                0x22   //fixed by Silicon Labs

//property definitions
#define GPO_IEN                       0x0001
#define GPO_IEN_STCIEN                0x0001
#define GPO_IEN_CTSIEN                0x0080
#define AM_SOFT_MUTE_MAX_ATTENUATION  0x3302
#define AM_PWR_LINE_NOISE_REJT_FILTER 0x0100
#define AM_CHANNEL_FILTER             0x3102
#define AM_CHFILT_6KHZ                0x0000
#define AM_CHFILT_4KHZ                0x0001
#define AM_CHFILT_3KHZ                0x0002
#define AM_CHFILT_2KHZ                0x0003
#define AM_CHFILT_1KHZ                0x0004

//command definitions
#define FM_TUNE_FREQ    0x20
#define FM_PWR_UP       0x01
#define AM_PWR_UP       0x01
#define AM_TUNE_FREQ    0x40
#define PWR_DOWN        0x11
#define SET_PROPERTY    0x12
#define GET_INT_STATUS  0x14
#define FM_TUNE_STATUS_IN_INTACK 0x01
#define FM_TUNE_STATUS  0x22
#define FM_RSQ_STATUS_IN_INTACK 0x01
#define FM_RSQ_STATUS   0x23
#define AM_TUNE_STATUS_IN_INTACK 0x01
#define AM_TUNE_STATUS  0x42
#define AM_RSQ_STATUS   0x43
#define AM_RSQ_STATUS_IN_INTACK 0x01
#define GET_REV         0x10

#define FALSE           0x00
#define TRUE            0x01

//si4734.c function prototypes
uint8_t get_int_status();
void    fm_tune_freq();
void    am_tune_freq();
void    sw_tune_freq();
void    fm_tune_status();
void    fm_rsq_status();
void    am_tune_status();
void    am_rsq_status();
void    fm_pwr_up();
void    am_pwr_up();
void    sw_pwr_up();
void    radio_pwr_dwn();
void    set_property();
void    get_rev();
void    get_fm_rsq_status();
