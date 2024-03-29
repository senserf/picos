/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_cc1100_h
#define	__pg_cc1100_h	1


// ============================================================================

// Temporary definitions for old driver compatibility
#ifndef	RADIO_POWER_SETTABLE
#define	RADIO_POWER_SETTABLE	1
#endif

#ifndef	RADIO_BITRATE_SETTABLE
#define	RADIO_BITRATE_SETTABLE	1
#endif

#ifndef	RADIO_CHANNEL_SETTABLE
#define	RADIO_CHANNEL_SETTABLE	1
#endif

#ifndef	RADIO_CAV_SETTABLE
#define	RADIO_CAV_SETTABLE	1
#endif

// ============================================================================

#ifndef __SMURPH__

#include "kernel.h"
#include "cc1100_sys.h"
#include "rfleds.h"

/*
 * CRC calculation mode
 *
 *     0 = standard built-in CRC
 *     1 = built-in CC2400-compatible
 *     2 = 0 + AUTOFLUSH (don't present packets with wrong CRC)
 *     3 = 1 + AUTOFLUSH
 *     4 = SOFTWARE CHECKSUM
 */
#ifndef	RADIO_CRC_MODE
#define	RADIO_CRC_MODE		0	/* Changed (R100617) to 0 from 2 */
#endif

#ifndef	RADIO_CRC_TRANSPARENT
// If nonzero, pass received packets with bad CRC; it is up to the user to
// decide what to do with them
#define	RADIO_CRC_TRANSPARENT	0
#endif

#ifndef	RADIO_OPTIONS
#define	RADIO_OPTIONS		0
#endif
// ============================================================================
// OPTIONS, EXTENSIONS (see modysms.h for symbolic names):
//
//	0x001 overwrite packet [1] with the packet's backoff value
//	0x002 trace: diag messages on important events and problems)
//	0x004 track packets: 6 counters + PHYSOPT_ERROR (see phys_cc1100.c)
//	0x008 prechecks: initial check for RF chip present (to prevent hangups)
//  	0x010 lean code: remove parameter checks for driver requests
//	0x020 WOR test mode: PHYSOPT_SETPARAMS sets extended WOR parameters
//	0x040 RESET with argument writes into registers (doing no reset)
//	0x080 collect entropy from the air on startup
//	0x100 last word of outgoing packet specifies transmission options
// ============================================================================

// ============================================================================
// LBT parameters =============================================================
// ============================================================================

// The randomized backoff after a failed channel assessment is determined as
// MIN_BACKOFF + random [0 ... 2^BACKOFF_EXP - 1]
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

//
// LBT modes:
//
//	0 - no LBT, transmit blindly ignoring everything
//	1 - not if receiving a packet
//	2 - not if CC assessment failure or receiving a packet
//	3 - staged, limited [up to 8 times degrading sensitivity]
//
#ifndef	RADIO_LBT_MODE
#define	RADIO_LBT_MODE		3
#endif

#ifndef	RADIO_RECALIBRATE
#define	RADIO_RECALIBRATE	0
#endif

#if RADIO_LBT_MODE < 0 || RADIO_LBT_MODE > 3
#error "S: illegal setting of RADIO_LBT_MODE, must be 0...3"
#endif

// RSSI thresholds (for LBT); these values are only relevant when
// RADIO_LBT_MODE == 2
#ifndef	RADIO_CC_THRESHOLD
#define	RADIO_CC_THRESHOLD	1	// 1 lowest, 15 highest, 0 off
#endif

#ifndef	RADIO_CC_THRESHOLD_REL
#define	RADIO_CC_THRESHOLD_REL	1	// 1 lowest, 3 highest, 0 off
#endif

#if RADIO_LBT_MODE == 3

#define	LBT_RETR_LIMIT		8	// Retry count
#define	LBT_RETR_FORCE_RCV	5	// Force threshold, honor own reception
#define	LBT_RETR_FORCE_OFF	7	// Force threshold, LBT completely off

#endif

// ============================================================================

// Default power is max
#ifndef	RADIO_DEFAULT_POWER
#define	RADIO_DEFAULT_POWER	7
#endif

#ifndef	RADIO_POWER_SETTABLE
#define	RADIO_POWER_SETTABLE	0
#endif

#if RADIO_DEFAULT_POWER < 0 || RADIO_DEFAULT_POWER > 7
#error "S: RADIO_DEFAULT_POWER > 7!!!"
#endif

#ifndef	RADIO_DEFAULT_BITRATE
#define	RADIO_DEFAULT_BITRATE	1
#endif

#define	CC1100_NRATES	4

#ifndef	RADIO_BITRATE_SETTABLE
#define	RADIO_BITRATE_SETTABLE	0
#endif

#ifndef RADIO_DEFAULT_CHANNEL
#define	RADIO_DEFAULT_CHANNEL	0
#endif

#ifndef	RADIO_CHANNEL_SETTABLE
#define	RADIO_CHANNEL_SETTABLE	0
#endif

#if RADIO_DEFAULT_CHANNEL < 0 || RADIO_DEFAULT_CHANNEL > 255
#error "S: RADIO_DEFAULT_CHANNEL > 255!!!"
#endif

#ifndef	RADIO_SYSTEM_IDENT
#define	RADIO_SYSTEM_IDENT	0xAB35	/* Sync word */
#endif

// ============================================================================

#ifndef	RADIO_CAV_SETTABLE
#define	RADIO_CAV_SETTABLE	0
#endif

#ifndef	RADIO_WOR_MODE
#define	RADIO_WOR_MODE		0
#endif

// ============================================================================

// Various delays needed by the driver to prevent hangups and so on

// Time the CPU must remain powered up while the radio is transiting from a 
// power down state (approximately in usecs)
#ifndef	DELAY_CHIP_READY
// No, I don't think this matters anymore. I am removing the delay from the
// driver and replacing it with a spin loop just in case. I don't think the
// actual delay is ever executed.
// #define	DELAY_CHIP_READY	900
#define	DELAY_CHIP_READY	0
#endif

// After detecting a state that needs settling until next try (usecs)
#ifndef	DELAY_SETTLING_STATE
#define	DELAY_SETTLING_STATE	50
#endif

// After detecting SRX failure until next try (usecs)
#ifndef	DELAY_SRX_FAILURE
#define	DELAY_SRX_FAILURE	100
#endif

// This is the delay after power up and before starting a transmission (in
// usec), to make sure that CCA for that transmission is done properly. Without
// it, the first CCA unavoidably fails. Previously the delay (400) was applied
// after enter_idle () because of some hangups (that don't seem to be there any
// more, at least on CC430). See the comments in power_up (). I am adding 10
// because 400 seems to be tight. 231024
#ifndef	DELAY_IDLE_PDOWN
#define	DELAY_IDLE_PDOWN	410
#endif
// ============================================================================

#if (RADIO_CRC_MODE & 4)
// Software CRC
#define	CRC_FLAGS		0x00
#define	AUTOFLUSH_FLAG		0x00
#define	SOFTWARE_CRC		1
#define	CC1100_MAXPLEN		60	/* Including checksum */

#else

#define	SOFTWARE_CRC		0
#define	CC1100_MAXPLEN		62

#if (RADIO_CRC_MODE & 2)
#define	AUTOFLUSH_FLAG		0x08
#else
#define	AUTOFLUSH_FLAG		0x00
#endif

#if (RADIO_CRC_MODE & 1)
#define	CRC_FLAGS		0x0C
#else
#define	CRC_FLAGS		0x04
#endif

#endif	/* RADIO_CRC_MODE */

#if AUTOFLUSH_FLAG
#error "S: AUTOFLUSH mode doesn't work due to a bug in CC1100, don't use!!!"
#define	IOCFG0_INT		0x0F	/* CRC OK */
#else
#define	IOCFG0_INT		0x01	/* FIFO nonempty, packet received */
#endif

#define	MAX_TOTAL_PL		62

// Register Numbers ===========================================================

#define CCxxx0_IOCFG2       0x00        // GDO2 output pin configuration
#define CCxxx0_IOCFG1       0x01        // GDO1 output pin configuration
#define CCxxx0_IOCFG0       0x02        // GDO0 output pin configuration
#define CCxxx0_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CCxxx0_SYNC1        0x04        // Sync word, high byte
#define CCxxx0_SYNC0        0x05        // Sync word, low byte
#define CCxxx0_PKTLEN       0x06        // Packet length
#define CCxxx0_PKTCTRL1     0x07        // Packet automation control
#define CCxxx0_PKTCTRL0     0x08        // Packet automation control
#define CCxxx0_ADDR         0x09        // Device address
#define CCxxx0_CHANNR       0x0A        // Channel number
#define CCxxx0_FSCTRL1      0x0B        // Frequency synthesizer control
#define CCxxx0_FSCTRL0      0x0C        // Frequency synthesizer control
#define CCxxx0_FREQ2        0x0D        // Frequency control word, high byte
#define CCxxx0_FREQ1        0x0E        // Frequency control word, middle byte
#define CCxxx0_FREQ0        0x0F        // Frequency control word, low byte
#define CCxxx0_MDMCFG4      0x10        // Modem configuration
#define CCxxx0_MDMCFG3      0x11        // Modem configuration
#define CCxxx0_MDMCFG2      0x12        // Modem configuration
#define CCxxx0_MDMCFG1      0x13        // Modem configuration
#define CCxxx0_MDMCFG0      0x14        // Modem configuration
#define CCxxx0_DEVIATN      0x15        // Modem deviation setting
#define CCxxx0_MCSM2        0x16        // MRC State Machine configuration
#define CCxxx0_MCSM1        0x17        // MRC State Machine configuration
#define CCxxx0_MCSM0        0x18        // MRC State Machine configuration
#define CCxxx0_FOCCFG       0x19        // Freq Off Compensation configuration
#define CCxxx0_BSCFG        0x1A        // Bit Synchronization configuration
#define CCxxx0_AGCCTRL2     0x1B        // AGC control
#define CCxxx0_AGCCTRL1     0x1C        // AGC control
#define CCxxx0_AGCCTRL0     0x1D        // AGC control
#define CCxxx0_WOREVT1      0x1E        // High byte Event 0 timeout
#define CCxxx0_WOREVT0      0x1F        // Low byte Event 0 timeout
#define CCxxx0_WORCTRL      0x20        // Wake On Radio control
#define CCxxx0_FREND1       0x21        // Front end RX configuration
#define CCxxx0_FREND0       0x22        // Front end TX configuration
#define CCxxx0_FSCAL3       0x23        // Frequency synthesizer calibration
#define CCxxx0_FSCAL2       0x24        // Frequency synthesizer calibration
#define CCxxx0_FSCAL1       0x25        // Frequency synthesizer calibration
#define CCxxx0_FSCAL0       0x26        // Frequency synthesizer calibration
#define CCxxx0_RCCTRL1      0x27        // RC oscillator configuration
#define CCxxx0_RCCTRL0      0x28        // RC oscillator configuration
#define CCxxx0_FSTEST       0x29        // Frequency synth calibration control
#define CCxxx0_PTEST        0x2A        // Production test
#define CCxxx0_AGCTEST      0x2B        // AGC test
#define CCxxx0_TEST2        0x2C        // Various test settings
#define CCxxx0_TEST1        0x2D        // Various test settings
#define CCxxx0_TEST0        0x2E        // Various test settings

// Strobe commands
#define CCxxx0_SRES         0x30        // Reset chip.
#define CCxxx0_SFSTXON      0x31        // Enable and calibrate freq synth
#define CCxxx0_SXOFF        0x32        // Turn off crystal oscillator.
#define CCxxx0_SCAL         0x33        // Calibrate freq synth and turn it off
#define CCxxx0_SRX          0x34        // Enable RX
#define CCxxx0_STX          0x35        // Enable TX
#define CCxxx0_SIDLE        0x36        // Exit RX / TX
#define CCxxx0_SAFC         0x37        // Perform AFC adjustment
#define CCxxx0_SWOR         0x38        // Start automatic RX polling sequence
#define CCxxx0_SPWD         0x39        // Enter power down mode
#define CCxxx0_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CCxxx0_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CCxxx0_SWORRST      0x3C        // Reset real time clock.
#define CCxxx0_SNOP         0x3D        // No operation

#define CCxxx0_PARTNUM      (0x30 | 0xC0)
#define CCxxx0_VERSION      (0x31 | 0xC0)
#define CCxxx0_FREQEST      (0x32 | 0xC0)
#define CCxxx0_LQI          (0x33 | 0xC0)
#define CCxxx0_RSSI         (0x34 | 0xC0)
#define CCxxx0_MARCSTATE    (0x35 | 0xC0)
#define CCxxx0_WORTIME1     (0x36 | 0xC0)
#define CCxxx0_WORTIME0     (0x37 | 0xC0)
#define CCxxx0_PKTSTATUS    (0x38 | 0xC0)
#define CCxxx0_VCO_VC_DAC   (0x39 | 0xC0)
#define CCxxx0_TXBYTES      (0x3A | 0xC0)
#define CCxxx0_RXBYTES      (0x3B | 0xC0)

#define CCxxx0_PATABLE      0x3E
#define CCxxx0_TXFIFO       0x3F
#define CCxxx0_RXFIFO       0x3F

// This one is for power control; we set it up separately
#define FREND0_SET	    0x10

//
// Changed R100617 from 0x1E to 0x22, yielding 904 MHz as the base frequency
// (the same as in GENESIS). The formula is:
//
//	((FREQ2 << 16) + (FREQ1 << 8) + FREQ0) * 26MHz / 65536
//
#ifndef CC1100_FREQ_FREQ2_VALUE
	#define	CC1100_FREQ_FREQ2_VALUE	0x22
#endif

#ifndef CC1100_FREQ_FREQ1_VALUE
	#define	CC1100_FREQ_FREQ1_VALUE	0xC4
#endif

#ifndef CC1100_FREQ_FREQ0_VALUE
	#define	CC1100_FREQ_FREQ0_VALUE	0xEC
#endif

// Channel spacing exponent
#define	CC1100_CHANSPC_E		0x02	// These values yield 200 kHz
// Channel spacing mantissa
#define	CC1100_CHANSPC_M		0xF8

// ============================================================================
// ============================================================================

// Formula for calculating the base frequency * 10
#define	CC1100_BFREQ_T10 \
	(((((lword) CC1100_FREQ_FREQ2_VALUE << 16) | \
   	   ((lword) CC1100_FREQ_FREQ1_VALUE <<  8) | \
      	   ((lword) CC1100_FREQ_FREQ0_VALUE      ) ) * 260 + \
					32768L) / 65536L)
#define	CC1100_BFREQ		((word)(CC1100_BFREQ_T10 / 10))
#define	CC1100_BFREQ_10		((word)(CC1100_BFREQ_T10 - (CC1100_BFREQ *10)))

// Channel spacing in kHz
#define	CC1100_CHANSPC_T1000 \
	((word)(((26000L * (256 + (lword) CC1100_CHANSPC_M)) + \
		((lword) 1 << (17 - CC1100_CHANSPC_E))) / \
		((lword) 1 << (18 - CC1100_CHANSPC_E))))

// Default channel frequency
#define	CC1100_DFREQ_T10 (CC1100_BFREQ_T10 + \
		(RADIO_DEFAULT_CHANNEL * CC1100_CHANSPC_T1000) / 100)
#define	CC1100_DFREQ		((word)(CC1100_DFREQ_T10 / 10))
#define	CC1100_DFREQ_10		((word)(CC1100_DFREQ_T10 - (CC1100_DFREQ *10)))

// ============================================================================
// ============================================================================

#if RADIO_RECALIBRATE
// Note that the recalibration is ALWAYS done (see CCxxx0_MCSM0_x below) when
// transiting from IDLE to RX or TX, so this options only causes extra
// transitions
#if RADIO_RECALIBRATE < 0 || RADIO_RECALIBRATE > 63
#error "S: RADIO_RECALIBRATE must be between 0 and 63 inclusively"
#endif
// When periodic recalibration is selected, we make TX transit to IDLE, so we
// will recalibrate on the subsequent transition to RX (forced after every
// TX->IDLE and also periodically, if staying in RX for longer than
// RADIO_RECALIBRATE seconds). Note that periodic recalibration is recommended
// for those nodes whose receivers tend to stay constantly on.
#define	MCSM1_TRANS_MODE	0x00
#else
// No periodic recalibration, TX transits to RX. This mode is recommended for
// low-power nodes whose radios are mostly off being turned on for short bursts
// (where the initial calibration suffices). This kind of assumes that the
// burst of such a node consists of a TX followed by leaving the receiver open
// for a (short) while
#define	MCSM1_TRANS_MODE	0x03
#endif

// MSCM1 register settings (selecting global LBT mode)
#define	MCSM1_LBT_OFF		(MCSM1_TRANS_MODE | (0 << 4))	// OFF
#define	MCSM1_LBT_RCV		(MCSM1_TRANS_MODE | (2 << 4))	// If receiving
#define	MCSM1_LBT_FULL		(MCSM1_TRANS_MODE | (3 << 4))	// Rcv+assess

#define	CCxxx0_MCSM2_x		0x07
#define	CCxxx0_MCSM2_WOR_RP	0x18
#define	CCxxx0_MCSM2_WOR_P	0x08

#define	CCxxx0_AGCCTRL1_x	(0x40 | (((RADIO_CC_THRESHOLD-8) & 0xf) | \
					(RADIO_CC_THRESHOLD_REL << 4)))

#define	CCxxx0_PKTCTRL1_x	(0x04 | AUTOFLUSH_FLAG)

#if RADIO_LBT_MODE == 0
	#define	CCxxx0_MCSM1_x	MCSM1_LBT_OFF
#endif
#if RADIO_LBT_MODE == 1
	#define	CCxxx0_MCSM1_x	MCSM1_LBT_RCV
#endif
#if RADIO_LBT_MODE == 2 || RADIO_LBT_MODE == 3
	#define	CCxxx0_MCSM1_x	MCSM1_LBT_FULL
#endif

#ifdef	__CC430__
	// Recalibrate on exiting IDLE state, full SLEEP; bits 2 and 3 are
	// reserved (not used) on CC430, so better not to set them
	#define	CCxxx0_MCSM0_x	0x10
#else
	// Changed from 0x14 to 0x18 (PG 100620), see page 78 PO_TIMEOUT
	// (cc1101)
	#define	CCxxx0_MCSM0_x	0x18
#endif

// ============================================================================
	
#if RADIO_WOR_MODE

#ifndef	RADIO_WOR_IDLE_TIMEOUT
#define	RADIO_WOR_IDLE_TIMEOUT		3072
#endif

#ifndef	RADIO_WOR_PREAMBLE_TIME
#define	RADIO_WOR_PREAMBLE_TIME		2048
#endif

// This is the maximum, which, at WOR_RES == 0, should yield ca. 1.89 sec for
// the complete cycle (assuming 26MHz crystal, which we assume throughout)
#ifndef	WOR_EVT0_TIME
#ifdef	__CC430__
// On CC430, the WOR clock is simply ACLK running at 32768Hz, whereas on CC110x,
// the WOR clock runs at 26MHz/750 = 34667Hz. The ratio is 0.9452, which yields
// 61944, instead of 65535.
#define	WOR_EVT0_TIME	0xF1F8
#else
#define	WOR_EVT0_TIME	0xFFFF
#endif
#endif

// With WOR_RES == 0, 5 yields 0.391% duty cycle, while 6 is 0.195%; at max
// WOR_EVT0_TIME, they translate into ca. 7.4ms and 3.7ms, i.e., 74 and 37
// bits @ 10kbps
#ifndef	WOR_RX_TIME
#define	WOR_RX_TIME	6
#endif

// With WOR_AUTOSYNC = 0 (which is our only sensible choice), this gives the
// number of RC oscillator ticks (fosc/750): 4, 6, 8, 12, 16, 24, 32, 48
// (1 tick = 2.88x10^-5 sec = 0.0288 msec) until EVENT1. The documents are
// confusing. At some point they say that FS calibration counts to EVENT1
// delay, at some other that it doesn't. The most conservative setting is 7.
// We shall experiment.
#ifndef	WOR_EVT1_TIME
#ifdef	__CC430__
// This doesn't seem to make any difference whatsoever
#define	WOR_EVT1_TIME	7
#else
#define	WOR_EVT1_TIME	0
#endif
#endif

// Preamble quality threshold: 1 means 4 bits, 2 - 8 bits, 3 - 12 bits; it is
// in fact a bit more complicated (see PKTCTRL1 in doc)
#ifndef	WOR_PQ_THR
#define	WOR_PQ_THR	5
#endif

// RSSI threshold for WOR (from 1 to 15, converted to signed nibble)
#ifndef	WOR_RSSI_THR
#define	WOR_RSSI_THR	2
#endif

// ============================================================================

#define	CCxxx0_PKTCTRL1_WORx	(WOR_PQ_THR << 5)
#define	CCxxx0_AGCCTRL1_WORx	(0x40 | ((WOR_RSSI_THR - 8) & 0x0F))
#define	CCxxx0_WORCTRL_WORx	((WOR_EVT1_TIME << 4) | 0x8)
#if WOR_RSSI_THR == 0
// Switch off RSSI thresholding
#define	CCxxx0_MCSM2_WORx	(CCxxx0_MCSM2_WOR_P  | WOR_RX_TIME)
#else
// RSSI thresholding is on
#define	CCxxx0_MCSM2_WORx	(CCxxx0_MCSM2_WOR_RP | WOR_RX_TIME)
#endif

#endif

// ============================================================================
// Rate-specific settings
// ============================================================================

#define CCxxx0_FSCTRL1_RATE0	0x0C
// 4.8 kbps
#define CCxxx0_MDMCFG4_RATE0	0xC7
#define CCxxx0_MDMCFG3_RATE0	0x83
// FSK + double sync word
#define CCxxx0_MDMCFG2_RATE0	(0x00 + 0x03)
// Preamble + ch spacing
#define CCxxx0_MDMCFG1_RATE0	(0x40 + CC1100_CHANSPC_E)
#define CCxxx0_DEVIATN_RATE0	0x34
#define CCxxx0_FOCCFG_RATE0 	0x15
#define CCxxx0_FSCAL2_RATE0 	0x2A

// ============================================================================

#define CCxxx0_FSCTRL1_RATE1	CCxxx0_FSCTRL1_RATE0
// 10 kbps
#define CCxxx0_MDMCFG4_RATE1	0x68
#define CCxxx0_MDMCFG3_RATE1	0x93
#define CCxxx0_MDMCFG2_RATE1	CCxxx0_MDMCFG2_RATE0	
#define CCxxx0_MDMCFG1_RATE1	CCxxx0_MDMCFG1_RATE0
#define CCxxx0_DEVIATN_RATE1	CCxxx0_DEVIATN_RATE0
#define CCxxx0_FOCCFG_RATE1	CCxxx0_FOCCFG_RATE0
#define CCxxx0_FSCAL2_RATE1	CCxxx0_FSCAL2_RATE0

// ============================================================================

#define CCxxx0_FSCTRL1_RATE2	CCxxx0_FSCTRL1_RATE0
// 38.4 kbps
#define CCxxx0_MDMCFG4_RATE2	0xCA
#define CCxxx0_MDMCFG3_RATE2	0x83
#define CCxxx0_MDMCFG2_RATE2	CCxxx0_MDMCFG2_RATE0	
#define CCxxx0_MDMCFG1_RATE2	CCxxx0_MDMCFG1_RATE0
#define CCxxx0_DEVIATN_RATE2	CCxxx0_DEVIATN_RATE0
#define CCxxx0_FOCCFG_RATE2	CCxxx0_FOCCFG_RATE0
#define CCxxx0_FSCAL2_RATE2	CCxxx0_FSCAL2_RATE0

// ============================================================================

#define CCxxx0_FSCTRL1_RATE3	0x0A
// 200 kbps
#define CCxxx0_MDMCFG4_RATE3	0x8C
#define CCxxx0_MDMCFG3_RATE3	0x22
// GFSK + single sync word
#define CCxxx0_MDMCFG2_RATE3	(0x10 + 0x02)
#define CCxxx0_MDMCFG1_RATE3	(0x20 + CC1100_CHANSPC_E)
#define CCxxx0_DEVIATN_RATE3	0x47
#define CCxxx0_FOCCFG_RATE3	0x36
#define CCxxx0_FSCAL2_RATE3	0x0A

#if RADIO_DEFAULT_BITRATE == 5000 || RADIO_DEFAULT_BITRATE == 0
	#define	RADIO_BITRATE_INDEX	0
	#define CCxxx0_FSCTRL1_RATEx	CCxxx0_FSCTRL1_RATE0
	#define CCxxx0_MDMCFG4_RATEx	CCxxx0_MDMCFG4_RATE0
	#define CCxxx0_MDMCFG3_RATEx	CCxxx0_MDMCFG3_RATE0
	#define CCxxx0_MDMCFG2_RATEx	CCxxx0_MDMCFG2_RATE0
	#define CCxxx0_MDMCFG1_RATEx	CCxxx0_MDMCFG1_RATE0
	#define CCxxx0_DEVIATN_RATEx	CCxxx0_DEVIATN_RATE0
	#define CCxxx0_FOCCFG_RATEx	CCxxx0_FOCCFG_RATE0 
	#define CCxxx0_FSCAL2_RATEx	CCxxx0_FSCAL2_RATE0
#endif

#if RADIO_DEFAULT_BITRATE == 10000 || RADIO_DEFAULT_BITRATE == 1
	#define	RADIO_BITRATE_INDEX	1
	#define CCxxx0_FSCTRL1_RATEx	CCxxx0_FSCTRL1_RATE1
	#define CCxxx0_MDMCFG4_RATEx	CCxxx0_MDMCFG4_RATE1
	#define CCxxx0_MDMCFG3_RATEx	CCxxx0_MDMCFG3_RATE1
	#define CCxxx0_MDMCFG2_RATEx	CCxxx0_MDMCFG2_RATE1
	#define CCxxx0_MDMCFG1_RATEx	CCxxx0_MDMCFG1_RATE1
	#define CCxxx0_DEVIATN_RATEx	CCxxx0_DEVIATN_RATE1
	#define CCxxx0_FOCCFG_RATEx	CCxxx0_FOCCFG_RATE1 
	#define CCxxx0_FSCAL2_RATEx	CCxxx0_FSCAL2_RATE1
#endif

#if RADIO_DEFAULT_BITRATE == 38400 || RADIO_DEFAULT_BITRATE == 2
	#define	RADIO_BITRATE_INDEX	2
	#define CCxxx0_FSCTRL1_RATEx	CCxxx0_FSCTRL1_RATE2
	#define CCxxx0_MDMCFG4_RATEx	CCxxx0_MDMCFG4_RATE2
	#define CCxxx0_MDMCFG3_RATEx	CCxxx0_MDMCFG3_RATE2
	#define CCxxx0_MDMCFG2_RATEx	CCxxx0_MDMCFG2_RATE2
	#define CCxxx0_MDMCFG1_RATEx	CCxxx0_MDMCFG1_RATE2
	#define CCxxx0_DEVIATN_RATEx	CCxxx0_DEVIATN_RATE2
	#define CCxxx0_FOCCFG_RATEx	CCxxx0_FOCCFG_RATE2 
	#define CCxxx0_FSCAL2_RATEx	CCxxx0_FSCAL2_RATE2
#endif

#if RADIO_DEFAULT_BITRATE == 200000 || RADIO_DEFAULT_BITRATE == 3
	#define	RADIO_BITRATE_INDEX	3
	#define CCxxx0_FSCTRL1_RATEx	CCxxx0_FSCTRL1_RATE3
	#define CCxxx0_MDMCFG4_RATEx	CCxxx0_MDMCFG4_RATE3
	#define CCxxx0_MDMCFG3_RATEx	CCxxx0_MDMCFG3_RATE3
	#define CCxxx0_MDMCFG2_RATEx	CCxxx0_MDMCFG2_RATE3
	#define CCxxx0_MDMCFG1_RATEx	CCxxx0_MDMCFG1_RATE3
	#define CCxxx0_DEVIATN_RATEx	CCxxx0_DEVIATN_RATE3
	#define CCxxx0_FOCCFG_RATEx	CCxxx0_FOCCFG_RATE3 
	#define CCxxx0_FSCAL2_RATEx	CCxxx0_FSCAL2_RATE3
#endif

#ifndef	RADIO_BITRATE_INDEX
#error "S: Unknown bit rate, legal rates are 0-5000, 1-10000, 2-38400, 3-200000"
#endif

// ============================================================================
// Driver section, in-memory data =============================================
// ============================================================================

#ifdef	CC1100_DEFINE_RF_SETTINGS

#if RADIO_BITRATE_SETTABLE

static const	byte	cc1100_ratereg [] = {
        					CCxxx0_FSCTRL1,
       						CCxxx0_MDMCFG4,
       						CCxxx0_MDMCFG3,
        					CCxxx0_MDMCFG2,
        					CCxxx0_MDMCFG1,
        					CCxxx0_DEVIATN,
        					CCxxx0_FOCCFG, 
        					CCxxx0_FSCAL2
					    };

static const	byte	__cc1100_v_rate0 [] = {

        					CCxxx0_FSCTRL1_RATE0,
       						CCxxx0_MDMCFG4_RATE0,
       						CCxxx0_MDMCFG3_RATE0,
        					CCxxx0_MDMCFG2_RATE0,
        					CCxxx0_MDMCFG1_RATE0,
        					CCxxx0_DEVIATN_RATE0,
        					CCxxx0_FOCCFG_RATE0,
        					CCxxx0_FSCAL2_RATE0
					};

static const	byte	__cc1100_v_rate1 [] = {

        					CCxxx0_FSCTRL1_RATE1,
       						CCxxx0_MDMCFG4_RATE1,
       						CCxxx0_MDMCFG3_RATE1,
        					CCxxx0_MDMCFG2_RATE1,
        					CCxxx0_MDMCFG1_RATE1,
        					CCxxx0_DEVIATN_RATE1,
        					CCxxx0_FOCCFG_RATE1,
        					CCxxx0_FSCAL2_RATE1
					};

static const	byte	__cc1100_v_rate2 [] = {

					        CCxxx0_FSCTRL1_RATE2,
       						CCxxx0_MDMCFG4_RATE2,
       						CCxxx0_MDMCFG3_RATE2,
        					CCxxx0_MDMCFG2_RATE2,
        					CCxxx0_MDMCFG1_RATE2,
        					CCxxx0_DEVIATN_RATE2,
        					CCxxx0_FOCCFG_RATE2,
        					CCxxx0_FSCAL2_RATE2
					};

static const	byte	__cc1100_v_rate3 [] = {

        					CCxxx0_FSCTRL1_RATE3,
       						CCxxx0_MDMCFG4_RATE3,
       						CCxxx0_MDMCFG3_RATE3,
        					CCxxx0_MDMCFG2_RATE3,
        					CCxxx0_MDMCFG1_RATE3,
        					CCxxx0_DEVIATN_RATE3,
        					CCxxx0_FOCCFG_RATE3,
        					CCxxx0_FSCAL2_RATE3
					};

static const	byte	*cc1100_ratemenu [] = {	__cc1100_v_rate0,
						__cc1100_v_rate1,
						__cc1100_v_rate2,
						__cc1100_v_rate3  };

#define	approx_xmit_time(p)	(vrate < 2 ? (p) : 1)

#else

#define	approx_xmit_time(p)	(RADIO_BITRATE_INDEX < 2 ? (p) : 1)

#endif	/* RADIO_BITRATE_SETTABLE */

#if RADIO_WOR_MODE

// ============================================================================
// WOR data ===================================================================
// ============================================================================

#ifdef	__CC430__
// We use the interrupt status for GIO1 to tell when the chip is ready; needed
// for safe transition from a sleep state to active
#define	IOCFG1_VAL	0x29			// RF_RDYn
#endif

static const byte cc1100_wor_sr [] = {
// Registers to set when entering WOR
	CCxxx0_PKTCTRL1,
	CCxxx0_AGCCTRL1,
	CCxxx0_WORCTRL,
	CCxxx0_MCSM2
#ifndef	__CC430__
	// No need to reconfigure IOCFG0 on CC430
      , CCxxx0_IOCFG0
#endif
};

static
#if (RADIO_OPTIONS & 0x20) == 0
const
#endif
byte	cc1100_wor_von [] = {
// Register values when WOR is on
	CCxxx0_PKTCTRL1_WORx,
	CCxxx0_AGCCTRL1_WORx,
	CCxxx0_WORCTRL_WORx,
	CCxxx0_MCSM2_WORx
#ifndef	__CC430__
      , 0x08
#endif
};

static const byte cc1100_wor_voff [] = {
// Register values when WOR is off
	CCxxx0_PKTCTRL1_x,
	CCxxx0_AGCCTRL1_x,
	0xf8,					// Reset value
	CCxxx0_MCSM2_x
#ifndef	__CC430__
      , IOCFG0_INT
#endif
};

#endif	/* RADIO_WOR_MODE */

// ============================================================================
// Global settings ============================================================
// ============================================================================

#ifndef	IOCFG1_VAL
#define	IOCFG1_VAL	0x2f		// Grounded
#endif

static const byte cc1100_rfsettings [] = {
	0x2f,		// IOCFG2	unused and grounded
	IOCFG1_VAL,	// IOCFG1	unused or RX ready (n) flag
	IOCFG0_INT,	// IOCFG0	RX interrupt condition
	0x0F,		// FIFOTHR	64 bytes in FIFO
	((RADIO_SYSTEM_IDENT >> 8) & 0xff),	// SYNC1
	((RADIO_SYSTEM_IDENT     ) & 0xff),	// SYNC0
        MAX_TOTAL_PL,				// PKTLEN
	CCxxx0_PKTCTRL1_x, 			// PKTCTRL1
	// Whitening, packet length follows SYNC
	(0x41 | CRC_FLAGS), 			// PKTCTRL0
        0x00,   				// ADDR
	RADIO_DEFAULT_CHANNEL,			// CHANNR
	CCxxx0_FSCTRL1_RATEx,			// FSCTRL1 rate-specific
	0x00,					// FSCTRL0
	CC1100_FREQ_FREQ2_VALUE,   		// FREQ2
        CC1100_FREQ_FREQ1_VALUE,   		// FREQ1 
        CC1100_FREQ_FREQ0_VALUE,   		// FREQ0
	CCxxx0_MDMCFG4_RATEx,			// MDMCFG4 rate-specific
	CCxxx0_MDMCFG3_RATEx,			// MDMCFG3 rate-specific
	CCxxx0_MDMCFG2_RATEx,			// MDMCFG2 rate-specific
	CCxxx0_MDMCFG1_RATEx,			// MDMCFG1 rate-specific
        CC1100_CHANSPC_M,   	   		// MDMCFG0 channel spacing
	CCxxx0_DEVIATN_RATEx,			// DEVIATN rate-specific
	CCxxx0_MCSM2_x,				// MCSM2
	CCxxx0_MCSM1_x,				// MCSM1
	CCxxx0_MCSM0_x,				// MCSM0
	CCxxx0_FOCCFG_RATEx,			// FOCCFG rate-specific
	0x6c,					// BSCFG
	// Changed Dec 31, 2006 PG from 0x83
        0x03,   				// AGCCTRL2
	CCxxx0_AGCCTRL1_x,			// AGCCTRL1 (LBT thresholding)
	0x91,					// AGCCTRL0
#if RADIO_WOR_MODE
	WOR_EVT0_TIME >> 8,			// WOREVT1
	WOR_EVT0_TIME & 0xff,			// WOREVT0
#else
	0x87,					// WOREVT1 (reset value)
	0x6B,					// WOREVT0 (reset value)
#endif
	0xF8,					// WORCTRL (reset value)
	0x56,					// FREND1
	FREND0_SET | RADIO_DEFAULT_POWER,	// FREND0
	0xA9,					// FSCAL3
        CCxxx0_FSCAL2_RATEx,			// FSCAL2 rate-specific
	0x00,					// FSCAL1
	0x0D					// FSCAL0
	// Note: removed FSTEST (RFM recommended the setting of 0x59; the
	// manual insists the register should never be set
};

// ============================================================================
// ============================================================================

#if RADIO_LBT_MODE == 3

static const byte cc1100_agcctrl_table [LBT_RETR_FORCE_RCV] = {
	// This table is ignored for retr >= LBT_RETR_FORCE_RCV
	0x59, 0x5A, 0x6B, 0x7D, 0x4F
};

#endif

// ============================================================================

#endif

// ============================================================================

#ifndef CC1100_PATABLE

#if 0
#define	CC1100_PATABLE { 0x00, 0x03, 0x1C, 0x57, 0x8E, 0x85, 0xCC, 0xE0 }
#endif
// This is the recommended PATABLE setting determined in experiments with
// OLIMEX boards (with no external antennas) 231024:
#define	CC1100_PATABLE { 0x6C, 0x30, 0x03, 0x0A, 0x1C, 0x65, 0xAD, 0xD0 }

#endif

// ============================================================================

#define	TXEND_POLL_DELAY	1	/* Milliseconds */

// ============================================================================

/*
 * Chip states
 */
#define CC1100_STATE_MASK			0x70
#define CC1100_FIFO_BYTES_AVAILABLE_MASK	0x0F
#define CC1100_STATE_TX              		0x20
#define CC1100_STATE_TX_UNDERFLOW    		0x70
#define CC1100_STATE_RX              		0x10
#define CC1100_STATE_RX_OVERFLOW     		0x60
#define CC1100_STATE_IDLE            		0x00

// Transitional states
#define	CC1100_STATE_CALIBRATE			0x40
#define	CC1100_STATE_SETTLING			0x50

#ifdef __CC430__

// ============================================================================

#define	CC1100_SPI_START	CNOP
#define	CC1100_SPI_END		CNOP

// Note that SRES on CC430 leaves the chip in SLEEP rather than IDLE, so a
// safe transition to IDLE involves a wake delay
#define	cc1100_full_reset	do { \
					strobe (CCxxx0_SRES); \
					strobe (CCxxx0_SNOP); \
				} while (0)

#include "irq_cc430_rf.h"

#else	/* __CC430__ */

// ============================================================================

#define	CC1100_SPI_START	do { \
					cc1100_csn_down; \
					while (cc1100_so_val); \
				} while (0)

#define	CC1100_SPI_END		cc1100_csn_up

#define	cc1100_full_reset	do { \
					cc1100_sclk_down; \
					cc1100_csn_up; \
        				CC1100_UWAIT1; \
					cc1100_csn_down; \
					CC1100_UWAIT1; \
					cc1100_csn_up; \
					CC1100_UWAIT41; \
					CC1100_SPI_START; \
					spi_out (CCxxx0_SRES); \
					while (cc1100_so_val); \
					CC1100_SPI_END; \
				} while (0)

#if RADIO_WOR_MODE

#define	wor_enable_int	cc1100_rcv_int_enable

#endif

#endif	/* __CC430__ */

// ============================================================================

#if RADIO_WOR_MODE

typedef struct {
//
// WOR parameters as passed in the PHYSOPT call
//
	word	offdelay;
	word	interval;
#if RADIO_OPTIONS & RADIO_OPTION_WORPARAMS
	byte	evt0_time, rx_time, pq_thr, rssi_thr, evt1_time;
#endif
} cc1100_rfparams_t;

#endif

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

extern word		__cc1100_v_drvprcs, __cc1100_v_qevent;

#endif /* SMURPH: end stuff NOT needed by VUEE */

//
// Useful for VUEE, will be adding more
//

#define	RADIO_N_CHANNELS	256

#if (RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS)
// PXOPTIONS access
#define	set_pxopts(pkt,cav,lbt,pwr) 	do { \
	((address)(pkt)) [(tcv_tlength ((address)(pkt)) >> 1) - 1] = \
	((lbt) ? 0x0000 : 0x8000) | ((pwr) << 12) | (cav); } while (0)
#endif


#endif
