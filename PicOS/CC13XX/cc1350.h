#ifndef	__pg_cc1350_h
#define	__pg_cc1350_h

#include "kernel.h"
#include "rfleds.h"

// ============================================================================

#ifndef	RADIO_DEFAULT_POWER
#define	RADIO_DEFAULT_POWER	7
#endif

#if RADIO_DEFAULT_POWER < 0 || RADIO_DEFAULT_POWER > 8
#error "S: RADIO_DEFAULT_POWER > 8!!!"
#endif

// Rates: 	0: 625 bps, long range, must be compiled in with power 8
//		1: 10.0K
//		2: 38.4K
//		3: 50.0K (TI default)

#ifndef	RADIO_DEFAULT_BITRATE
#define	RADIO_DEFAULT_BITRATE	3
#endif

#if RADIO_DEFAULT_BITRATE == 625 || RADIO_DEFAULT_BITRATE == 0
#define	RADIO_BITRATE_INDEX	0
#elif RADIO_DEFAULT_BITRATE == 10000 || RADIO_DEFAULT_BITRATE == 1
#define	RADIO_BITRATE_INDEX	1
#elif RADIO_DEFAULT_BITRATE == 38400 || RADIO_DEFAULT_BITRATE == 2
#define	RADIO_BITRATE_INDEX	2
#elif RADIO_DEFAULT_BITRATE == 50000 || RADIO_DEFAULT_BITRATE == 3
#define	RADIO_BITRATE_INDEX	2
#else
#error "S: Unknown bit rate, legal rates are: 0-625, 1-10000, 2-38400, 3-50000"
#endif

// Channels have to be implemented by direct frequency adjustments; to be
// determined, but we will probably have just a few

#ifndef RADIO_DEFAULT_CHANNEL
#define	RADIO_DEFAULT_CHANNEL	0
#endif

#if RADIO_DEFAULT_CHANNEL < 0 || RADIO_DEFAULT_CHANNEL > 7
#error "S: RADIO_DEFAULT_CHANNEL > 255!!!"
#endif

#ifndef	RADIO_SYSTEM_IDENT
#define	RADIO_SYSTEM_IDENT	0xAB3553BA	/* Sync word */
#endif

#ifndef	RADIO_DEFAULT_OFFDELAY
// The deafult linger time before turning off the radio after last xmit with
// RX off (msecs)
#define	RADIO_DEFAULT_OFFDELAY	256
#endif

// WOR and stuff later, if ever

// ============================================================================

// Consistency checks
#if RADIO_BITRATE_INDEX == 0
// This is long range, high power setting
#if RADIO_DEFAULT_POWER < 8
#error "S: RADIO_DEFAULT_BITRATE == 625 requires RADIO_DEFAULT_POWER == 8"
#endif
#endif

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

// Highest power has a separate smartf_settings.c in two versions:
//	- long range, low rate
//	- 50 kbps, TI default

// From SmartRF Studio (starting from index = 1, because the lowest rate
// comes with non-settable, special config
#define	CC1350_RATABLE 		{ \
					{ 0xB,	0x12C5 }, \
					{ 0x7,	0x2DE0 }, \
					{ 0xF,	0x8000 }, \
				}

// Channel increment; for now, 1 channel == 1 MHz
#define	CC1350_NCHANNELS	 16
#define	CC1350_BASEFREQ		868	// Megahertz

#define	CC1350_MAXPLEN	250

// ============================================================================
// Backoff + LBT ==============================================================
// ============================================================================

#define	gbackoff(e)	do { \
				if (e) \
					utimer_set (bckf_timer, \
						RADIO_LBT_MIN_BACKOFF + \
						(rnd () & ((1 << (e)) - 1))); \
				else \
					utimer_set (bckf_timer, 0); \
			} while (0)

#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	2
#endif

#ifndef	RADIO_LBT_BACKOFF_EXP
#define	RADIO_LBT_BACKOFF_EXP	6
#endif

// Fixed minimum space (in milliseconds) between two consecutively transmitted
// packets
#ifndef	RADIO_LBT_XMIT_SPACE
#define	RADIO_LBT_XMIT_SPACE	2
#endif

// Carrier sense time before transmission in milliseconds
// 0 disables LBT
#ifndef	RADIO_LBT_SENSE_TIME
#define	RADIO_LBT_SENSE_TIME	2
#endif

#ifndef	RADIO_LBT_RSSI_THRESHOLD
#define	RADIO_LBT_RSSI_THRESHOLD	70
#endif

#ifndef	RADIO_LBT_RSSI_NIDLE
#define	RADIO_LBT_RSSI_NIDLE		4
#endif

#ifndef	RADIO_LBT_RSSI_NBUSY
#define	RADIO_LBT_RSSI_NBUSY		4
#endif

#ifndef	RADIO_LBT_CORR_PERIOD
#define	RADIO_LBT_CORR_PERIOD		(4 * 1024)
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



#endif