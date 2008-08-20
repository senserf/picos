#ifndef	__pg_cc1100_h
#define	__pg_cc1100_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "kernel.h"
#include "phys_cc1100.h"
#include "cc1100_sys.h"
#include "rfleds.h"

/*
 * CRC calculation mode
 *
 *     0 = standard built-in CRC
 *     1 = built-in CC2400-compatible
 *     2 = software
 */
#ifndef	RADIO_CRC_MODE
#define	RADIO_CRC_MODE		2
#endif

#ifndef	RADIO_BITRATE
#define	RADIO_BITRATE		10000	/* This is the default */
#endif

#ifndef	RADIO_GUARD
#define	RADIO_GUARD		1	/* Guard process present */
#endif

#ifndef	RADIO_SYSTEM_IDENT
#define	RADIO_SYSTEM_IDENT	0xAB35	/* Sync word */
#endif

/*
 * Channel clear indication for TX:
 *
 *    0  = always (no LBT at all)
 *    1  = if RSSI below threshold
 *    2  = if not receiving a packet
 *    3  = 1+2
 *
 * Level 2 is the smallest recommended.
 *
 */
#define	CC_INDICATION		3	/* If not receiving a packet */

// RSSI threshold (for LBT), previous values: 8, 0
#define	CC_THRESHOLD		6	/* 1 lowest, 15 highest, 0 disable */
#define	CC_THRESHOLD_REL	0	/* 1 lowest, 3 highest, 0 disable */

// ============================================================================

#define	GUARD_LONG_DELAY	2	/* Minutes */
#define	GUARD_SHORT_DELAY	5000	/* 5 seconds */
#define	TRACE_DRIVER		0	/* Debugging diags */

#define	CC1100_MAXPLEN		60	/* Including checksum */

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
#define FREND0		    0x10

#ifdef	DEFINE_RF_SETTINGS

// NOTE: PicOS's concepts of bitrate and frequency are deeply entangled.
//       I hope that this can make it easier to use a custom frequency
//       So these likely only work for the 200000 bitrate... TJS
// setup defaults to use the 868MHz frequency range with no offset

#ifndef CC1100_FREQ_FREQ2_VALUE
	#define	CC1100_FREQ_FREQ2_VALUE	0x1E
#endif

#ifndef CC1100_FREQ_FREQ1_VALUE
	#define	CC1100_FREQ_FREQ1_VALUE	0xC4
#endif

#ifndef CC1100_FREQ_FREQ0_VALUE
	#define	CC1100_FREQ_FREQ0_VALUE	0xEC
#endif

// Define Register Contents ===================================================

const	byte	cc1100_rfsettings [] = {
//
// Common settings for all rates
//
        CCxxx0_FSCTRL0, 0x00,   // FSCTRL0
        CCxxx0_FREQ1,   CC1100_FREQ_FREQ1_VALUE,   // FREQ1 3B the base radio
        CCxxx0_FREQ0,   CC1100_FREQ_FREQ0_VALUE,   // FREQ0 13 frequency
        CCxxx0_MDMCFG0, 0xF8,   // MDMCFG0	      F8 channel spacing
        CCxxx0_FREND1,  0x56,   // FREND1
        CCxxx0_BSCFG,   0x6C,   // BSCFG

//      CCxxx0_AGCCTRL2,0x83,   // AGCCTRL2
// Changed Dec 31, 2006 PG
        CCxxx0_AGCCTRL2,0x03,   // AGCCTRL2

#define	RSSI_AGC	(((CC_THRESHOLD-8) & 0xf) | (CC_THRESHOLD_REL << 4))

#define	AGCCTRL1	(0x40 | RSSI_AGC)

	CCxxx0_AGCCTRL1,AGCCTRL1,

        CCxxx0_AGCCTRL0,0x91,   // AGCCTRL0
        CCxxx0_FSCAL3,  0xA9,   // FSCAL3
	CCxxx0_FSCAL1,	0x00,	// RFM
        CCxxx0_FSCAL0,  0x0D,   // FSCAL0
        CCxxx0_FSTEST,  0x59,   // FSTEST

	CCxxx0_IOCFG2,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG1,	0x2f,	// Unused and grounded
	CCxxx0_IOCFG0,	0x01,	// Reception, EOP or threshold

#if RADIO_CRC_MODE > 1
#define	MAX_TOTAL_PL	(CC1100_MAXPLEN+2)
#else
#define	MAX_TOTAL_PL	CC1100_MAXPLEN
#endif
        CCxxx0_PKTLEN,  MAX_TOTAL_PL,

	CCxxx0_PKTCTRL1,0x0C,	// Autoflush + 2 status bytes on reception

#if RADIO_CRC_MODE == 0
#define	CRC_FLAGS	0x04
#endif

#if RADIO_CRC_MODE == 1
#define	CRC_FLAGS	0x0C
#endif

#if RADIO_CRC_MODE > 1
#define	CRC_FLAGS	0x00
#endif
	CCxxx0_PKTCTRL0,(0x41 | CRC_FLAGS), // Whitening, pkt lngth follows sync

        CCxxx0_ADDR,    0x00,   // ADDR      Device address.

	CCxxx0_FIFOTHR,	0x0F,	// 64 bytes in FIFO

	CCxxx0_SYNC1,	((RADIO_SYSTEM_IDENT >> 8) & 0xff),
	CCxxx0_SYNC0,	((RADIO_SYSTEM_IDENT     ) & 0xff),

        CCxxx0_MCSM0,   0x14,   // Recalibrate on exiting IDLE state, full SLEEP

#define	MCSM1_TRANS_MODE	0x03	// RX->IDLE, TX->RX
#define	MCSM1_CCA		(CC_INDICATION << 4)

	CCxxx0_MCSM1,	(MCSM1_TRANS_MODE | MCSM1_CCA),

	CCxxx0_MCSM2,	0x07,

/* ========== */

        255
};

static const	byte	zz_rate0 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
	CCxxx0_FREQ2,   0x22,   // FREQ2
       	CCxxx0_MDMCFG4, 0xC7,   // MDMCFG4 	(4.8 kbps)
       	CCxxx0_MDMCFG3, 0x83,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,		// Double sync word
        CCxxx0_MDMCFG1, 0x42,   // MDMCFG1	22 FEC + 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	zz_rate1 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
	CCxxx0_FREQ2,   0x22,   // FREQ2
       	CCxxx0_MDMCFG4, 0x68,   // MDMCFG4 	(10 kbps)
       	CCxxx0_MDMCFG3, 0x93,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,
        CCxxx0_MDMCFG1, 0x42,   // MDMCFG1	22 FEC + 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	zz_rate2 [] = {

        CCxxx0_FSCTRL1, 0x0C,   // FSCTRL1
	CCxxx0_FREQ2,   0x22,   // FREQ2
       	CCxxx0_MDMCFG4, 0xCA,   // MDMCFG4 	(38.4 kbps)
       	CCxxx0_MDMCFG3, 0x83,   // MDMCFG3
        CCxxx0_MDMCFG2, 0x00 + 0x03,
        CCxxx0_MDMCFG1, 0x42,   // MDMCFG1	22 FEC + 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x34,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x15,   // FOCCFG
        CCxxx0_FSCAL2,  0x2A,   // FSCAL2

	255
};

static const	byte	zz_rate3 [] = {

        CCxxx0_FSCTRL1, 0x0A,   // FSCTRL1	WAS 0C
        CCxxx0_FREQ2,   CC1100_FREQ_FREQ2_VALUE,   // FREQ2

 #ifdef TRUE_200KBAUD
       	CCxxx0_MDMCFG4, 0x5C,   // TJS was 0x8C,   // MDMCFG4	(200 kbps)
	CCxxx0_MDMCFG3, 0xF8,   // TJS 0x22,   // MDMCFG3
 #else
       	CCxxx0_MDMCFG4, 0x8C,   // MDMCFG4	(200 kbps)
	CCxxx0_MDMCFG3, 0x22,   // MDMCFG3
 #endif

        CCxxx0_MDMCFG2, 0x10 + 0x02,		// Single sync word
        CCxxx0_MDMCFG1, 0x22,   // MDMCFG1	22 FEC + 4 pre + ch spacing
        CCxxx0_DEVIATN, 0x47,   // DEVIATN	47 -> 40 -> 34
        CCxxx0_FOCCFG,  0x36,   // FOCCFG
        CCxxx0_FSCAL2,  0x0A,   // FSCAL2

	255
};

const	byte	*cc1100_ratemenu [] = {
			zz_rate0, zz_rate1, zz_rate2, zz_rate3
};

#ifndef PATABLE
  #define	PATABLE { 0x03, 0x1C, 0x67, 0x60, 0x85, 0xCC, 0xC6, 0xC3 }
#endif

// ============================================================================

#else	/* DEFINE_RF_SETTINGS */

// No Register Definitions ====================================================

extern const byte cc1100_rfsettings [];
extern const byte *cc1100_ratemenu [];

#endif	/* DEFINE_RF_SETTINGS */

#define	CC1100_NRATES	4

#define	aggressive_transmitter	(vrate > 2)
#define	backoff_after_receive	(vrate < 2)
#define	default_xmit_space	(0x10 >> vrate)
#define	approx_xmit_time(p)	(vrate < 2 ? (p) : 1)

#define	TXEND_POLL_DELAY	1	/* Milliseconds */

#ifndef	XMIT_SPACE
#define	XMIT_SPACE		default_xmit_space
#endif

#if	RADIO_BITRATE == 5000
#define	RADIO_BITRATE_INDEX	0
#endif

#if	RADIO_BITRATE == 10000
#define	RADIO_BITRATE_INDEX	1
#endif

#if	RADIO_BITRATE == 38400
#define	RADIO_BITRATE_INDEX	2
#endif

#if	RADIO_BITRATE == 200000
#define	RADIO_BITRATE_INDEX	3
#endif

#ifndef	RADIO_BITRATE_INDEX
#error	"unknown bit rate, legal rates are 5000, 10000, 38400, 200000"
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
				warm_reset; \
			} while (0)

#define	warm_reset	do { \
				SPI_START; \
				cc1100_spi_out (CCxxx0_SRES); \
				while (so_val); \
				SPI_END; \
			} while (0)

#define	gbackoff	(bckf_timer = MIN_BACKOFF + (rnd () & MSK_BACKOFF))
// REMOVEME
// #define		gbackoff	9

extern word		zzv_drvprcs, zzv_qevent;
extern byte		zzv_iack, zzv_gwch;

#endif
