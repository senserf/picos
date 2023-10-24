/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_cc1350_h
#define	__pg_cc1350_h

#ifndef __SMURPH__

#include "kernel.h"
#include "rfleds.h"

// ============================================================================

#ifndef RADIO_BASE_BITRATE
// This determines the set, i.e., the base config file (from SmartRF Studio)
// to be applied. This will determine the actual range of other parameters
#define	RADIO_BASE_BITRATE	1
#endif

#ifndef	RADIO_DEFAULT_POWER
#define	RADIO_DEFAULT_POWER	7
#endif

#if RADIO_DEFAULT_POWER < 0 || RADIO_DEFAULT_POWER > 8
#error "S: RADIO_DEFAULT_POWER > 8!!!"
#endif

#ifndef	RADIO_DEFAULT_BITRATE
#define	RADIO_DEFAULT_BITRATE	3
#endif

#if RADIO_DEFAULT_BITRATE < 0 || RADIO_DEFAULT_BITRATE > 3
#error "S: RADIO_DEFAULT_BITRATE > 3"
#endif
//
// Rates are relative. I no longer mention the (intended) bps values because
// they were wrong. I will try to calibrate and give numbers here. For now,
// stay tuned.
//

#if RADIO_BASE_BITRATE == 0
// This is the special low-rate mode with max power
#if RADIO_DEFAULT_POWER != 8
#error "S: RADIO_BASE_BITRATE == 0 requires RADIO_DEFAULT_POWER == 8"
#endif 
#if RADIO_DEFAULT_BITRATE > 0
#error "S: RADIO_DEFAULT_BITRATE must be 0 with RADIO_BASE_BITRATE == 0"
#endif
#endif

#if RADIO_DEFAULT_POWER == 8
#ifdef CC1350_PATABLE
#error "S: No PATABLE can be defined for RADIO_DEFAULT_POWER == 8"
#endif
#endif

#if RADIO_BASE_BITRATE == 0
// There is just one option: fixed power, fixed rate
//+++ smartrf_settings_625b_p8.c
#ifdef CC1350_RATABLE
#error "S: No RATABLE can be defined for RADIO_BASE_BITRATE == 0"
// FIXME: we need it in the driver
#endif
#endif

// ============================================================================

#if RADIO_BASE_BITRATE == 1
#if RADIO_DEFAULT_POWER == 8
//+++ smartrf_settings_50k_p8.c
#else
//+++ smartrf_settings_50k_p7.c
#endif

#ifndef	CC1350_RATABLE
// The entry number 0 is not used for setting the rate, because rate #0 must be
// hardwired (and the rate is then not settable)
#define	CC1350_RATABLE 		{ \
					{ 0xF,  0x199A, 10000 }, \
					{ 0xB,	0x12C5, 9997 }, \
					{ 0x7,	0x2DE0, 38399 }, \
					{ 0xF,	0x8000, 50000 }, \
				}
#endif

#endif

// ============================================================================

#if RADIO_BASE_BITRATE == 2
#if RADIO_DEFAULT_POWER == 8
//+++ smartrf_settings_200k_p8.c
#else
//+++ smartrf_settings_200k_p7.c
#endif

#ifndef	CC1350_RATABLE
// This is dummy for now, the last enty works; first entry unused
#define	CC1350_RATABLE 		{ \
					{ 0xF,  0x10000, 200000 }, \
					{ 0xF,	0x10000, 200000 }, \
					{ 0xF,	0x18000, 200000 }, \
					{ 0xF,	0x20000, 200000 }, \
				}
#endif

#endif

// ============================================================================

#if RADIO_DEFAULT_POWER != 8

// This works for both BASE BITRATES (SmartRF Studio returns same values)

#ifndef	CC1350_PATABLE
// ============================================================================
// Power settings (from SmartRF Studio):
//
//	14dBm	-> AB3F, special override (different smartrf_settings.c)
//	12  	-> BC2B
//	11	-> 90E5
//	10	-> 58D8
//	 9  	-> 40D2
//	 8  	-> 32CE
//	 7  	-> 2ACB
//	 6  	-> 24C9
//	 5  	-> 20C8
//	 4  	-> 1844
//	 3  	-> 1CC6
//	 2  	-> 18C5
//	 1  	-> 16C4
//	 0  	-> 12C3
//     -10	-> 04C0
//
#define	CC1350_PATABLE		{ \
					0x04C0, \
					0x12C3, \
					0x18C5, \
					0x1844, \
					0x24C9, \
					0x32CE, \
					0x58D8, \
					0xBC2B, \
				}
#endif
#endif

// ============================================================================

// Channels have to be implemented by direct frequency adjustments; to be
// determined, but we will probably have just a few

#ifndef RADIO_DEFAULT_CHANNEL
#define	RADIO_DEFAULT_CHANNEL	0
#endif

#if RADIO_DEFAULT_CHANNEL < 0 || RADIO_DEFAULT_CHANNEL > 7
#error "S: RADIO_DEFAULT_CHANNEL > 7!!!"
#endif

#ifndef	RADIO_SYSTEM_IDENT
// Can be set to zero and means "use the value provided by SmartRF Studio";
// well, this is the value:
#define	RADIO_SYSTEM_IDENT	0x930B51DE
#endif

#ifndef	RADIO_DEFAULT_OFFDELAY
// The deafult linger time before turning off the radio after last xmit with
// RX off (msecs)
#define	RADIO_DEFAULT_OFFDELAY	256
#endif

#ifndef	RADIO_WOR_MODE
#define	RADIO_WOR_MODE		0
#endif

#if RADIO_WOR_MODE && !defined(CC1350_RATABLE)
#error "S: RADIO_WOR_MODE requires PATABLE and RATABLE"
#endif

#ifndef	RADIO_DEFAULT_WOR_INTERVAL
#define	RADIO_DEFAULT_WOR_INTERVAL	512
#endif

#ifndef	RADIO_DEFAULT_WOR_RSSI
#define	RADIO_DEFAULT_WOR_RSSI		(-111 + 128)
#endif

// ============================================================================

#define	RADIO_BITRATE_INDEX RADIO_DEFAULT_BITRATE

// Channel increment; for now, 1 channel == 1 MHz; this should depend on
// rate; FIXME
#ifndef	CC1350_BASEFREQ
#define	CC1350_BASEFREQ		868	// Megahertz
#endif

// ============================================================================
// Backoff + LBT ==============================================================
// ============================================================================

// The conditions will compile out as the actual arguments to gbackoff are
// always constants. If there's no interval, set the timer to the fixed (min)
// value, unless it is zero (in which case do nothing). Otherwise (an
// interval), generate a random number between min and max.
#define	gbackoff(min,max)  do { \
			     if ((max) <= (min)) { \
				if (min) \
				  utimer_set (bckf_timer, min); \
			     } else { \
				utimer_set (bckf_timer, (min) + \
		     		  (((lword)((max)-(min)+1) * rnd ()) >> 16)); \
			     } \
			   } while (0)

#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	2
#endif

#ifndef	RADIO_LBT_MAX_BACKOFF
#ifdef	RADIO_LBT_BACKOFF_EXP
// The legacy style
#define	RADIO_LBT_MAX_NBACKOFF	(RADIO_LBT_MIN_BACKOFF + \
					(1 << RADIO_LBT_BACKOFF_EXP) - 1)
#endif
#endif

#ifndef	RADIO_LBT_MAX_BACKOFF
#define	RADIO_LBT_MAX_BACKOFF	32
#endif

#ifndef	RADIO_RCV_MIN_BACKOFF
#define	RADIO_RCV_MIN_BACKOFF	2
#endif

#ifndef	RADIO_RCV_MAX_BACKOFF
#define	RADIO_RCV_MAX_BACKOFF	2
#endif

#ifndef	RADIO_XMT_MIN_BACKOFF
#ifdef	RADIO_LBT_XMIT_SPACE
// Legacy
#define	RADIO_XMT_MIN_BACKOFF	RADIO_LBT_XMIT_SPACE
#define	RADIO_XMT_MAX_BACKOFF	RADIO_LBT_XMIT_SPACE
#endif
#endif

// This should be preferably zero. No need to waste bandwidth for nothing, if
// the application doesn't care.
#ifndef	RADIO_XMT_MIN_BACKOFF
#define	RADIO_XMT_MIN_BACKOFF	0
#endif

#ifndef	RADIO_XMT_MAX_BACKOFF
#define	RADIO_XMT_MAX_BACKOFF	0
#endif

#define	gbackoff_lbt	gbackoff (RADIO_LBT_MIN_BACKOFF, RADIO_LBT_MAX_BACKOFF)
#define	gbackoff_rcv	gbackoff (RADIO_RCV_MIN_BACKOFF, RADIO_RCV_MAX_BACKOFF)
#define	gbackoff_xmt	gbackoff (RADIO_XMT_MIN_BACKOFF, RADIO_XMT_MAX_BACKOFF)

// Carrier sense time before transmission in milliseconds
// 0 disables LBT
#ifndef	RADIO_LBT_SENSE_TIME
#define	RADIO_LBT_SENSE_TIME	2
#endif

#ifndef	RADIO_LBT_RSSI_THRESHOLD
#define	RADIO_LBT_RSSI_THRESHOLD	70
#endif

// This is probably completely irrelevant, because we only care about detecting
// the busy status
#ifndef	RADIO_LBT_RSSI_NIDLE
#define	RADIO_LBT_RSSI_NIDLE		4
#endif

#ifndef	RADIO_LBT_RSSI_NBUSY
#define	RADIO_LBT_RSSI_NBUSY		4
#endif

// This is in RAT ticks (at 4MHz)
#ifndef	RADIO_LBT_CORR_PERIOD
#define	RADIO_LBT_CORR_PERIOD		(4 * 32)
#endif

#ifndef	RADIO_LBT_CORR_NINVD
#define	RADIO_LBT_CORR_NINVD		3
#endif

#ifndef	RADIO_LBT_CORR_NBUSY
#define	RADIO_LBT_CORR_NBUSY		3
#endif

#ifndef	RADIO_LBT_MAX_TRIES
#define	RADIO_LBT_MAX_TRIES		16
#endif

#endif /* SMURPH (not needed by VUEE) */

// ============================================================================
// Force the piece to be included in VUEE compilation "as is"
//+++

#define	RADIO_N_CHANNELS	8

// This isn't negotiable, but, obviously, the application can use less than
// this
#define	CC1350_MAXPLEN		250

typedef struct {
//
// RF parameters as passed in the PHYSOPT call
//
	word	offdelay;
#if RADIO_WOR_MODE
	word	interval;
	byte	rss;		// RSSI threshold
	byte	pqt;		// This is yes or no for now
#endif
} cc1350_rfparams_t;

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
// PXOPTIONS access
#define	set_pxopts(pkt,cav,lbt,pwr) 	do { \
	((address)(pkt)) [(tcv_tlength ((address)(pkt)) >> 1) - 1] = \
	((lbt) ? 0x0000 : 0x8000) | ((pwr) << 12) | (cav); } while (0)
#endif

#endif
