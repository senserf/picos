/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_as3932_h
#define	__pg_as3932_h

#include "pins.h"

#ifndef	AS3932_NBYTES
// Bytes of data
#define	AS3932_NBYTES		4
#endif

#ifndef	AS3932_CRCVALUE
// No CRC byte
#define	AS3932_CRCVALUE		(-1)
#endif

// ============================================================================
// ============================================================================

// How many bytes to actually expect
#if AS3932_CRCVALUE >= 0
#define	AS3932_DATASIZE		(AS3932_NBYTES + 1)
#else
#define	AS3932_DATASIZE		AS3932_NBYTES
#endif

// Request mode codes
#define	AS3932_MODE_WRITE	0x00
#define	AS3932_MODE_READ	0x40
#define	AS3932_MODE_CMD		0xC0

// Command codes
#define	AS3932_CMD_CWAKE	0xC0	// Back to listening mode
#define	AS3932_CMD_RSRST	0xC1	// Reset RSS measurement
#define	AS3932_CMD_OSTRM	0xC2	// Start oscillator trim
#define	AS3932_CMD_CFALS	0xC3	// Reset false wakeup register
#define	AS3932_CMD_DEFAU	0xC4	// All registers to default

typedef struct {
	byte	val [AS3932_NBYTES];
} as3932_data_t;

#ifndef __SMURPH__

#include "as3932_sys.h"
//+++ "as3932.c"

void as3932_init (void);
void as3932_read (word, const byte*, address);

extern byte as3932_status, as3932_bytes [];

#define	AS3932_STATUS_RUNNING	0x01
#define	AS3932_STATUS_ON	0x02
#define	AS3932_STATUS_WAIT	0x04
#define	AS3932_STATUS_EVENT	0x08
#define	AS3932_STATUS_DATA	0x10
#define	AS3932_STATUS_BOUNDARY	0x20
#define	AS3932_STATUS_ABSENT	0x80

#endif

#ifdef	__SMURPH__

#define as3932_on()		( emul (12, "AS3932_ON"), YES)
#define as3932_off()		emul (12, "AS3932_OFF")

#else

Boolean as3932_on ();
void as3932_off ();
void as3932_clearall (byte);
Boolean as3932_addbit ();

#ifdef	AS3932_REGACCESS
#define	as3932_static
byte as3932_rreg (byte);
void as3932_wcmd (byte);
void as3932_wreg (byte, byte);
#else
#define	as3932_static	static
#endif

#endif

#endif
