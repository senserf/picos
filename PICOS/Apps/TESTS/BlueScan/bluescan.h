/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __bluescan_h__
#define	__bluescan_h__

//+++ "bluescan.cc"

// Note: older devices impose the limit of 16 characters for the "friendly
// name". For newer devices, the limit is 256 (or so says the Linkmatik
// manual). We stick to 16.
#define	BSCAN_NAMELENGTH	16	// Excluding the sentinel
#define	BSCAN_CACHESIZE		64	// Number of entries in the cache
#define	BSCAN_INTERVAL		(20*1024)
#define	BSCAN_RETENTION		4
#define	BSCAN_ILINELENGTH	64
#define	BSCAN_HBCREDITS		3	// Heartbeat credits

#ifdef	UART_B
#define	BT_UART	UART_B
#else
#define	BT_UART	UART_A
#endif

typedef struct {

	byte status;			// 1 +
	byte mac [6];			// 6 +
	char name [BSCAN_NAMELENGTH+1];	// 17	= 24

} bscan_item_t;

// 64 * 24 = 1536

// status = AXXXXXXX, where:
//
//	N 	= attention for the item
//	XXXXXXX = counter
//			  0 == empty (can be recycled if N == 0 as well)
//			> 0 == used
//
// Every bscan_int we do this:
//
//	- decrement counters that are > 0 if N is 0; if the counter becomes 0,
//	  set N)
//
//	- issue the scan command to the module
//
// The cache is filled by a separate FSM that just reads lines from the module
//

void bscan_start (Boolean (*)(const bscan_item_t*)), bscan_stop ();
extern bscan_item_t bscan_cache [];

#define	BSCAN_LIMIT	(bscan_cache + BSCAN_CACHESIZE)
#define	BSCAN_EVENT	(&bscan_cache)

#define	for_all_bscan_entries(p)	for ((p) = bscan_cache; \
						(p) < BSCAN_LIMIT; (p)++)

#define	bscan_pending(p)	((p)->status & 0x80)
#define	bscan_counter(p)	((p)->status &= 0x7f)

#endif
