/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"

#define	OSS_PRAXIS_ID		65538
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
	blob	diag;
} command_system_t;

#define	command_battery_code	3
typedef struct {
	word	threshold;
	byte	arm;
	byte	pin;
	byte	interval;
} command_battery_t;

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
} message_status_t;

#define	message_packet_code	2
typedef struct {
	lword	counter;
	byte	rssi;
	blob	payload;
} message_packet_t;

#define	message_battery_code	3
typedef struct {
	word	status;
	word	threshold;
	byte	arm;
	byte	pin;
	byte	interval;
} message_battery_t;


// ===================================
// End of automatically generated code 
// ===================================
