#include "sysio.h"

#define	OSS_PRAXIS_ID		589826
#define	OSS_UART_RATE		9600
#define	OSS_PACKET_LENGTH	82

// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

typedef	struct {
	word size;
	byte content [];
} blob;

typedef	struct {
	byte code, ref;
} oss_hdr_t;

// ==================
// Command structures
// ==================

#define	command_radio_code	1
typedef struct {
	byte	status;
	byte	power;
	word	channel;
	word	interval [2];
	word	length [2];
	blob	data;
} command_radio_t;

#define	command_system_code	2
typedef struct {
	word	spin;
	word	leds;
	byte	request;
	byte	blinkrate;
	byte	power;
	byte	swtch;
	blob	diag;
} command_system_t;

#define	command_dht11_code	3
typedef struct {
	byte	duration;
} command_dht11_t;

// ==================
// Message structures
// ==================

#define	message_status_code	1
typedef struct {
	byte	rstatus;
	byte	rpower;
	byte	rchannel;
	word	rinterval [2];
	word	rlength [2];
	word	smemstat [3];
	byte	spower;
	byte	swtch;
} message_status_t;

#define	message_packet_code	2
typedef struct {
	lword	counter;
	byte	rssi;
	blob	payload;
} message_packet_t;

#define	message_dht11t_code	3
typedef struct {
	blob	times;
} message_dht11t_t;


// ===================================
// End of automatically generated code 
// ===================================