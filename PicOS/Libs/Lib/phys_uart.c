/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "uart.h"

#include "checksum.h"

#ifdef BLUETOOTH_PRESENT
#include "bluetooth.h"
#endif

/* ========================================= */

#if UART_TCV > 1

static int option (int, address, uart_t*);
static int option0 (int, address);
static int option1 (int, address);
#define	INI_UART(a)	ini_uart (a)
#define	START_UART(a,b)	start_uart (a,b)

#else

static int option (int, address);
#define	INI_UART(a)	ini_uart ()
#define	START_UART(a,b)	start_uart (b)

#endif

#if UART_RATE_SETTABLE
Boolean zz_uart_setrate (word, uart_t*);
word zz_uart_getrate (uart_t*);
#endif

#define	RC_LOOP		0
#define	RC_START	1
#define	RC_RESET	2
#define	RC_END		3

strand (rcvuart, uart_t)

#if UART_TCV > 1
#define	UA data
#else
#define	UA zz_uart
#endif

    entry (RC_LOOP)

	LEDI (2, 0);
	if (UA->v_flags & UAFLG_ROFF) {
		UA->r_prcs = 0;
		// Terminate
		finish;
	}

	when (OFFEVENT, RC_LOOP);
	when (RSEVENT, RC_START);

	UART_START_RECEIVER;
	release;

    entry (RC_START)

	LEDI (2, 1);
	// We are past the length byte
	delay (RXTIME, RC_RESET);
	when (RXEVENT, RC_END);
	when (OFFEVENT, RC_LOOP);
	release;

    entry (RC_RESET)

	// Timeout, i.e., packet messup
	UART_STOP_RECEIVER;
	proceed (RC_LOOP);

    entry (RC_END)

	UART_STOP_RECEIVER;

	if (UA->r_buffer [0] == 0xffff) {
		// Length field error, reset the thing
			delay (RCVSPACE, RC_LOOP);
			when (OFFEVENT, RC_LOOP);
			release;
	}

	// Check network ID
	if (UA->v_statid != 0 && UA->v_statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (UA->r_buffer [0] != 0 && UA->r_buffer [0] != UA->v_statid)
			// Drop
			proceed (RC_LOOP);
	}

	// Validate checksum: r_buffp is even
	if (w_chk (UA->r_buffer, UA->r_buffp >> 1, 0)) {
		// Wrong
		proceed (RC_LOOP);
	}

	// Receive the packet
	tcvphy_rcv (UA->v_physid, UA->r_buffer, UA->r_buffp);
	proceed (RC_LOOP);

endstrand

#define	XM_LOOP		0
#define	XM_END		1

strand (xmtuart, uart_t)

#if UART_TCV > 1
#define	UA data
#else
#define	UA zz_uart
#endif

    word stln;

    entry (XM_LOOP)

	LEDI (1, 0);
	if ((UA->v_flags & UAFLG_HOLD)) {
		// Solid OFF
		if ((UA->v_flags & UAFLG_DRAI)) {
			// Draining
Drain:
			UART_STOP_XMITTER;
			tcvphy_erase (UA->v_physid);
			when (UA->x_qevent, XM_LOOP);
			release;
		}
		// Queue held
		UA->x_prcs = 0;
		finish;
	}

	if ((UA->x_buffer = tcvphy_get (UA->v_physid, &stln)) == NULL) {
		// Nothing to transmit
		if ((UA->v_flags & UAFLG_DRAI)) {
			UA->v_flags |=  UAFLG_HOLD;
			goto Drain;
		}
		when (UA->x_qevent, XM_LOOP);
		release;
	}

#ifdef BLUETOOTH_PRESENT
/*
 * Do not send anything is the BlueTooth link is inactive
 */
#if BLUETOOTH_PRESENT == 1
	// On the first UART
	if (UA == zz_uart && !blue_ready)
		proceed (XM_LOOP);
#else
	// On the second UART
	if (UA != zz_uart && !blue_ready)
		proceed (XM_LOOP);
#endif
#endif /* BLUETOOTH_PRESENT */

	if (stln < 4 || (stln & 1) != 0)
		syserror (EREQPAR, "xmtu/length");

	LEDI (1, 1);

	// In bytes
	UA->x_buffl = stln;
	stln >>= 1;
	// Checksum
	if (UA->v_statid != 0xffff)
		UA->x_buffer [0] = UA->v_statid;
	UA->x_buffer [stln - 1] = w_chk (UA->x_buffer, stln - 1, 0);

	when (TXEVENT, XM_END);
	UART_START_XMITTER;
	release;

    entry (XM_END)

	tcvphy_end (UA->x_buffer);
	proceed (XM_LOOP);

#undef UA

endstrand;

#if UART_TCV > 1
static void ini_uart (uart_t *ua) {
#define	WHICH	(ua == zz_uart ? 0 : 1)
#else
static void ini_uart () {
#define	WHICH	0
#endif
/*
 * Initialize the device
 */
	diag ("UART %d initialized, rate: %d00, bits: %d, parity: %s",
		WHICH,
		UART_RATE/100, UART_BITS, (UART_BITS == 8) ? "none" :
		(UART_PARITY ? "odd" : "even"));
#undef	WHICH
}

#if UART_TCV > 1
static void start_uart (uart_t *ua, word what) {
#define	UA ua
#else
static void start_uart (word what) {
#define	UA zz_uart
#endif

	if (what & 0x1) {
		// Transmitter
		if (UA->x_prcs == 0)
			UA->x_prcs = runstrand (xmtuart, UA);
		UA->v_flags &= ~(UAFLG_HOLD + UAFLG_DRAI);
		trigger (UA->x_qevent);
	}

	if (what & 0x2) {
		// Receiver
		if (UA->r_prcs == 0)
			UA->r_prcs = runstrand (rcvuart, UA);
		UA->v_flags &= ~UAFLG_ROFF;
		trigger (OFFEVENT);
	}
#undef UA
}

void phys_uart (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (including checksum and statid)
 * which - which uart (0 or 1)
 */

#if UART_TCV > 1
#define	UA	(zz_uart + which)
#else
#define	UA	zz_uart
#endif

	if (which < 0 || which >= UART_TCV)
		syserror (EREQPAR, "phys_uart");

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if (mbs < 0 || mbs > 254)
		syserror (EREQPAR, "phys_uart mbs");
	else if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;

	mbs = (((mbs + 1) >> 1) << 1);

#ifdef BLUETOOTH_PRESENT
	if (which == BLUETOOTH_PRESENT - 1) {
		ini_blue_regs;
		blue_reset;
	}
#endif

	if ((UA->r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	UA->r_prcs = UA->x_prcs = 0;

	// Length in bytes
	UA->r_buffl = mbs;

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy,
#if UART_TCV > 1
		UA == zz_uart ? option0 : option1,
#else
		option,
#endif

#ifdef BLUETOOTH_PRESENT
			((which == BLUETOOTH_PRESENT - 1) ?
			  INFO_PHYS_UARTB :
			    INFO_PHYS_UART)
#else
			INFO_PHYS_UART
#endif
				+ which);

	INI_UART (UA);
	START_UART (UA, 0x3);
	// Start in the OFF state
	UA->v_flags |= UAFLG_HOLD + UAFLG_DRAI + UAFLG_ROFF;

#undef	UA
}

#if UART_TCV > 1
static int option0 (int opt, address val) {
	return option (opt, val, zz_uart + 0);
}
static int option1 (int opt, address val) {
	return option (opt, val, zz_uart + 1);
}
static int option (int opt, address val, uart_t *UA) {
#else
static int option (int opt, address val) {
#define	UA zz_uart
#endif	/* UART_TCV > 1 */
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = (((UA->v_flags & (UAFLG_HOLD + UAFLG_DRAI)) != 0) << 1) |
		       ((UA->v_flags &  UAFLG_ROFF) != 0);

		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		START_UART (UA, 1);
		break;

	    case PHYSOPT_RXON:

		START_UART (UA, 2);
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		if ((UA->v_flags & (UAFLG_DRAI + UAFLG_HOLD)))
			break;
		UA->v_flags |= UAFLG_DRAI;
		trigger (UA->x_qevent);
		break;

	    case PHYSOPT_TXHOLD:

		if ((UA->v_flags & (UAFLG_DRAI + UAFLG_HOLD)))
			break;

		UA->v_flags |= UAFLG_HOLD;
		trigger (UA->x_qevent);
		break;

	    case PHYSOPT_RXOFF:

		UA->v_flags |= UAFLG_ROFF;
		trigger (OFFEVENT);
		break;

	    case PHYSOPT_SETSID:

		UA->v_statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) (UA->v_statid);
		if (val != NULL)
			*val = ret;
		break;

#if UART_RATE_SETTABLE

	    case PHYSOPT_SETRATE:

		if (zz_uart_setrate (*val, UA) {
			ret = *val;
			break;
		}
		syserror (EREQPAR, "phys_uart rate");

	    case PHYSOPT_GETRATE:

		ret = zz_uart_getrate (UA);
		break;
#endif
	    default:

		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}
