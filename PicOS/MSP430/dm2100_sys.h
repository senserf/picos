#ifndef	__pg_dm2100_sys_h
#define	__pg_dm2100_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "dm2100irq.c"

#include "pins.h"

/* ================================================================== */

#include "board_rf.h"

/*
 * The timer runs in the up mode setting up TAIFG whenever the count is
 * reached.
 *
 * For transmission, the timer triggers comparator interrupts whenever it
 * reaches the value in TACCR0. These interrupts strobe signal level flips.
 * For reception, signal transitions on CCI0B trigger capture interrupts,
 * with the time of the transition returned in TACCR0. Additionally, TACCR1
 * is used to trigger a timeout interrupt if the signal does not change for
 * a longish while.
 *
 */

#if CRYSTAL_RATE != 32768
#define	TASSEL_RADIO	TASSEL_ACLK
#define	TAFREQ		CRYSTAL_RATE
#else
#define	TASSEL_RADIO	TASSEL_SMCLK
#define	TAFREQ		4500000
#endif

#define	DM_RATE			(TAFREQ/BIT_RATE)

#define	timer_init		do { TACTL = TASSEL_RADIO | TACLR; } while (0)

#define DM_RATE_X1		DM_RATE
#define DM_RATE_X2		(DM_RATE + DM_RATE)
#define DM_RATE_X3		(DM_RATE + DM_RATE + DM_RATE)
#define DM_RATE_X4		(DM_RATE + DM_RATE + DM_RATE + DM_RATE)

#define	DM_RATE_DELTA		(DM_RATE / 2)

#define	DM_RATE_R1		(DM_RATE_X1 + DM_RATE_DELTA)
#define	DM_RATE_R2		(DM_RATE_X2 + DM_RATE_DELTA)
#define	DM_RATE_R3		(DM_RATE_X3 + DM_RATE_DELTA)
#define	DM_RATE_R4		(DM_RATE_X4 + DM_RATE_DELTA)
#define	DM_RATE_R5		(DM_RATE_X4 + DM_RATE_X1 + DM_RATE_DELTA)

#define	enable_xmt_timer	do { \
					TACCR0 = DM_RATE_X1; \
					TACCTL1 = 0; \
					TACCTL0 = CCIE | OUT; \
					_BIS (TACTL, MC_2 | TACLR); \
				} while (0)

#define	set_signal_length(v)	TACCR0 = zzv_tmaux + (v)

#define	toggle_signal		do { \
					TACCTL0 ^= OUT; \
					zzv_tmaux = TACCR0; \
				} while (0)
					

#define	current_signal_level	(TACCTL0 & OUT)

/*
 * This stops the transmitter timer (and interrupts)
 */
#define	disable_xcv_timer	do { \
					TACCTL0 = 0; \
					TACCTL1 = 0; \
					_BIC (TACTL, MC_3); \
				} while (0)

/*
 * The reception is trickier. I tried capture on both slopes simultaneously,
 * but I couldn't get the signal right from SCCI (don't know why, perhaps the
 * input voltage from RXDATA tends to be flaky). So I have settled for 
 * switching the slope after every capture. This is done by starting with
 * CM_1 (meaning transition from low to high) and then switching to CM_2
 * (accomplished by adding CM_1 to what was there), then back to CM_1, and
 * so on. This way the actual perceived signal is implied from the last
 * transition and determined by the contents of CM.
 */
#define	enable_rcv_timer	do { \
					zzv_tmaux = 0; \
					TACCTL0 = CM_1 | CCIS_1 | SCS | CAP \
						| CCIE; \
					_BIS (TACTL, MC_2 | TACLR); \
				} while (0)

#define	enable_rcv_timeout	do { TACCTL1 = CCIE; } while (0)
#define	disable_rcv_timeout	do { TACCTL1 =    0; } while (0)

#define get_signal_params(t,v)	do { \
					(t) = ((word) TACCR0) - zzv_tmaux; \
					zzv_tmaux = (word) TACCR0; \
					if (((v) = (TACCTL0 & CM_1))) \
						TACCTL0 += CM_1; \
					else \
						TACCTL0 -= CM_1; \
				} while (0)

#define	set_rcv_timeout		do { TACCR1 = TACCR0 + SH5; } while (0)

#define	hard_lock		do { \
					_BIC (TACCTL0, CCIE); \
					_BIC (TACCTL1, CCIE); \
				} while (0)

#define	hard_drop		do { \
					if (zzv_status) { \
						_BIS (TACCTL0, CCIE); \
						if (zzv_status == HSTAT_RCV) \
							_BIS (TACCTL1, CCIE); \
					} \
				} while (0)

#endif
