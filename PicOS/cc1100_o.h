#ifndef	__pg_cc1100_h
#define	__pg_cc1100_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "cc1100_sys.h"
#include "rfleds.h"

#ifdef RADIO_WOR_MODE
#if RADIO_WOR_MODE
#error "S: RADIO_WOR_MODE incompatible with old CC1100 driver!!!"
#endif
#endif

/*
 * CRC calculation mode
 *
 *     0 = standard built-in CRC
 *     1 = built-in CC2400-compatible
 *     2 = 0 + AUTOFLUSH (don't present packets with wrong CRC)
 *     3 = 1 + AUTOFLUSH
 *     4 = SOFTWARE
 */
#ifndef	RADIO_CRC_MODE
#define	RADIO_CRC_MODE		0	/* Changed (R100617) to 0 from 2 */
#endif

#ifndef	RADIO_OPTIONS
#define	RADIO_OPTIONS		0
#endif
// ============================================================================
// OPTIONS, EXTENSIONS:
//
//	0x01 debug: diag messages on various abnormal conditions)
//	0x02 trace: diag messages on important events)
//	0x04 track packets: 6 counters + PHYSOPT_ERROR (see phys_cc1100.c)
//	0x08 prechecks: initial check for RF chip present (to prevent hangups)
//  	0x10 guard process present
//	0x20 extra consistency checks
//	0x40 PHYSOPT_RESET admits a parameter pointing to a substitution table
//	     for changing selected register values
//	0x80 collect entropy from the air on startup
// ============================================================================

// ============================================================================
// LBT parameters =============================================================
// ============================================================================

// The randomized backoff after a failed channel assessment is determined as
// MIN_BACKOFF + random [0 ... 2^BACKOFF_EXP - 1]
#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	2
#endif

// If this is zero, we have an "aggressive" transmitter (it tries to grab the
// channel at 1 msec intervals); generally not recommended
#ifndef	RADIO_LBT_BACKOFF_EXP
#define	RADIO_LBT_BACKOFF_EXP	6
#endif

// If BACKOFF_RX is nonzero, then there is a backoff after packet reception
// amounting to MIN_BACKOFF + random [0 ... 2^BACKOFF_RX - 1]
#ifndef	RADIO_LBT_BACKOFF_RX
#define	RADIO_LBT_BACKOFF_RX	3
#endif

// Fixed minimum space (in milliseconds) between two consecutively transmitted
// packets
#ifndef	RADIO_LBT_XMIT_SPACE
#define	RADIO_LBT_XMIT_SPACE	2
#endif

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

// MSCM1 register settings (selecting global LBT mode)
#define	MCSM1_LBT_OFF		(MCSM1_TRANS_MODE | (0 << 4))	// OFF
#define	MCSM1_LBT_RCV		(MCSM1_TRANS_MODE | (2 << 4))	// If receiving
#define	MCSM1_LBT_FULL		(MCSM1_TRANS_MODE | (3 << 4))	// Rcv+assess

// RSSI thresholds (for LBT); these values are only relevant when
// RADIO_LBT_MODE == 2
#ifndef	RADIO_CC_THRESHOLD
#define	RADIO_CC_THRESHOLD	6	// 1 lowest, 15 highest, 0 off
#endif

#ifndef	RADIO_CC_THRESHOLD_REL
#define	RADIO_CC_THRESHOLD_REL	0	// 1 1 lowest, 3 highest, 0 off
#endif

// ============================================================================

// Default power corresponds to the center == 0dBm
#ifndef	RADIO_DEFAULT_POWER
#define	RADIO_DEFAULT_POWER	2
#endif

#ifndef	RADIO_DEFAULT_BITRATE
#define	RADIO_DEFAULT_BITRATE	10000
#endif

#ifndef RADIO_DEFAULT_CHANNEL
#define	RADIO_DEFAULT_CHANNEL	0
#endif

#if RADIO_DEFAULT_CHANNEL > 255
#error "S: RADIO_DEFAULT_CHANNEL > 255!!!"
#endif

#ifndef	RADIO_SYSTEM_IDENT
#define	RADIO_SYSTEM_IDENT	0xAB35	/* Sync word */
#endif

// ============================================================================

#define	GUARD_LONG_DELAY	65367	/* Max == 64 seconds minus 1 tick */
#define	GUARD_SHORT_DELAY	5000	/* 5 seconds */

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

// This is set in the program that wants to have in-memory constants defined,
// like the driver itself
#ifdef	CC1100_DEFINE_RF_SETTINGS

// ============================================================================
// Define Register Contents ===================================================
// ============================================================================

const	byte	cc1100_rfsettings [] = {
//
// Common settings for all rates
//
        CCxxx0_FSCTRL0, 0x00,   // FSCTRL0
	CCxxx0_FREQ2,   CC1100_FREQ_FREQ2_VALUE,   // FREQ2
        CCxxx0_FREQ1,   CC1100_FREQ_FREQ1_VALUE,   // FREQ1 3B the base radio
        CCxxx0_FREQ0,   CC1100_FREQ_FREQ0_VALUE,   // FREQ0 13 frequency
        CCxxx0_MDMCFG0, CC1100_CHANSPC_M,   	   // MDMCFG0 channel spacing
        CCxxx0_FREND1,  0x56,   // FREND1
        CCxxx0_BSCFG,   0x6C,   // BSCFG

//      CCxxx0_AGCCTRL2,0x83,   // AGCCTRL2
// Changed Dec 31, 2006 PG
        CCxxx0_AGCCTRL2,0x03,   // AGCCTRL2

#define	RSSI_AGC	(((RADIO_CC_THRESHOLD-8) & 0xf) | \
				(RADIO_CC_THRESHOLD_REL << 4))

#define	AGCCTRL1_VAL	(0x40 | RSSI_AGC)

	CCxxx0_AGCCTRL1,AGCCTRL1_VAL,

        CCxxx0_AGCCTRL0,0x91,   // AGCCTRL0
        CCxxx0_FSCAL3,  0xA9,   // FSCAL3
	CCxxx0_FSCAL1,	0x00,	// RFM
        CCxxx0_FSCAL0,  0x0D,   // FSCAL0
        CCxxx0_FSTEST,  0x59,   // FSTEST (FIXME: according to doc, this is
				// redundant)
	CCxxx0_IOCFG2,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG1,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG0,	IOCFG0_INT,	// Reception interrupt condition

        CCxxx0_PKTLEN,  MAX_TOTAL_PL,

				// Autoflush + 2 status bytes on reception
	CCxxx0_PKTCTRL1,(0x04 | AUTOFLUSH_FLAG),

	CCxxx0_PKTCTRL0,(0x41 | CRC_FLAGS), // Whitening, pkt lngth follows sync

        CCxxx0_ADDR,    0x00,   // ADDR      Device address.

	CCxxx0_FIFOTHR,	0x0F,	// 64 bytes in FIFO

	CCxxx0_SYNC1,	((RADIO_SYSTEM_IDENT >> 8) & 0xff),
	CCxxx0_SYNC0,	((RADIO_SYSTEM_IDENT     ) & 0xff),

#ifdef	__CC430__
        CCxxx0_MCSM0,   0x10,   // Recalibrate on exiting IDLE state, full SLEEP
#else
	// Changed from 0x14 to 0x18 (PG 100620), see page 78 PO_TIMEOUT
	// (cc1101)
        CCxxx0_MCSM0,   0x18,   // Recalibrate on exiting IDLE state, full SLEEP
#endif

#if RADIO_LBT_MODE < 0 || RADIO_LBT_MODE > 3
#error "S: illegal setting of RADIO_LBT_MODE, must be 0...3"
#endif

// Automatic transitions of the RF automaton: after RX to IDLE, after TX to RX
#define	MCSM1_TRANS_MODE	0x03

#if RADIO_LBT_MODE == 0
	CCxxx0_MCSM1,	MCSM1_LBT_OFF,
#endif
#if RADIO_LBT_MODE == 1
	CCxxx0_MCSM1,	MCSM1_LBT_RCV,
#endif
#if RADIO_LBT_MODE == 2 || RADIO_LBT_MODE == 3
	CCxxx0_MCSM1,	MCSM1_LBT_FULL,
#endif
	CCxxx0_MCSM2,	0x07,

/* ========== */

        255
};

static const	byte	__pi_rate0 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
       	CCxxx0_MDMCFG4, 0xC7,   // MDMCFG4 	(4.8 kbps)
       	CCxxx0_MDMCFG3, 0x83,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,		// Double sync word
        CCxxx0_MDMCFG1, 0x40 + CC1100_CHANSPC_E,// MDMCFG1 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	__pi_rate1 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
       	CCxxx0_MDMCFG4, 0x68,   // MDMCFG4 	(10 kbps)
       	CCxxx0_MDMCFG3, 0x93,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,
        CCxxx0_MDMCFG1, 0x40 + CC1100_CHANSPC_E,// MDMCFG1 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	__pi_rate2 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
       	CCxxx0_MDMCFG4, 0xCA,   // MDMCFG4 	(38.4 kbps)
       	CCxxx0_MDMCFG3, 0x83,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,
        CCxxx0_MDMCFG1, 0x40 + CC1100_CHANSPC_E,// MDMCFG1 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	__pi_rate3 [] = {

        CCxxx0_FSCTRL1, 0x0A,   // FSCTRL1	WAS 0C

 #ifdef TRUE_200KBAUD
       	CCxxx0_MDMCFG4, 0x5C,   // TJS was 0x8C,   // MDMCFG4	(200 kbps)
	CCxxx0_MDMCFG3, 0xF8,   // TJS 0x22,   // MDMCFG3
 #else
       	CCxxx0_MDMCFG4, 0x8C,   // MDMCFG4	(200 kbps)
	CCxxx0_MDMCFG3, 0x22,   // MDMCFG3
 #endif

        CCxxx0_MDMCFG2, 0x10 + 0x02,		// Single sync word
        CCxxx0_MDMCFG1, 0x20 + CC1100_CHANSPC_E,// MDMCFG1 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x47,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x36,   // FOCCFG
        CCxxx0_FSCAL2,  0x0A,   // FSCAL2

	255
};

const	byte	*cc1100_ratemenu [] = {
			__pi_rate0, __pi_rate1, __pi_rate2, __pi_rate3
};

// ============================================================================
// ============================================================================

#if RADIO_LBT_MODE == 3

#define	LBT_RETR_LIMIT		8	// Retry count
#define	LBT_RETR_FORCE_RCV	5	// Force threshold, honor own reception
#define	LBT_RETR_FORCE_OFF	7	// Force threshold, LBT completely off

const byte cc1100_agcctrl_table [LBT_RETR_FORCE_RCV] = {
	// This table is ignored for retr >= LBT_RETR_FORCE_RCV
	0x59, 0x5A, 0x6B, 0x7D, 0x4F
};

#endif

// ============================================================================

#else	/* DEFINE_RF_SETTINGS */

// No Register Definitions ====================================================

extern const byte cc1100_rfsettings [];
extern const byte *cc1100_ratemenu [];
#if RADIO_LBT_MODE == 3
extern const byte cc1100_agcctrl_table [];
#endif

#endif	/* DEFINE_RF_SETTINGS */

#ifndef CC1100_PATABLE
#define	CC1100_PATABLE { 0x03, 0x1C, 0x57, 0x8E, 0x85, 0xCC, 0xC6, 0xC3 }
#endif

#define	CC1100_NRATES	4

#define	approx_xmit_time(p)	(vrate < 2 ? (p) : 1)

#define	TXEND_POLL_DELAY	1	/* Milliseconds */

#if	RADIO_DEFAULT_BITRATE == 5000
#define	RADIO_BITRATE_INDEX	0
#endif

#if	RADIO_DEFAULT_BITRATE == 10000
#define	RADIO_BITRATE_INDEX	1
#endif

#if	RADIO_DEFAULT_BITRATE == 38400
#define	RADIO_BITRATE_INDEX	2
#endif

#if	RADIO_DEFAULT_BITRATE == 200000
#define	RADIO_BITRATE_INDEX	3
#endif

#ifndef	RADIO_BITRATE_INDEX
#error	"S: Unknown bit rate, legal rates are 5000, 10000, 38400, 200000"
#endif

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

#define	SPI_START	CNOP
#define	SPI_END		CNOP

#define	full_reset	do { \
				cc1100_strobe (CCxxx0_SRES); \
				cc1100_strobe (CCxxx0_SNOP); \
			} while (0)

#include "irq_cc430_rf.h"

#else

// ============================================================================

#define	SPI_START	do { \
				csn_down; \
				while (so_val); \
			} while (0)

#define	SPI_END		csn_up

#define	full_reset	do { \
				sclk_down; \
				csn_up; \
        			UWAIT1; \
				csn_down; \
				UWAIT1; \
				csn_up; \
				UWAIT41; \
				SPI_START; \
				cc1100_spi_out (CCxxx0_SRES); \
				while (so_val); \
				SPI_END; \
			} while (0)

#endif

// ============================================================================

#define	gbackoff(e)	do { \
				if (e) \
					utimer_set (bckf_timer, \
						RADIO_LBT_MIN_BACKOFF + \
						(rnd () & ((1 << (e)) - 1))); \
				else \
					utimer_set (bckf_timer, 0); \
			} while (0)

extern word		__pi_v_drvprcs, __pi_v_qevent;

#endif