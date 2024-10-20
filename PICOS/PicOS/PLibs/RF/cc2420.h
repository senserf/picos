/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_cc2420_h
#define	__pg_cc2420_h	1

#include "kernel.h"
#include "cc2420_sys.h"
#include "rfleds.h"

// ============================================================================

#ifndef	RADIO_OPTIONS
#define	RADIO_OPTIONS		0
#endif

#ifndef	RADIO_CC_THRESHOLD
#define	RADIO_CC_THRESHOLD	(-32)
#endif

#ifndef	RADIO_DEEP_POWER_DOWN
// By default use the deepest power down state, i.e., disable VREG
#define	RADIO_DEEP_POWER_DOWN	1
#endif

#ifndef	RADIO_SYSTEM_IDENT
#define	RADIO_SYSTEM_IDENT	0xA70F	/* Sync word (default) */
#endif

#ifndef	RADIO_CRC_TRANSPARENT
#define	RADIO_CRC_TRANSPARENT	0
#endif

#define	CC2420_MAXPLEN		126	/* Including checksum */

// Base frequency: Fc = 2048 + FREQ_VALUE MHz; the official channels in
// 802.15.4 are numbered 11 through 26 (16 channels); we shall number them
// 0-15 with the freq setting 357 + 5 * k
#define	CC2420_FREQ_BASE	357

// Strobe commands
#define CC2x20_SNOP		0x00	// Nothing, read status
#define CC2x20_SXOSCON		0x01	// Oscillator on
#define	CC2x20_STXCAL		0x02	// Enable & calibrate freq synth TX
#define	CC2x20_SRXON		0x03	// RX on
#define	CC2x20_STXON		0x04	// Enable TX
#define	CC2x20_STXONCCA		0x05	// Enable TX if channel clear
#define	CC2x20_SRFOFF		0x06	// Disable RX/TX and freq synth
#define	CC2x20_SXOSCOFF		0x07	// Turn off cryst osc and RF
#define	CC2x20_SFLUSHRX		0x08	// Flush RX FIFO & reset rcv
#define	CC2x20_SFLUSHTX		0x09	// Flush TX FIFO
#define	CC2x20_SACK		0x0A	// Send ACK, pending cleared
#define	CC2x20_SACKPEND		0x0B	// Send ACK, pending set
#define	CC2x20_SRXDEC		0x0C	// Start RXFIFO decryption
#define	CC2x20_STXENC		0x0D	// Start TXFIFO encryption
#define	CC2x20_SAES		0x0E	// Stand alone encryption

// Registers
#define	CC2x20_MAIN	0x10
#define	CC2x20_MDMCTRL0	0x11
#define	CC2x20_MDMCTRL1	0x12
#define	CC2x20_RSSI	0x13 // R/W RSSI and CCA Status and Control
#define	CC2x20_SYNCWORD	0x14 // R/W Synchronisation word control
#define	CC2x20_TXCTRL	0x15 // R/W Transmit Control
#define	CC2x20_RXCTRL0	0x16 // R/W Receive Control 0
#define	CC2x20_RXCTRL1	0x17 // R/W Receive Control 1
#define	CC2x20_FSCTRL	0x18 // R/W Frequency Synthesizer Control and Status
#define	CC2x20_SECCTRL0	0x19 // R/W Security Control 0
#define	CC2x20_SECCTRL1	0x1A // R/W Security Control 1
#define	CC2x20_BATTMON	0x1B // R/W Battery Monitor Control and Status
#define	CC2x20_IOCFG0	0x1C // R/W Input / Output Control 0
#define	CC2x20_IOCFG1	0x1D // R/W Input / Output Control 1
#define	CC2x20_MANFIDL	0x1E // R/W Manufacturer ID, Low 16 bits
#define	CC2x20_MANFIDH	0x1F // R/W Manufacturer ID, High 16 bits
#define	CC2x20_FSMTC	0x20 // R/W Finite State Machine Time Constants
#define	CC2x20_MANAND	0x21 // R/W Manual signal AND override
#define	CC2x20_MANOR	0x22 // R/W Manual signal OR override
#define	CC2x20_AGCCTRL	0x23 // R/W AGC Control
#define	CC2x20_AGCTST0	0x24 // R/W AGC Test 0
#define	CC2x20_AGCTST1	0x25 // R/W AGC Test 1
#define	CC2x20_AGCTST2	0x26 // R/W AGC Test 2
#define	CC2x20_FSTST0	0x27 // R/W Frequency Synthesizer Test 0
#define	CC2x20_FSTST1	0x28 // R/W Frequency Synthesizer Test 1
#define	CC2x20_FSTST2	0x29 // R/W Frequency Synthesizer Test 2
#define	CC2x20_FSTST3	0x2A // R/W Frequency Synthesizer Test 3
#define	CC2x20_RXBPFTST	0x2B // R/W Receiver Bandpass Filter Test
#define	CC2x20_FSMSTATE	0x2C // R Finite State Machine State Status
#define	CC2x20_ADCTST	0x2D // R/W ADC Test
#define	CC2x20_DACTST	0x2E // R/W DAC Test
#define	CC2x20_TOPTST	0x2F // R/W Top Level Test
#define	CC2x20_TXFIFO	0x3E // W Transmit FIFO Byte
#define	CC2x20_RXFIFO	0x3F // R/W Receiver FIFO Byte

// Status bits
#define	CC2x20_STAT_XOSCON	0x40
#define	CC2x20_STAT_TXUNDF	0x20
#define	CC2x20_STAT_ENBUSY	0x10
#define	CC2x20_STAT_TXACTV	0x08
#define	CC2x20_STAT_PLLOCK	0x04
#define	CC2x20_STAT_RSSVAL	0x02

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

#if RADIO_LBT_MODE == 3

#define	LBT_RETR_LIMIT		8	// Retry count
#define	LBT_RETR_FORCE_RCV	7	// Force threshold, honor own reception
#define	LBT_RETR_FORCE_OFF	7	// Force threshold, always honor RCPT

#endif

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

#if RADIO_LBT_MODE < 0 || RADIO_LBT_MODE > 3
#error "S: illegal setting of RADIO_LBT_MODE, must be 0...3"
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

#ifndef RADIO_DEFAULT_CHANNEL
#define	RADIO_DEFAULT_CHANNEL	0
#endif

#ifndef	RADIO_CHANNEL_SETTABLE
#define	RADIO_CHANNEL_SETTABLE	0
#endif

#if RADIO_DEFAULT_CHANNEL < 0 || RADIO_DEFAULT_CHANNEL > 15
#error "S: RADIO_DEFAULT_CHANNEL > 15!!!"
#endif

// ============================================================================

#ifndef	RADIO_CAV_SETTABLE
#define	RADIO_CAV_SETTABLE	0
#endif

#define	CC2420_SPI_START	cc2420_csn_down
#define	CC2420_SPI_END		do { cc2420_csn_up; cc2420_si_up; } while (0)

// ============================================================================

#define	gbackoff(e)	do { \
				if (e) \
					utimer_set (bckf_timer, \
						RADIO_LBT_MIN_BACKOFF + \
						(rnd () & ((1 << (e)) - 1))); \
				else \
					utimer_set (bckf_timer, 0); \
			} while (0)

extern word		__cc2420_v_drvprcs, __cc2420_v_qevent;

#endif
