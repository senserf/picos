#ifndef	__pg_dm2200_h
#define	__pg_dm2200_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "phys_dm2200.h"
#include "dm2200_sys.h"
#include "rfleds.h"

/*
 * Some of these constants can be changed to taste
 */
#define	RADIO_DEF_BUF_LEN	48	/* Default buffer length (bytes) */
#define	PREAMBLE_LENGTH		14	/* Preamble bits/2 */
#define	MINIMUM_PACKET_LENGTH	8	/* Minimum legitimate packet length */
#define	DM2200_DEF_RCVMODE	0x01	/* Flat range, no high sensitivity */
#define	COLLECT_RXDATA_ON_LOW	1	/* RXDATA taken on H-L clk transition */

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

#define	CFG0_RCV_STOP	0x00
#define	CFG0_XMT	0x48
#define	CFG0_OFF	0x80

#define	CFG1		0x23
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

#define	gbackoff	(zzx_backoff = MIN_BACKOFF + rnd () & MSK_BACKOFF)

#define	start_rcv	do { \
				zzr_length = 0; \
				zzv_istate = IRQ_RPR; \
				zzv_status = HSTAT_RCV; \
				rcv_setedge; \
				rcv_clrint; \
			} while (0)

#define	end_rcv		dm2200_wreg (0, CFG0_RCV_STOP)
				
#define	start_xmt	do { \
				LEDI (1, 1); \
				zzv_prmble = PREAMBLE_LENGTH; \
				zzv_istate = IRQ_XPR; \
				zzv_status = HSTAT_XMT; \
				enable_xmt_timer; \
			} while (0)

#define receiver_busy	(zzv_istate > IRQ_RPR)
#define	receiver_active	(zzv_status == HSTAT_RCV)
#define	xmitter_active	(zzv_status == HSTAT_XMT)

extern word	*zzr_buffer, *zzr_buffp, *zzr_buffl,
		*zzx_buffer, *zzx_buffp, *zzx_buffl,
		zzv_qevent, zzv_physid, zzv_statid, zzx_backoff;

extern byte	zzv_status, zzv_istate, zzv_prmble, zzv_curbit, zzr_length,
		zzr_rcvmode, zzv_curnib, zzv_cursym, zzv_rxoff, zzv_txoff;

extern const byte zzv_symtable [], zzv_nibtable [], zzv_srntable [];

#define	rxevent	((word)&zzr_buffer)
#define	txevent	((word)&zzx_buffer)

#if	PULSE_MONITOR


#define	pmon	zz_pmon

#define	PMON_CNT_EDGE_UP	0x40	// Edge UP triggers counter
#define	PMON_NOT_EDGE_UP	0x80	// Edge UP triggers notifier
#define	PMON_CMP_ON		0x20	// Comparator is on
#define	PMON_CMP_PENDING	0x10	// Comparator event pending
#define	PMON_NOT_ON		0x08	// Notifier is on
#define	PMON_NOT_PENDING	0x04	// Notifier event pending
#define	PMON_CNT_ON		0x02	// Counter is on

#define	PCS_WPULSE		0	// Interrupts states
#define	PCS_WENDP		1
#define	PCS_WECYC		2
#define	PCS_WNEWC		3

#define	PMON_DEBOUNCE_UNIT	16	// 16 clock ticks
#define	PMON_RETRY_DELAY	255	// 1/4 sec (persistent status report)
#define	PMON_DEBOUNCE_CNT_ON	3	// 48 msec on
#define	PMON_DEBOUNCE_CNT_OFF	3	// 48 msec off
#define	PMON_DEBOUNCE_NOT_ON	4	// 64 msec on
#define	PMON_DEBOUNCE_NOT_OFF	100	// 2 sec off

#endif	/* PULSE_MONITOR */

#endif
