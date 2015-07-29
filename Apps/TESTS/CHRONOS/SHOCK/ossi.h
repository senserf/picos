#include "sysio.h"

#define	OSS_PRAXIS_ID		65569
#define	OSS_UART_RATE		115200
#define	OSS_PACKET_LENGTH	56

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

#define	command_acconfig_code	1
typedef struct {
	blob	regs;
} command_acconfig_t;

#define	command_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} command_ap_t;

#define	command_accturn_code	2
typedef struct {
	lword	after;
	lword	duration;
	byte	what;
	byte	pack;
	word	interval;
} command_accturn_t;

#define	command_time_code	3
typedef struct {
	byte	time [6];
} command_time_t;

#define	command_radio_code	4
typedef struct {
	word	delay;
} command_radio_t;

#define	command_display_code	5
typedef struct {
	byte	what;
} command_display_t;

#define	command_getinfo_code	6
typedef struct {
	byte	what;
} command_getinfo_t;

// ==================
// Message structures
// ==================

#define	message_status_code	1
typedef struct {
	lword	uptime;
	lword	after;
	lword	duration;
	byte	accstat;
	byte	display;
	word	delay;
	word	battery;
	word	freemem;
	word	minmem;
	byte	time [6];
} message_status_t;

#define	message_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} message_ap_t;

#define	message_presst_code	2
typedef struct {
	lword	press;
	sint	temp;
} message_presst_t;

#define	message_accvalue_code	3
typedef struct {
	word	stat;
	sint	xx;
	sint	yy;
	sint	zz;
	char	temp;
} message_accvalue_t;

#define	message_accstats_code	4
typedef struct {
	lword	after;
	lword	duration;
	lword	nevents;
	lword	total;
	word	max;
	word	last;
	byte	on;
} message_accstats_t;

#define	message_accregs_code	5
typedef struct {
	blob	regs;
} message_accregs_t;

#define	message_accreport_code	6
typedef struct {
	byte	time [6];
	word	sernum;
	blob	data;
} message_accreport_t;


// ===================================
// End of automatically generated code 
// ===================================
