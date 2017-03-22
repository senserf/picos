#ifndef	__pg_uart_h
#define	__pg_uart_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Note: at present, this is only used by the phys uart. Later we will clean
// things up, such that the drivers (also for the non-phys version) will be
// machine independent.

#include "ualeds.h"

#if UART_TCV
#include "phys_uart.h"
#include "uart_sys.h"
#endif

#ifdef	N_UARTS_TCV				// Set in uart_sys.h

#define	UAFLG_SMAB		0x01		// AB to send
#define	UAFLG_EMAB		0x02		// Expected message AB
#define	UAFLG_PERS		0x04		// Persisted mode
#define	UAFLG_ROFF		0x08		// RCV off
#define	UAFLG_UNAC		0x10		// Last out message unacked
#define	UAFLG_SACK		0x20		// Send ACK ASAP
#define	UAFLG_NOTR		0x40		// Non-transparent BT (L-mode)
#define	UAFLG_ESCP		UAFLG_SACK	// Escape (DLE)
// Bit 0x80 is available


#define	TXEVENT			((aword)(&(UA->x_buffer)))
#define	RXEVENT			((aword)(&(UA->r_buffer)))
#define	RSEVENT			((aword)(&(UA->r_buffl)))
#define	OFFEVENT		((aword)(&(UA->r_buffs)))
#define	ACKEVENT		((aword)(&(UA->r_buffp)))
#define	RDYEVENT		((aword)(&(UA->r_istate)))

#define	RCVSPACE		50
#define	RXTIME			1024
#define	RETRTIME		1024

typedef	struct	{

	address	x_buffer;
	address r_buffer;

	byte	x_buffl,  x_buffp,
#if UART_TCV_MODE == UART_TCV_MODE_P
		x_buffh,  x_buffc,	// First two bytes (the order matters)
		x_chk0,   x_chk1,	// Checksum
#endif
		r_buffl,  r_buffs,
		r_buffp,  x_istate,
		r_istate, v_flags;
#if UART_TCV_MODE == UART_TCV_MODE_N
	word	v_statid;
#endif
	word	v_physid;
	aword	x_qevent, r_prcs, x_prcs;

#if UART_RATE_SETTABLE
	byte	flags;
#endif

} uart_t;

#define	UART_DEF_BUF_LEN	82

extern	uart_t __pi_uart [N_UARTS_TCV];

// IRQ states (the OFF states must be zero for proper initialization)

#define	IRQ_R_OFF	0
#define	IRQ_R_STRT	1
#define	IRQ_R_LEN	2
#define	IRQ_R_PKT	3
#define	IRQ_R_CHK1	4
#define	IRQ_R_LIN	IRQ_R_LEN

#define	IRQ_X_OFF	0
#define	IRQ_X_STRT	1
#define	IRQ_X_LEN	2
#define	IRQ_X_PKT	3
#define	IRQ_X_CH1	4
#define	IRQ_X_PAE	5
#define	IRQ_X_ETX	IRQ_X_CH1
#define	IRQ_X_LIN	IRQ_X_LEN
#define	IRQ_X_ESC	IRQ_X_LEN
#define	IRQ_X_EOL	IRQ_X_PKT

#endif	/* N_UARTS_TCV */

#endif
