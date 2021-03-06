/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_dm2100_h
#define	__pg_dm2100_h	1

#include "phys_dm2100.h"
#include "dm2100_sys.h"
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

#ifndef	RADIO_LBT_DELAY
#define	RADIO_LBT_DELAY		8
#endif

#ifndef	RADIO_LBT_THRESHOLD
#define	RADIO_LBT_THRESHOLD	50
#endif

/*
 * Some of these constants can be changed to taste
 */
#define	RADIO_DEF_BUF_LEN	48	/* Default buffer length (bytes) */
#define	PREAMBLE_LENGTH		14	/* Preamble bits/2 */
#define	MINIMUM_PACKET_LENGTH	8	/* Minimum legitimate packet length */

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

#define	SH1	DM_RATE_R1
#define	SH2	DM_RATE_R2
#define	SH3	DM_RATE_R3
#define	SH4	DM_RATE_R4
#define	SH5	DM_RATE_R5	// Timeout

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
#define IRQ_RSV		11
#define IRQ_RPK		12

#define	HSTAT_SLEEP	0
#define	HSTAT_RCV	1
#define	HSTAT_XMT	2

// Note: e is a constant, so the condition will be optimized out
#define	gbackoff(e) 	do { if (e) __pi_x_backoff = RADIO_LBT_MIN_BACKOFF + \
				(rnd () & ((1 << (e)) - 1)); } while (0)

#define	start_rcv	do { \
				__pi_v_prmble = 0; \
				__pi_r_length = 0; \
				__pi_v_istate = IRQ_RPR; \
				__pi_v_status = HSTAT_RCV; \
				enable_rcv_timer; \
			} while (0)

#define	start_xmt	do { \
				LEDI (1, 1); \
				__pi_v_prmble = PREAMBLE_LENGTH; \
				__pi_v_istate = IRQ_XPR; \
				__pi_v_status = HSTAT_XMT; \
				enable_xmt_timer; \
			} while (0)

#define	end_rcv		do { } while (0)
				
#define receiver_busy	(__pi_v_istate > IRQ_RPR)
#define	receiver_active	(__pi_v_status == HSTAT_RCV)
#define	xmitter_active	(__pi_v_status == HSTAT_XMT)

extern word	*__pi_r_buffer, *__pi_r_buffp, *__pi_r_buffl,
		*__pi_x_buffer, *__pi_x_buffp, *__pi_x_buffl, __pi_v_tmaux,
		__pi_v_qevent, __pi_v_physid, __pi_v_statid, __pi_x_backoff;

extern byte	__pi_v_status, __pi_v_istate, __pi_v_prmble, __pi_v_curbit,
		__pi_r_length, __pi_v_curnib, __pi_v_cursym, __pi_v_rxoff,
		__pi_v_txoff;

extern const byte __pi_v_symtable [], __pi_v_nibtable [], __pi_v_srntable [];

#define	rxevent	((word)&__pi_r_buffer)
#define	txevent	((word)&__pi_x_buffer)

// To trigger P1.0-P1.3 up events
#define	DM2100PINS_INT	((word)(&__pi_v_tmaux))

#endif
