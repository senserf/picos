#ifndef	__pg_dm2100_h
#define	__pg_dm2100_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_dm2100.h"
#include "dm2100_sys.h"

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

#define	gbackoff	do { \
				zzx_seed = (zzx_seed + entropy + 1) * 6789; \
				zzx_backoff = MIN_BACKOFF + \
					(zzx_seed & MSK_BACKOFF); \
			} while (0)

#define	start_rcv	do { \
				zzv_prmble = 0; \
				zzr_length = 0; \
				zzv_istate = IRQ_RPR; \
				zzv_status = HSTAT_RCV; \
				enable_rcv_timer; \
			} while (0)

#define	start_xmt	do { \
				LEDI (1, 1); \
				zzv_prmble = PREAMBLE_LENGTH; \
				zzv_istate = IRQ_XPR; \
				zzv_status = HSTAT_XMT; \
				enable_xmt_timer; \
			} while (0)

#define	end_rcv		do { } while (0)
				
#define receiver_busy	(zzv_istate > IRQ_RPR)
#define	receiver_active	(zzv_status == HSTAT_RCV)
#define	xmitter_active	(zzv_status == HSTAT_XMT)

extern word	*zzr_buffer, *zzr_buffp, *zzr_buffl,
		*zzx_buffer, *zzx_buffp, *zzx_buffl, zzv_tmaux,
		zzv_qevent, zzv_physid, zzv_statid, zzx_seed, zzx_backoff;

extern byte	zzv_status, zzv_istate, zzv_prmble, zzv_curbit, zzr_length,
		zzv_curnib, zzv_cursym, zzv_rxoff, zzv_txoff;

extern const byte zzv_symtable [], zzv_nibtable [], zzv_srntable [];

#define	rxevent	((word)&zzr_buffer)
#define	txevent	((word)&zzx_buffer)

// To trigger P1.0-P1.3 up events
#define	DM2100PINS_INT	((word)(&zzv_tmaux))

#endif
