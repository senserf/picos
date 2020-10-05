# 44 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"










# 40 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/include/stdarg.h"
typedef __builtin_va_list __gnuc_va_list;
# 102 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/include/stdarg.h"
typedef __gnuc_va_list va_list;
# 1 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/in430.h"



# 47 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/intrinsics.h"
void __nop (void);



void __dint (void);



void __eint (void);


unsigned int __read_status_register (void);


typedef unsigned int __istate_t;




__istate_t __get_interrupt_state (void);


void __write_status_register (unsigned int sr);






void __set_interrupt_state (__istate_t sv);


void *__read_stack_pointer (void);


void __write_stack_pointer (void *sp);


void __bic_status_register (unsigned int bits);


void __bis_status_register (unsigned int bits);



void __bic_status_register_on_exit (unsigned int bits);



void __bis_status_register_on_exit (unsigned int bits);



void *__builtin_frame_address (unsigned int level);



void *__builtin_return_address (unsigned int level);



void __delay_cycles (unsigned long int delay);


unsigned int __swap_bytes (unsigned int v);



unsigned int __get_watchdog_clear_value ();




void __set_watchdog_clear_value (unsigned int v);





void __watchdog_clear ();
# 145 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char IE1 __asm__("__" "IE1");









extern volatile unsigned char IFG1 __asm__("__" "IFG1");








extern volatile unsigned char ME1 __asm__("__" "ME1");






extern volatile unsigned char IE2 __asm__("__" "IE2");





extern volatile unsigned char IFG2 __asm__("__" "IFG2");





extern volatile unsigned char ME2 __asm__("__" "ME2");











extern volatile unsigned int WDTCTL __asm__("__" "WDTCTL");
# 242 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int MPY __asm__("__" "MPY");

extern volatile unsigned int MPYS __asm__("__" "MPYS");

extern volatile unsigned int MAC __asm__("__" "MAC");

extern volatile unsigned int MACS __asm__("__" "MACS");

extern volatile unsigned int OP2 __asm__("__" "OP2");

extern volatile unsigned int RESLO __asm__("__" "RESLO");

extern volatile unsigned int RESHI __asm__("__" "RESHI");

extern const volatile unsigned int SUMEXT __asm__("__" "SUMEXT");








extern const volatile unsigned char P1IN __asm__("__" "P1IN");

extern volatile unsigned char P1OUT __asm__("__" "P1OUT");

extern volatile unsigned char P1DIR __asm__("__" "P1DIR");

extern volatile unsigned char P1IFG __asm__("__" "P1IFG");

extern volatile unsigned char P1IES __asm__("__" "P1IES");

extern volatile unsigned char P1IE __asm__("__" "P1IE");

extern volatile unsigned char P1SEL __asm__("__" "P1SEL");


extern const volatile unsigned char P2IN __asm__("__" "P2IN");

extern volatile unsigned char P2OUT __asm__("__" "P2OUT");

extern volatile unsigned char P2DIR __asm__("__" "P2DIR");

extern volatile unsigned char P2IFG __asm__("__" "P2IFG");

extern volatile unsigned char P2IES __asm__("__" "P2IES");

extern volatile unsigned char P2IE __asm__("__" "P2IE");

extern volatile unsigned char P2SEL __asm__("__" "P2SEL");








extern const volatile unsigned char P3IN __asm__("__" "P3IN");

extern volatile unsigned char P3OUT __asm__("__" "P3OUT");

extern volatile unsigned char P3DIR __asm__("__" "P3DIR");

extern volatile unsigned char P3SEL __asm__("__" "P3SEL");


extern const volatile unsigned char P4IN __asm__("__" "P4IN");

extern volatile unsigned char P4OUT __asm__("__" "P4OUT");

extern volatile unsigned char P4DIR __asm__("__" "P4DIR");

extern volatile unsigned char P4SEL __asm__("__" "P4SEL");








extern const volatile unsigned char P5IN __asm__("__" "P5IN");

extern volatile unsigned char P5OUT __asm__("__" "P5OUT");

extern volatile unsigned char P5DIR __asm__("__" "P5DIR");

extern volatile unsigned char P5SEL __asm__("__" "P5SEL");


extern const volatile unsigned char P6IN __asm__("__" "P6IN");

extern volatile unsigned char P6OUT __asm__("__" "P6OUT");

extern volatile unsigned char P6DIR __asm__("__" "P6DIR");

extern volatile unsigned char P6SEL __asm__("__" "P6SEL");
# 382 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char U0CTL __asm__("__" "U0CTL");

extern volatile unsigned char U0TCTL __asm__("__" "U0TCTL");

extern volatile unsigned char U0RCTL __asm__("__" "U0RCTL");

extern volatile unsigned char U0MCTL __asm__("__" "U0MCTL");

extern volatile unsigned char U0BR0 __asm__("__" "U0BR0");

extern volatile unsigned char U0BR1 __asm__("__" "U0BR1");

extern const volatile unsigned char U0RXBUF __asm__("__" "U0RXBUF");

extern volatile unsigned char U0TXBUF __asm__("__" "U0TXBUF");
# 439 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char U1CTL __asm__("__" "U1CTL");

extern volatile unsigned char U1TCTL __asm__("__" "U1TCTL");

extern volatile unsigned char U1RCTL __asm__("__" "U1RCTL");

extern volatile unsigned char U1MCTL __asm__("__" "U1MCTL");

extern volatile unsigned char U1BR0 __asm__("__" "U1BR0");

extern volatile unsigned char U1BR1 __asm__("__" "U1BR1");

extern const volatile unsigned char U1RXBUF __asm__("__" "U1RXBUF");

extern volatile unsigned char U1TXBUF __asm__("__" "U1TXBUF");
# 496 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char I2CIE __asm__("__" "I2CIE");










extern volatile unsigned char I2CIFG __asm__("__" "I2CIFG");










extern volatile unsigned char I2CNDAT __asm__("__" "I2CNDAT");










extern volatile unsigned char I2CTCTL __asm__("__" "I2CTCTL");
# 551 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char I2CDCTL __asm__("__" "I2CDCTL");








extern volatile unsigned char I2CPSC __asm__("__" "I2CPSC");

extern volatile unsigned char I2CSCLH __asm__("__" "I2CSCLH");

extern volatile unsigned char I2CSCLL __asm__("__" "I2CSCLL");

extern volatile unsigned char I2CDRB __asm__("__" "I2CDRB");

extern volatile unsigned int I2CDRW __asm__("__" "I2CDRW");


extern volatile unsigned int I2COA __asm__("__" "I2COA");

extern volatile unsigned int I2CSA __asm__("__" "I2CSA");


extern const volatile unsigned int I2CIV __asm__("__" "I2CIV");
















extern const volatile unsigned int TAIV __asm__("__" "TAIV");

extern volatile unsigned int TACTL __asm__("__" "TACTL");

extern volatile unsigned int TACCTL0 __asm__("__" "TACCTL0");

extern volatile unsigned int TACCTL1 __asm__("__" "TACCTL1");

extern volatile unsigned int TACCTL2 __asm__("__" "TACCTL2");

extern volatile unsigned int TAR __asm__("__" "TAR");

extern volatile unsigned int TACCR0 __asm__("__" "TACCR0");

extern volatile unsigned int TACCR1 __asm__("__" "TACCR1");

extern volatile unsigned int TACCR2 __asm__("__" "TACCR2");
# 716 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern const volatile unsigned int TBIV __asm__("__" "TBIV");

extern volatile unsigned int TBCTL __asm__("__" "TBCTL");

extern volatile unsigned int TBCCTL0 __asm__("__" "TBCCTL0");

extern volatile unsigned int TBCCTL1 __asm__("__" "TBCCTL1");

extern volatile unsigned int TBCCTL2 __asm__("__" "TBCCTL2");

extern volatile unsigned int TBCCTL3 __asm__("__" "TBCCTL3");

extern volatile unsigned int TBCCTL4 __asm__("__" "TBCCTL4");

extern volatile unsigned int TBCCTL5 __asm__("__" "TBCCTL5");

extern volatile unsigned int TBCCTL6 __asm__("__" "TBCCTL6");

extern volatile unsigned int TBR __asm__("__" "TBR");

extern volatile unsigned int TBCCR0 __asm__("__" "TBCCR0");

extern volatile unsigned int TBCCR1 __asm__("__" "TBCCR1");

extern volatile unsigned int TBCCR2 __asm__("__" "TBCCR2");

extern volatile unsigned int TBCCR3 __asm__("__" "TBCCR3");

extern volatile unsigned int TBCCR4 __asm__("__" "TBCCR4");

extern volatile unsigned int TBCCR5 __asm__("__" "TBCCR5");

extern volatile unsigned int TBCCR6 __asm__("__" "TBCCR6");
# 849 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char DCOCTL __asm__("__" "DCOCTL");

extern volatile unsigned char BCSCTL1 __asm__("__" "BCSCTL1");

extern volatile unsigned char BCSCTL2 __asm__("__" "BCSCTL2");
# 908 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char SVSCTL __asm__("__" "SVSCTL");
# 928 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int FCTL1 __asm__("__" "FCTL1");

extern volatile unsigned int FCTL2 __asm__("__" "FCTL2");

extern volatile unsigned int FCTL3 __asm__("__" "FCTL3");
# 977 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned char CACTL1 __asm__("__" "CACTL1");

extern volatile unsigned char CACTL2 __asm__("__" "CACTL2");

extern volatile unsigned char CAPD __asm__("__" "CAPD");
# 1021 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int ADC12CTL0 __asm__("__" "ADC12CTL0");

extern volatile unsigned int ADC12CTL1 __asm__("__" "ADC12CTL1");

extern volatile unsigned int ADC12IFG __asm__("__" "ADC12IFG");

extern volatile unsigned int ADC12IE __asm__("__" "ADC12IE");

extern volatile unsigned int ADC12IV __asm__("__" "ADC12IV");








extern volatile unsigned int ADC12MEM0 __asm__("__" "ADC12MEM0");

extern volatile unsigned int ADC12MEM1 __asm__("__" "ADC12MEM1");

extern volatile unsigned int ADC12MEM2 __asm__("__" "ADC12MEM2");

extern volatile unsigned int ADC12MEM3 __asm__("__" "ADC12MEM3");

extern volatile unsigned int ADC12MEM4 __asm__("__" "ADC12MEM4");

extern volatile unsigned int ADC12MEM5 __asm__("__" "ADC12MEM5");

extern volatile unsigned int ADC12MEM6 __asm__("__" "ADC12MEM6");

extern volatile unsigned int ADC12MEM7 __asm__("__" "ADC12MEM7");

extern volatile unsigned int ADC12MEM8 __asm__("__" "ADC12MEM8");

extern volatile unsigned int ADC12MEM9 __asm__("__" "ADC12MEM9");

extern volatile unsigned int ADC12MEM10 __asm__("__" "ADC12MEM10");

extern volatile unsigned int ADC12MEM11 __asm__("__" "ADC12MEM11");

extern volatile unsigned int ADC12MEM12 __asm__("__" "ADC12MEM12");

extern volatile unsigned int ADC12MEM13 __asm__("__" "ADC12MEM13");

extern volatile unsigned int ADC12MEM14 __asm__("__" "ADC12MEM14");

extern volatile unsigned int ADC12MEM15 __asm__("__" "ADC12MEM15");








extern volatile unsigned char ADC12MCTL0 __asm__("__" "ADC12MCTL0");

extern volatile unsigned char ADC12MCTL1 __asm__("__" "ADC12MCTL1");

extern volatile unsigned char ADC12MCTL2 __asm__("__" "ADC12MCTL2");

extern volatile unsigned char ADC12MCTL3 __asm__("__" "ADC12MCTL3");

extern volatile unsigned char ADC12MCTL4 __asm__("__" "ADC12MCTL4");

extern volatile unsigned char ADC12MCTL5 __asm__("__" "ADC12MCTL5");

extern volatile unsigned char ADC12MCTL6 __asm__("__" "ADC12MCTL6");

extern volatile unsigned char ADC12MCTL7 __asm__("__" "ADC12MCTL7");

extern volatile unsigned char ADC12MCTL8 __asm__("__" "ADC12MCTL8");

extern volatile unsigned char ADC12MCTL9 __asm__("__" "ADC12MCTL9");

extern volatile unsigned char ADC12MCTL10 __asm__("__" "ADC12MCTL10");

extern volatile unsigned char ADC12MCTL11 __asm__("__" "ADC12MCTL11");

extern volatile unsigned char ADC12MCTL12 __asm__("__" "ADC12MCTL12");

extern volatile unsigned char ADC12MCTL13 __asm__("__" "ADC12MCTL13");

extern volatile unsigned char ADC12MCTL14 __asm__("__" "ADC12MCTL14");

extern volatile unsigned char ADC12MCTL15 __asm__("__" "ADC12MCTL15");
# 1280 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int DAC12_0CTL __asm__("__" "DAC12_0CTL");

extern volatile unsigned int DAC12_1CTL __asm__("__" "DAC12_1CTL");
# 1320 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int DAC12_0DAT __asm__("__" "DAC12_0DAT");

extern volatile unsigned int DAC12_1DAT __asm__("__" "DAC12_1DAT");






extern volatile unsigned int DMACTL0 __asm__("__" "DMACTL0");
# 1389 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int DMACTL1 __asm__("__" "DMACTL1");





extern volatile unsigned int DMA0CTL __asm__("__" "DMA0CTL");

extern volatile unsigned int DMA1CTL __asm__("__" "DMA1CTL");

extern volatile unsigned int DMA2CTL __asm__("__" "DMA2CTL");
# 1442 "/cygdrive/c/mspgcc/lib/gcc/msp430/4.6.3/../../../../msp430/include/msp430f1611.h"
extern volatile unsigned int DMA0SA __asm__("__" "DMA0SA");

extern volatile unsigned int DMA0DA __asm__("__" "DMA0DA");

extern volatile unsigned int DMA0SZ __asm__("__" "DMA0SZ");

extern volatile unsigned int DMA1SA __asm__("__" "DMA1SA");

extern volatile unsigned int DMA1DA __asm__("__" "DMA1DA");

extern volatile unsigned int DMA1SZ __asm__("__" "DMA1SZ");

extern volatile unsigned int DMA2SA __asm__("__" "DMA2SA");

extern volatile unsigned int DMA2DA __asm__("__" "DMA2DA");

extern volatile unsigned int DMA2SZ __asm__("__" "DMA2SZ");
# 219 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/supplement.h"
extern char *__bss_end;
# 17 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/arch.h"
typedef unsigned char Boolean;
typedef unsigned int word;
typedef int sint;
typedef int wint;
typedef long int lint;
typedef unsigned char byte;
typedef unsigned long lword;
typedef unsigned int aword;
typedef word *address;



typedef struct {

 byte pdmode:1,
  evntpn:1,
  fstblk:1,
  ledblk:1,
  ledsts:4;

 byte ledblc;

} systat_t;

extern volatile systat_t __pi_systat;
# 69 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/arch.h"
void __pi_release () __attribute__ ((noreturn));
# 491 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/mach.h"
void tci_run_delay_timer ();
void tci_run_auxiliary_timer ();
word tci_update_delay_ticks (Boolean);
# 24 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/uart_def.h"





typedef struct {
 word rate;
 byte A, B;
} uart_rate_table_entry_t;
# 515 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/mach.h"






typedef struct {






 volatile byte flags;
 byte out;



 byte in [32];
 byte ib_in, ib_out, ib_count;

} uart_t;









extern uart_t __pi_uart [];
# 19 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/sensors_sys.h"
typedef struct {

 word W [3];

} i_sensdesc_t;

typedef struct {

 byte tp,
  adcpars [3];
 word nsamples;

} a_sensdesc_t;

typedef struct {

 byte tp, num;
 void (*fun_val) (word, const byte*, address);
 void (*fun_ini) ();

} d_sensdesc_t;
# 16 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Sensors/sensors.h"

void read_sensor (word, sint, address);
# 13 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Sensors/analog_sensor.h"



void analog_sensor_read (word, const a_sensdesc_t*, address);
# 14 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/actuators_sys.h"
typedef struct {

 word W [3];

} i_actudesc_t;

typedef struct {

 byte tp, iref;
 word dacpars,
  interval;
} a_actudesc_t;

typedef struct {

 byte tp, num;
 void (*fun_val) (word, const byte*, address);
 void (*fun_ini) ();

} d_actudesc_t;
# 16 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Sensors/actuators.h"

void write_actuator (word, sint, address);
# 13 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Sensors/analog_actuator.h"



void analog_actuator_write (word, const a_actudesc_t*, word);
# 216 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/pins_sys.h"
typedef struct {
 byte poff, pnum;
} pind_t;










Boolean __pi_pin_available (word);
Boolean __pi_pin_adc_available (word);
word __pi_pin_ivalue (word);
word __pi_pin_ovalue (word);
Boolean __pi_pin_adc (word);
Boolean __pi_pin_output (word);
void __pi_pin_set (word);
void __pi_pin_clear (word);
void __pi_pin_set_input (word);
void __pi_pin_set_output (word);
void __pi_pin_set_adc (word);
# 265 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/pins_sys.h"
Boolean __pi_pin_dac_available (word);
Boolean __pi_pin_dac (word);
void __pi_clear_dac (word);
void __pi_set_dac (word);
void __pi_write_dac (word, word, word);
# 894 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/MSP430/mach.h"



extern lword __pi_nseconds;
# 368 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
void root (word state);

typedef void (*fsmcode)(word);

void __pi_wait (aword, word);
void __pi_trigger (aword), __pi_ptrigger (aword, aword);
aword __pi_fork (fsmcode func, aword data);
aword __pi_join (aword, word);
void reset (void) __attribute__ ((noreturn)) ;
void halt (void) __attribute__ ((noreturn)) ;

int __pi_strlen (const char*);
void __pi_strcpy (char*, const char*);
void __pi_strncpy (char*, const char*, int);
void __pi_strcat (char*, const char*);
void __pi_strncat (char*, const char*, int);
void __pi_memcpy (char *dest, const char *src, int);
void __pi_memset (char *dest, char c, int);

extern const char __pi_hex_enc_table [];


extern const lword host_id;



aword *__pi_malloc (word);
void __pi_free (aword*);
void __pi_waitmem (word);


word __pi_memfree (address);
word __pi_maxfree (address);
# 427 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
word __pi_stackfree (void);







void __pi_syserror (word, const char*) __attribute__ ((noreturn)) ;
# 551 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
void diag (const char *, ...);







lword lrnd (void);
# 579 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
sint io (word, word, word, char*, word);








void unwait (void);


void delay (word, word);
word dleft (aword);





void kill (aword);

void killall (fsmcode);






aword running (fsmcode);
word crunning (fsmcode);

fsmcode getcode (aword);

void proceed (word);

void setpowermode (word);



void utimer_add (address), utimer_delete (address);


void __pi_utimer_set (address, word);






void udelay (word);
void mdelay (word);
# 671 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
void __pi_badstate (void);
# 732 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
extern lword entropy;
# 775 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/sysio.h"
typedef struct {



 word State;
 aword Event;
} __pi_event_t;

struct __pi_pcb_s {








 word Status;
 word Timer;
 fsmcode code;
 aword data;
 __pi_event_t Events [4];
 struct __pi_pcb_s *Next;
};

typedef struct __pi_pcb_s __pi_pcb_t;

extern __pi_pcb_t *__pi_curr;
# 13 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Serial/ser.h"



int ser_out (word, const char*);
int ser_in (word, char*, int);
int ser_outb (word, const char*);
# 15 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Serial/form.h"
word __pi_vfparse (char*, word, const char*, va_list);
char *vform (char*, const char*, va_list);
int vscan (const char*, const char*, va_list);
char *form (char*, const char*, ...);
word fsize (const char*, ...);
int scan (const char*, const char*, ...);
# 14 "C:/cygwin64/home/nripg/SOFTWARE/PICOS/PicOS/PLibs/Serial/serf.h"



int ser_outf (word, const char*, ...);
int ser_inf (word, const char*, ...);
# 12 "app.cc"











word control_interval = 16,


 monitor_interval = 1024;






lint Kp = 0, Ki = 0, Kd = 0;


Boolean Active = 0;

lint setpoint,
 output,
 setting,
 error,
 previous_error,
 integral,
 derivative;



extern void controller (word);

void get_error (word st) {



 word val;


 read_sensor (st, 0, &val);


 do { if ((val) < (0)) (val) = (0); else if ((val) > (2047)) (val) = (2047); } while (0);

 output = (lint) val;


 error = setpoint - output;
}

void set_plant (word st) {



 word val = (word) setting;

 write_actuator (st, 0, &val);
}

void update_integral () {






 if (Ki == 0 || error == 0) {

  integral = 0;
  return;
 }
 if (previous_error > 0 && error < 0 ||
     previous_error < 0 && error > 0) {

  integral = error;
  return;
 }


 integral += error;
}

void update_derivative () {



 derivative = error - previous_error;
}

void calculate_new_input () {



 lint delta;



 delta = ((


  Kp * error +





  (Ki * integral * control_interval)/1024 +




  (Kd * derivative * 1024)/control_interval



  ) * (1023 - 512)) /



  ((lint)(2047 - 0) * 100);



 setting = 512 + delta;


 do { if ((setting) < (1)) (setting) = (1); else if ((setting) > (1023)) (setting) = (1023); } while (0);
}

Boolean start () {

 if (Active)
  return 0;

 previous_error = integral = output = 0;
 setting = 512;
 __pi_fork (controller, 0);
 __pi_trigger ((aword)(&monitor_interval));

 Active = 1;
 return 1;
}

Boolean stop () {

 if (Active) {
  killall (controller);
  Active = 0;
  return 1;
 }

 return 0;
}

void control_cycle (word st) {



 get_error (st);

 update_integral ();
 update_derivative ();
 calculate_new_input ();


 previous_error = error;
}




#define LOOP 0
#define GET_RESPONSE 1
# 188 "app.cc"
void controller (word __pi_st) { switch (__pi_st) { 
# 188 "app.cc"


 case LOOP : __stlab_LOOP: {

  set_plant (LOOP);
  delay (control_interval, GET_RESPONSE);
  __pi_release ();

 } case GET_RESPONSE : __stlab_GET_RESPONSE: {

  control_cycle (GET_RESPONSE);
  goto __stlab_LOOP;
break; } default: __pi_badstate (); } }
#undef LOOP
#undef GET_RESPONSE
# 200 "app.cc"



#define LOOP 0
# 202 "app.cc"
void monitor (word __pi_st) { switch (__pi_st) { 
# 202 "app.cc"


 case LOOP : __stlab_LOOP: {

  ser_outf (LOOP, "%lu : %ld %ld <- %ld "
     "[E: %ld, I: %ld, D: %ld]\r\n",
      __pi_nseconds,
      setting,
      setpoint,
      output,
      error,
      integral,
      derivative);

  if (Active)

   delay (monitor_interval, LOOP);


  __pi_wait ((aword)(&monitor_interval),LOOP);
break; } default: __pi_badstate (); } }
#undef LOOP



#define INIT 0
#define BANNER 1
#define UART_INPUT 2
#define BAD_COMMAND 3
#define ILLEGAL_PARAMETER 4
#define RUNNING_ALREADY 5
#define STOPPED_ALREADY 6
#define SET_PLANT 7
#define SHOW_PARAMS 8
#define RUN_CYCLE 9
# 226 "app.cc"
void root (word __pi_st) { switch (__pi_st) { 
# 226 "app.cc"


 static char cmd[64];

 case INIT : __stlab_INIT: {

  __pi_fork (monitor, 0);

 } case BANNER : __stlab_BANNER: {

  ser_out (BANNER,
   "Commands:\r\n"
   "  c v [set control interval]\r\n"
   "  m v [set logging frequency]\r\n"
   "  t v [set the setpoint]\r\n"
   "  a v [set the actuator]\r\n"
   "  p v [set Kp]\r\n"
   "  i v [set Ki]\r\n"
   "  d v [set Kd]\r\n"
   "  r [run] \r\n"
   "  s [stop]\r\n"
   "  v [view, show]\r\n"
   "  . [cycle step]\r\n"
  );

 } case UART_INPUT : __stlab_UART_INPUT: {

  lint a, b, c;

  ser_in (UART_INPUT, cmd, 64);

  switch (cmd [0]) {

      case 'c' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 16 || a > 32768)
    goto __stlab_ILLEGAL_PARAMETER;
   control_interval = (word) a;
   goto __stlab_UART_INPUT;

      case 'm' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a > 32768)
    goto __stlab_ILLEGAL_PARAMETER;

   monitor_interval = (word) a;
   __pi_trigger ((aword)(&monitor_interval));
   goto __stlab_UART_INPUT;

      case 't' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 0 || a > 2047)
    goto __stlab_ILLEGAL_PARAMETER;

   setpoint = (word) a;
   goto __stlab_UART_INPUT;

      case 'a' :


   a = 0;
   scan (cmd + 1, "%ld", &a);
   if (a < 1 || a > 1023)
    goto __stlab_ILLEGAL_PARAMETER;

   setting = a;
   goto __stlab_SET_PLANT;

      case 'p' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Kp = a;
   goto __stlab_UART_INPUT;

      case 'i' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Ki = a;
   goto __stlab_UART_INPUT;

      case 'd' :

   a = 0;
   scan (cmd + 1, "%ld", &a);
   Kd = a;
   goto __stlab_UART_INPUT;

      case 'r' :

   if (!start ())
    goto __stlab_RUNNING_ALREADY;
   goto __stlab_UART_INPUT;

      case 's' :

   if (!stop ())
    goto __stlab_STOPPED_ALREADY;
   goto __stlab_UART_INPUT;

      case 'v' :

   proceed (SHOW_PARAMS);

      case '.' :


   if (Active)
    goto __stlab_RUNNING_ALREADY;
   proceed (RUN_CYCLE);
  }

 } case BAD_COMMAND : __stlab_BAD_COMMAND: {

  ser_out (BAD_COMMAND, "bad command!\r\n");
  goto __stlab_BANNER;

 } case ILLEGAL_PARAMETER : __stlab_ILLEGAL_PARAMETER: {

  ser_out (ILLEGAL_PARAMETER, "llegal parameter!\r\n");
  goto __stlab_BANNER;

 } case RUNNING_ALREADY : __stlab_RUNNING_ALREADY: {

  ser_out (RUNNING_ALREADY, "running already!\r\n");
  goto __stlab_UART_INPUT;

 } case STOPPED_ALREADY : __stlab_STOPPED_ALREADY: {

  ser_out (STOPPED_ALREADY, "stopped already!\r\n");
  goto __stlab_UART_INPUT;

 } case SET_PLANT : __stlab_SET_PLANT: {

  set_plant (SET_PLANT);
  __pi_trigger ((aword)(&monitor_interval));
  proceed (UART_INPUT);

 } case SHOW_PARAMS : __stlab_SHOW_PARAMS: {

  ser_outf (SHOW_PARAMS,
         "c=%ld, m=%ld, t=%ld, a=%ld, p=%ld, i=%ld, d=%ld\r\n",
   control_interval,
   monitor_interval,
   setpoint,
   setting,
   Kp, Ki, Kd);

  proceed (UART_INPUT);

 } case RUN_CYCLE : __stlab_RUN_CYCLE: {

  control_cycle (RUN_CYCLE);
  goto __stlab_SET_PLANT;

break; } default: __pi_badstate (); } }
#undef INIT
#undef BANNER
#undef UART_INPUT
#undef BAD_COMMAND
#undef ILLEGAL_PARAMETER
#undef RUNNING_ALREADY
#undef STOPPED_ALREADY
#undef SET_PLANT
#undef SHOW_PARAMS
#undef RUN_CYCLE
# 391 "app.cc"

