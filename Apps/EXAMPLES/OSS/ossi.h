#include "sysio.h"

#define	OSS_PRAXIS_ID		65537
#define	OSS_UART_RATE		9600
#define	OSS_PACKET_LENGTH	82

// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

typedef	struct {
	byte size;
	byte content [];
} blob;

typedef	struct {
	byte code, ref;
} oss_hdr_t;

// ==================
// Command structures
// ==================

typedef struct {
	byte	status;
	byte	power;
	word	channel;
	word	interval [2];
	word	length [2];
	blob	data;
} command_radio_t;

typedef struct {
	word	spin;
	word	leds;
	byte	request;
	byte	blinkrate;
	byte	power;
	blob	diag;
} command_system_t;

// ==================
// Message structures
// ==================

typedef struct {
	byte	rstatus;
	byte	rpower;
	byte	rchannel;
	word	rinterval [2];
	word	rlength [2];
	word	smemstat [3];
	byte	spower;
} message_status_t;

typedef struct {
	lword	counter;
	byte	rssi;
	blob	payload;
} message_packet_t;


// ===================================
// End of automatically generated code 
// ===================================
