/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_dm2200_h
#define	__pg_dm2200_h	1

#include "phys_dm2200.h"
#include "dm2200_sys.h"
#include "rfleds.h"

#ifndef	RADIO_LBT_MIN_BACKOFF
#define	RADIO_LBT_MIN_BACKOFF	8
#endif

#ifndef	RADIO_LBT_BACKOFF_EXP
#define	RADIO_LBT_BACKOFF_EXP	8
#endif

#ifndef	RADIO_LBT_BACKOFF_RX
#define	RADIO_LBT_BACKOFF_RX	6
#endif

#ifndef RADIO_LBT_THRESHOLD
#define RADIO_LBT_THRESHOLD     50
#endif

/*
 * Some of these constants can be changed to taste
 */
#define	RADIO_DEF_BUF_LEN	48	/* Default buffer length (bytes) */
#define	PREAMBLE_LENGTH		14	/* Preamble bits/2 */
#define	MINIMUM_PACKET_LENGTH	8	/* Minimum legitimate packet length */
#define	DM2200_DEF_RCVMODE	0x01	/* Flat range, no high sensitivity */
#define	COLLECT_RXDATA_ON_LOW	0	/* RXDATA taken on H-L clk transition */

#define	DISABLE_CLOCK_INTERRUPT	0	/* DEBUG ONLY !!!! */

#ifndef	FCC_TEST_MODE
#define	FCC_TEST_MODE		0
#endif

/*
 * Maximum mode setting. The mode (as per SETMODE) refers to the receiver
 * mode as described by bits 1 and 2 of CFG0. For example, with mode 3, both
 * bits are set; with mode 2, bit 2 is set and 1 is not.
 *
 *           X X X X X X X X    CFG0
 *                     ===      Mode
 */
#define	DM2200_N_RF_OPTIONS	0x07

/*
 * Configuration registers:
 *
 *                                                      our mode
 *                                                   =============
 *  0 CFG0   Sleep   TX/RX ASK/OOK  2.4GHz   Mode1   Mode0   RXHDR    SVEn
 *               0   0==RX  0==OOK       0       0       1       0       1
 *  1 CFG1   RXBlk   VCOLk  ISSMod    -        BR3     BR2     BR1     BR0
 *           R/O 0   R/O 0       1       0       0       0       1       1
 *  2 LOSYN   Test  LOSyn6  LOSyn5  LOSyn4  LOSyn3  LOSyn2  LOSyn1  LOSyn0
 *               0       0       0       0       0       0       0       0
 */

#ifndef	DM2200_CFG0
#define	DM2200_CFG0	0x48
#endif

#ifndef	DM2200_CFG1
#define	DM2200_CFG1	0x23
#endif

#define	CFG0_RCV_STOP	0x00
#define	CFG0_XMT	DM2200_CFG0
#define	CFG0_OFF	0x80

#define	CFG1		DM2200_CFG1
#define	LOSYN		0x00


#define	TRA(len)	switch ((len)) { \
				case 0:	 SL1; break; \
				case 1:  SL2; break; \
				case 2:  SL3; break; \
				default: SL4; \
			}

#define	SL1	set_signal_length (DM_RATE_X1)
#define	SL2	set_signal_length (DM_RATE_X2)
#define	SL3	set_signal_length (DM_RATE_X3)
#define	SL4	set_signal_length (DM_RATE_X4)
#define	SLE	set_signal_length (DM_RATE_XE)

/* States of the IRQ automaton */

#define IRQ_OFF		0
#define IRQ_XPR		1
#define IRQ_XPQ		2
#define IRQ_XPT		3
#define IRQ_XSV		4
#define IRQ_XSW		5
#define IRQ_XSX		6
#define IRQ_XPK		7
#define IRQ_XEP		8
#define IRQ_EXM		9

#define IRQ_RPR		10
#define IRQ_RP0		11
#define IRQ_RP1		12
#define IRQ_RP2		13
#define IRQ_RP3		14

#define	HSTAT_SLEEP	0
#define	HSTAT_RCV	1
#define	HSTAT_XMT	2

// Note: e is a constant, so the condition will be optimized out
#define	gbackoff(e) 	do { if (e) __pi_x_backoff = RADIO_LBT_MIN_BACKOFF + \
				(rnd () & ((1 << (e)) - 1)); } while (0)

#define	start_rcv	do { \
				__pi_r_length = 0; \
				__pi_v_istate = IRQ_RPR; \
				__pi_v_status = HSTAT_RCV; \
				rcv_setedge; \
				rcv_clrint; \
			} while (0)

#define	end_rcv		dm2200_wreg (0, CFG0_RCV_STOP)
				
#define	start_xmt	do { \
				LEDI (1, 1); \
				__pi_v_prmble = PREAMBLE_LENGTH; \
				__pi_v_istate = IRQ_XPR; \
				__pi_v_status = HSTAT_XMT; \
				enable_xmt_timer; \
			} while (0)

#define receiver_busy	(__pi_v_istate > IRQ_RPR)
#define	receiver_active	(__pi_v_status == HSTAT_RCV)
#define	xmitter_active	(__pi_v_status == HSTAT_XMT)

extern word	*__pi_r_buffer, *__pi_r_buffp, *__pi_r_buffl,
		*__pi_x_buffer, *__pi_x_buffp, *__pi_x_buffl,
		__pi_v_qevent, __pi_v_physid, __pi_v_statid, __pi_x_backoff;

extern byte	__pi_v_status, __pi_v_istate, __pi_v_prmble, __pi_v_curbit,
		__pi_r_length, __pi_r_rcvmode, __pi_v_curnib, __pi_v_cursym,
		__pi_v_rxoff, __pi_v_txoff;

extern const byte __pi_v_symtable [], __pi_v_nibtable [], __pi_v_srntable [];

#define	rxevent	((word)&__pi_r_buffer)
#define	txevent	((word)&__pi_x_buffer)

#endif
