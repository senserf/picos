/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

#define	OSS_PRAXIS_ID		65570
#define	OSS_UART_RATE		115200
#define	OSS_PACKET_LENGTH	56

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

#define	command_oximeter_code	1
typedef struct {
	word	mode;
	blob	regs;
} command_oximeter_t;

#define	command_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} command_ap_t;

#define	command_registers_code	2
typedef struct {
	blob	regs;
} command_registers_t;

#define	command_status_code	3
typedef struct {
} command_status_t;

#define	command_radio_code	4
typedef struct {
	word	delay;
} command_radio_t;

// ==================
// Message structures
// ==================

#define	message_report_code	1
typedef struct {
	word	time;
	byte	seqn;
	byte	sigs [5];
	word	vals [20];
} message_report_t;

#define	message_ap_code	128
typedef struct {
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
} message_ap_t;

#define	message_registers_code	2
typedef struct {
	blob	regs;
} message_registers_t;

#define	message_status_code	3
typedef struct {
	lword	uptime;
	word	oxistat;
	word	delay;
	word	battery;
	word	freemem;
	word	minmem;
} message_status_t;


// ===================================
// End of automatically generated code 
// ===================================
