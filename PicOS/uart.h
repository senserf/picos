#ifndef	__pg_uart_h
#define	__pg_uart_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Note: at present, this is only used by the phys uart. Later we will clean
// things up, such that the drivers (also for the non-phys version) will be
// machine independent.

#if UART_TCV

#include "phys_uart.h"
#include "uart_sys.h"

#define	UART_DEF_BUF_LEN	64

#define	UAFLG_OMAB		0x01		// Outgoing message AB
#define	UAFLG_OAAB		0x02		// Outgoing ACK AB
#define	UAFLG_SMAB		0x04		// AB to send
#define	UAFLG_EMAB		0x08		// Expected message AB
#define	UAFLG_UNAC		0x10		// Last out message unacked
#define	UAFLG_SACK		0x20		// Send ACK ASAP
#define	UAFLG_HOLD		0x40
#define	UAFLG_DRAI		0x80

#define	TXEVENT			((word)(&(UA->x_buffer)))
#define	RXEVENT			((word)(&(UA->r_buffer)))
#define	RSEVENT			((word)(&(UA->r_buffl)))
#define	OFFEVENT		((word)(&(UA->r_buffs)))
#define	ACKEVENT		((word)(&(UA->r_buffp)))

#define	XMTSPACE		(   20 + (entropy &   0xf))
#define	RCVSPACE		(   40 + (entropy &  0x1f))
#define	RXTIME			1024
#define	RETRTIME		( 1024 + (entropy & 0x1ff))

typedef	struct	{

	address	x_buffer;
	address r_buffer;

	byte	x_buffl,  x_buffp,
		x_chk0,   x_chk1,
		r_buffl,  r_buffs,
		r_buffp,  x_istate,
		r_istate, v_flags;

	word	v_physid, x_qevent;
	word	r_prcs,   x_prcs;

} uart_t;

extern	uart_t zz_uart [UART_TCV];

// IRQ states

#define	IRQ_R_OFF	0
#define	IRQ_R_STRT	1
#define	IRQ_R_LEN	2
#define	IRQ_R_PKT	3
#define	IRQ_R_CHK1	4

#define	IRQ_X_OFF	0
#define	IRQ_X_STRT	1
#define	IRQ_X_LEN	2
#define	IRQ_X_PKT	3
#define	IRQ_X_CH1	4
#define	IRQ_X_STOP	5




#endif	/* UART_TCV */

#endif