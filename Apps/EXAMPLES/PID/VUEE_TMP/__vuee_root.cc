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
#include "sysio.h"
#include "board.h"
#include "board.cc"
void __build_node (data_no_t*);
process Root : BoardRoot {
void buildNode (const char *tp, data_no_t *nddata) {
if (tp == NULL || strlen (tp) == 0) __build_node (nddata); else  excptn ("Illegal named node type in data file: %s", tp);
};
};
