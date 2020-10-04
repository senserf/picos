#ifndef __vueehdr_h
#ifndef THREADNAME
#define THREADNAME(a) a
#endif
#define SIZE_OF_SINT 2
#define SIZE_OF_AWORD 2
#define __TYPE_AWORD__ uint32_t
#define __TYPE_LINT__ int32_t
#define __TYPE_WINT__ int16_t
#define __TYPE_SINT__ int16_t
#define __TYPE_LWORD__ uint32_t
#define __TYPE_WORD__ uint16_t
#define INFO_FLASH 0
#define CODE_LONG_INTS 1
#define UART_TCV 0
#define UART_TCV_MODE UART_TCV_MODE_N
#define CC1000 0
#define CC1100 0
#define CC1350_RF 0
#define CC2420 0
#define DM2200 0
#define CC3000 0
#define ETHERNET_DRIVER 0
#define RADIO_OPTIONS 0
#define RADIO_USE_LEDS 0
#define UART_USE_LEDS 0
#define RADIO_CRC_MODE 0
#define DUMP_MEMORY 0
#define TCV_LIMIT_RCV 0
#define TCV_LIMIT_XMT 0
#define TCV_HOOKS 0
#define TCV_TIMERS 0
#define TCV_OPEN_CAN_BLOCK 0
#define TCV_MAX_DESC 8
#define TCV_MAX_PHYS 3
#define TCV_MAX_PLUGS 3
#define TARP_CACHES_MALLOCED 0
#define TARP_CACHES_TEST 0
#define TARP_DDCACHESIZE 20
#define TARP_SPDCACHESIZE 20
#define TARP_RTRCACHESIZE 10
#define TARP_COMPRESS 0
#define TARP_MAXHOPS 10
#define TARP_RTR 0
#define TARP_RTR_TOUT 1024
#define TARP_DEF_RSSITH 100
#define TARP_DEF_PXOPTS 0x7000
#define TARP_DEF_PARAMS 0xA3
#define DIAG_MESSAGES 2
#define dbg_level 0
#define DONT_INCLUDE_OPTIONS_SYS
#include "board.h"
station __NT : PicOSNode { 
char __attr_init_origin [0];
# 15 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
word __vattr_control_interval,__vattr_monitor_interval;

# 25 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
lint __vattr_Kp,__vattr_Ki,__vattr_Kd;

# 28 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
Boolean __vattr_Active;

# 30 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
lint __vattr_setpoint,__vattr_output,__vattr_setting,__vattr_error,__vattr_previous_error,__vattr_integral,__vattr_derivative;

# 220 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
char __vattr_root_cmd[64];

char __attr_init_end [0];
void __praxis_starter ();
void reset () { PicOSNode::reset (); };
void init (); 
};
#include "stdattr.h"
# 180 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
process controller : _PP_ (__NT) {
states { LOOP,GET_RESPONSE };
perform;
};
# 194 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
process monitor : _PP_ (__NT) {
states { LOOP };
perform;
};
# 218 "/home/nripg/SOFTWARE/PICOS/Apps/EXAMPLES/PID/app.cc"
process root : _PP_ (__NT) {
states { INIT,BANNER,UART_INPUT,BAD_COMMAND,ILLEGAL_PARAMETER,RUNNING_ALREADY,STOPPED_ALREADY,SET_PLANT,SHOW_PARAMS,RUN_CYCLE };
perform;
};
#endif
