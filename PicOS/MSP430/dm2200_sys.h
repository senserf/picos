#ifndef	__pg_dm2200_sys_h
#define	__pg_dm2200_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "pins.h"

//+++ "dm2200irq.c"

/* ================================================================== */

/*
 * Pin assignment
 *
 *  TR8100                      MSP430Fxxx
 * ===========================================
 *  RXDATA			P2.2 (P2.5)	        in
 *  RXDCLK			P2.6			in
 *  CFG                         P4.0                    out
 *  TXMOD			OUT0 (P2.7)		out (no change)
 *  CFGDAT (CNTRL1)		P5.0			in/out (no change)
 *  CFGCLK (CNTRL0)		P5.1			out (no change)
 *  RSSI			A0 (P6.0)		analog in (no change)
 *  RSSI POWER-UP		P2.0			up on 1 (no change)
 *
 */

/*
 * CFG set high and then can be set low - this select the serial mode which
 * remains active until power down. In serial mode, CFG up starts clocking
 * sequence for serial data transfer.
 */

// Set this to 0 (in options.sys) to direct CFG2 to P1.2 and RXDATA to P2.2
#ifndef	VERSA2_TARGET_BOARD
#define	VERSA2_TARGET_BOARD		1
#endif

/*
 * Transmission rate: this is determined as the number of SCLK ticks per
 * physical bit time. Remember that SLCK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 * I see that if we crank it up a little, things appear to work better. This
 * can be explained by the fact that the processing time tends to extend
 * slightly bit strobes beyond the calculated bounds.
 */
#define	BIT_RATE	9601		// 9700 used with 6MHz crystal

/*
 * All pins are preset to OUT HIGH for power savings on unconnected pins. Thus,
 * for adjustements in here, we only have to touch them up the right way.
 */

#if TARGET_BOARD == BOARD_VERSA2
#define	CFG_P1		_BIC (P1DIR, 0x08)
			// This one is not used by the radio, but we need it
			// for the reset button. Nowhere else to set its
			// direction.

#define	CFG_P2		_BIC (P2DIR, 0x78); \
			_BIS (P2SEL, 0x80)
#else

#define	CFG_P1		do { } while (0)

#define	CFG_P2		_BIC (P2DIR, 0x5C); \
			_BIS (P2SEL, 0x80)
#endif	/* TARGET_BOARD == BOADR_VERSA2 */

#define	CFG_P3		do { /* Not used by the radio */ } while (0)

#define	CFG_P4		_BIC (P4DIR, 0x0E); \
			_BIC (P4OUT, 0x01)

#define	CFG_P5		_BIC (P5OUT, 0x03)

#define	CFG_P6		_BIC (P6DIR, 0x01)

#define	ini_regs	do { CFG_P1; CFG_P2; CFG_P3; CFG_P4; CFG_P5; CFG_P6; } \
			while (0)
#if FCC_TEST_MODE
#define	fcc_test_send		((P1IN & 0x01) != 0)
#endif


/*
 * DM2200 signal operations. Timer's A Capture/Compare Block is used for signal
 * insertion (transmission).
 */
#define	cfg_up		_BIS (P4OUT, 0x01)
#define	cfg_down	_BIC (P4OUT, 0x01)

#define	ser_up		_BIS (P5OUT, 0x01)
#define	ser_down	_BIC (P5OUT, 0x01)
#define	ser_out		_BIS (P5DIR, 0x01)
#define	ser_in		_BIC (P5DIR, 0x01)
#define	ser_data	(P5IN & 0x1)

#define	ser_clk_up	_BIS (P5OUT, 0x02)
#define	ser_clk_down	_BIC (P5OUT, 0x02)

#define	rssi_on		_BIS (P2OUT, 0x01)
#define	rssi_off	_BIC (P2OUT, 0x01)

#if VERSA2_TARGET_BOARD
#define	rcv_sig_high	(P2IN & 0x20)
#else
#define	rcv_sig_high	(P2IN & 0x04)
#endif
#define	rcv_interrupt	(P2IFG & 0x40)
#define	rcv_edgelh	_BIC (P2IES, 0x40)
#define	rcv_edgehl	_BIS (P2IES, 0x40)
#define	rcv_clrint	_BIC (P2IFG, 0x40)
#define	rcv_enable	_BIS (P2IE, 0x40)
#define	rcv_disable	_BIC (P2IE, 0x40)

#if COLLECT_RXDATA_ON_LOW
#define	rcv_setedge	rcv_edgelh
#else
#define	rcv_setedge	do { } while (0)
#endif

/*
 * The timer runs in the up mode setting up TAIFG whenever the count is
 * reached.
 *
 * For transmission, the timer triggers comparator interrupts whenever it
 * reaches the value in TACCR0. These interrupts strobe signal level flips.
 *
 * For reception, we use the data clock extracted by TR8100.
 *
 */

#if CRYSTAL2_RATE

// Use XTL2 (should be high speed)

#define	TAFREQ		CRYSTAL2_RATE
#define	TASSEL_RADIO	TASSEL_SMCLK

#else	/* No XTL2 */

// If XTL1 is high-speed, use it; otherwise, use SMCLK == MCLK == DCO

#if CRYSTAL_RATE != 32768
#define	TASSEL_RADIO	TASSEL_ACLK
#define	TAFREQ		CRYSTAL_RATE
#else
#define	TASSEL_RADIO	TASSEL_SMCLK
#define	TAFREQ		4700000
#endif

#endif	/* CRYSTAL2_RATE */

#define	DM_RATE			(TAFREQ/BIT_RATE)

#define	timer_init		do { TACTL = TASSEL_RADIO | TACLR; } while (0)

#define DM_RATE_X1		DM_RATE
#define DM_RATE_X2		(DM_RATE + DM_RATE)
#define DM_RATE_X3		(DM_RATE + DM_RATE + DM_RATE)
#define DM_RATE_X4		(DM_RATE + DM_RATE + DM_RATE + DM_RATE)
// Used at the end of packet
#define	DM_RATE_XE		(DM_RATE_X4 + DM_RATE_X2)

#define	enable_xmt_timer	do { \
					TACCR0 = DM_RATE_X1; \
					TACCTL1 = 0; \
					TACCTL0 = OUTMOD_TOGGLE | CCIE; \
					_BIS (TACTL, MC_2 | TACLR); \
					_BIC (zzr_rcvmode, 0x80); \
				} while (0)

// Got rid of tmaux
//#define	set_signal_length(v)	TACCR0 = zzv_tmaux + (v)
#define	set_signal_length(v)	TACCR0 += (v)

#if 0
#define	toggle_signal		do { \
					zzr_rcvmode ^= 0x80; \
					zzv_tmaux = TACCR0; \
				} while (0)
#endif

#define	toggle_signal		(zzr_rcvmode ^= 0x80)

#define	current_signal_level	(zzr_rcvmode & 0x80)
					
/*
 * This stops the transmitter timer (and interrupts)
 */
#define	disable_xmt_timer	do { \
					TACCTL0 = 0; \
					TACCTL1 = 0; \
					_BIC (TACTL, MC_3); \
				} while (0)

// Needed by xcvcommon
#define	disable_xcv_timer	rcv_disable

#define	hard_lock		do { \
					_BIC (TACCTL0, CCIE); \
					_BIC (TACCTL1, CCIE); \
					rcv_disable; \
				} while (0)

#define	hard_drop		do { \
					if (zzv_status == HSTAT_RCV) \
						rcv_enable; \
					else if (zzv_status == HSTAT_XMT) \
						_BIS (TACCTL0, CCIE); \
				} while (0)

#endif
