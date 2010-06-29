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

/*
 * Transmission rate: this is determined as the number of SCLK ticks per
 * physical bit time. Remember that SCLK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 * I see that if we crank it up a little, things appear to work better. This
 * can be explained by the fact that the processing time tends to extend
 * slightly bit strobes beyond the calculated bounds.
 */
#define	BIT_RATE	9601	// 9700 seems to give best results

#include "board_rf.h"

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

#define	set_signal_length(v)	TACCR0 += (v)

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

#define	lbt_ok(v)	((v) < (word)(((long)LBT_THRESHOLD * 4096) / 100))

#endif
