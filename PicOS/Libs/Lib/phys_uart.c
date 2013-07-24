/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "uart.h"

#if UART_TCV_MODE == UART_TCV_MODE_P
// Checksum (CRC) is mandatory in P mode
#undef	UART_TCV_MODE_NOCHECKSUM
#define	UART_TCV_MODE_NOCHECKSUM 0
#endif

#if UART_TCV_MODE == UART_TCV_MODE_L
// No CRC in L mode
#undef	UART_TCV_MODE_NOCHECKSUM
#define	UART_TCV_MODE_NOCHECKSUM 1
#endif

#if (UART_TCV_MODE == UART_TCV_MODE_N) && !defined(UART_TCV_MODE_NOCHECKSUM)
// In N mode, CRC is optional defaulting to YES
#define	UART_TCV_MODE_NOCHECKSUM 0
#endif

#if UART_TCV_MODE_NOCHECKSUM == 0
#include "checksum.h"
#endif

#if UART_TCV == 0
#error "S: phys_uart.c can only be compiled when UART_TCV > 0"
#endif

// ============================================================================

#if UART_TCV > 1

static int option (int, address, uart_t*);
static int option0 (int, address);
static int option1 (int, address);

#define	INI_UART(a)	ini_uart (a)

#else /* SINGLE UART */

static int option (int, address);
#define	INI_UART(a)	ini_uart ()

#endif	/* UART_TCV > 1 */

#if UART_RATE_SETTABLE
Boolean __pi_uart_setrate (word, uart_t*);
word __pi_uart_getrate (uart_t*);
#endif

// ============================================================================

#if UART_TCV_MODE == UART_TCV_MODE_N

// ============================================================================
// Non-persistent packet mode =================================================
// ============================================================================

#if UART_TCV > 1
#define	START_UART(a)	start_uart (a)
#else
#define	START_UART(a)	start_uart ()
#endif

#define	RC_LOOP		0
#define	RC_START	1
#define	RC_RESET	2
#define	RC_END		3

strand (rcvuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    entry (RC_LOOP)

	LEDIU (2, 0);

	when (OFFEVENT, RC_LOOP);
	if ((UA->v_flags & UAFLG_ROFF) == 0) {
		// Do not terminate (as it used to be)! We should keep the
		// process slot in case there is a congestion; it doesn't
		// cost much, and we are always ready; note that we do not
		// fork the process persistently
		when (RSEVENT, RC_START);
		UART_START_RECEIVER;
	}
	release;

    entry (RC_START)

	// This state must be interrupt safe
	LEDIU (2, 1);
	// We are past the length byte
	delay (RXTIME, RC_RESET);
	when (OFFEVENT, RC_LOOP);
	// The ordering of these requests does matter, as an interrupt can
	// unblock the last one. We need not be obsessive, as the likelihood
	// of missing a reception is not very high (the packet is probably
	// broken then, anyway), and we are timing out to reset, but if the
	// interrupt found this event pending before other events in this
	// state have been issued, things could become messy
	when (RXEVENT, RC_END);
	release;

    entry (RC_RESET)

	// Timeout, i.e., packet messup
	UART_STOP_RECEIVER;
	proceed (RC_LOOP);

    entry (RC_END)

	UART_STOP_RECEIVER;

	if (UA->r_buffer [0] == 0xffff) {
		// Length field error flag, reset the thing
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

#if UART_TCV_MODE_NOCHECKSUM == 0
	// Validate checksum: r_buffp is even
	if (w_chk (UA->r_buffer, UA->r_buffp >> 1, 0)) {
		// Wrong
		proceed (RC_LOOP);
	}
#endif

	// Receive the packet
	tcvphy_rcv (UA->v_physid, UA->r_buffer, UA->r_buffp);
	proceed (RC_LOOP);

endstrand

#define	XM_LOOP		0
#define	XM_END		1
#define	XM_WRCV		2
#define	XM_SXMT		3
#define	XM_OFF		4

strand (xmtuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    word stln;

    entry (XM_LOOP)

	LEDIU (1, 0);
	if ((UA->x_buffer = tcvphy_get (UA->v_physid, &stln)) == NULL) {
		// Nothing to transmit
		when (UA->x_qevent, XM_LOOP);
		release;
	}

#if defined(blue_ready) && defined(BLUETOOTH_UART)
// Do not send anything (drop packets) if the BlueTooth link is inactive.
// N mode doesn't implement non-transparent (command) mode of BT UART (only L
// mode can do that).
#if BLUETOOTH_UART == 0
	// On the first UART
	if (UA == __pi_uart && !blue_ready)
		goto Drop;
#else
	// On the second UART
	if (UA != __pi_uart && !blue_ready)
		goto Drop;
#endif
#endif /* BLUETOOTH_UART */

	if (stln <
#if UART_TCV_MODE_NOCHECKSUM
	  2
#else
	  4
#endif
	    || stln > UA->r_buffl || (stln & 1) != 0)
		syserror (EREQPAR, "xmtu/length");

	// In bytes
	UA->x_buffl = stln;
	stln >>= 1;
	if (UA->v_statid != 0xffff)
		UA->x_buffer [0] = UA->v_statid;

#if UART_TCV_MODE_NOCHECKSUM == 0
	// Checksum
	UA->x_buffer [stln - 1] = w_chk (UA->x_buffer, stln - 1, 0);
#endif

#ifdef UART_XMITTER_ON

    entry (XM_WRCV)

	// Half duplex, do not interfere with the receiver
	cli;
	if (UART_RCV_RUNNING) {
		when (RDYEVENT, XM_WRCV);
		sti;
		release;
	}
	UART_XMITTER_ON;
	sti;

	delay (UART_XMITTER_ON_DELAY, XM_SXMT);
	release;

    entry (XM_SXMT)

#endif
	LEDIU (1, 1);
	when (TXEVENT, XM_END);
	UART_START_XMITTER;
	release;

    entry (XM_END)

#ifdef UART_XMITTER_ON

	delay (UART_XMITTER_OFF_DELAY, XM_OFF);
	release;

    entry (XM_OFF)

	UART_XMITTER_OFF;
#endif

Drop:
	tcvphy_end (UA->x_buffer);

	proceed (XM_LOOP);

#undef UA

endstrand;

#if UART_TCV > 1
static void ini_uart (uart_t *ua) {
#define	WHICH	(ua == __pi_uart ? 0 : 1)
#else
static void ini_uart () {
#define	WHICH	0
#endif
/*
 * Initialize the device
 */
	diag ("UART %d, rate: %d00", WHICH, (int)(UART_RATE/100));
#undef	WHICH
}

#if UART_TCV > 1
static void start_uart (uart_t *ua) {
#define	UA ua
#else
static void start_uart () {
#define	UA __pi_uart
#endif

	// Transmitter
	UA->x_prcs = runstrand (xmtuart, UA);
	UA->r_prcs = runstrand (rcvuart, UA);

	if (UA->r_prcs == 0 || UA->x_prcs == 0)
		syserror (ERESOURCE, "phys_uart");

	UA->v_flags = 0;
#undef UA
}

void phys_uart (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (including statid, which counts to the payload,
 *	   excluding checksum)
 * which - which uart (0 or 1)
 */

#if UART_TCV > 1
#define	UA	(__pi_uart + which)
#else
#define	UA	__pi_uart
#endif

	if ((word)which >= UART_TCV)
		syserror (EREQPAR, "phys_uart");

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if ((mbs & 1) || (word)mbs > 252)
		syserror (EREQPAR, "phys_uart mbs");
	else if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;

	// Make sure the checksum is extra
	mbs += 2;

#if defined(blue_reset) && defined(BLUETOOTH_UART)
	if (which == BLUETOOTH_UART)
		blue_reset;
#endif

	if ((UA->r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	// Length in bytes, includes statid (counts to payload) and checksum
	UA->r_buffl = mbs;

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy,
#if UART_TCV > 1
		UA == __pi_uart ? option0 : option1,
#else
		option,
#endif

#ifdef BLUETOOTH_UART
			((which == BLUETOOTH_UART) ?
			  INFO_PHYS_UARTB :
			    INFO_PHYS_UART)
#else
			INFO_PHYS_UART
#endif
				+ which);

	INI_UART (UA);
	START_UART (UA);

#undef	UA
}

#if UART_TCV > 1
static int option0 (int opt, address val) {
	return option (opt, val, __pi_uart + 0);
}
static int option1 (int opt, address val) {
	return option (opt, val, __pi_uart + 1);
}
static int option (int opt, address val, uart_t *UA) {
#else
static int option (int opt, address val) {
#define	UA __pi_uart
#endif	/* UART_TCV > 1 */
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | ((UA->v_flags & UAFLG_ROFF) == 0);

		if (val != NULL)
			*val = ret;

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		break;

	    case PHYSOPT_ON:

		UA->v_flags &= ~UAFLG_ROFF;
TOffEv:
		trigger (OFFEVENT);
		break;

	    case PHYSOPT_OFF:

		UA->v_flags |= UAFLG_ROFF;
		goto TOffEv;

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

		if (__pi_uart_setrate (*val, UA)) {
			ret = *val;
			break;
		}
		syserror (EREQPAR, "phys_uart rate");

	    case PHYSOPT_GETRATE:

		ret = __pi_uart_getrate (UA);
		break;
#endif
	    case PHYSOPT_GETMAXPL:

		ret = UA->r_buffl - 2;
		break;

	    default:

		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}

#endif /* UART_TCV_MODE_N */

// ============================================================================
// ============================================================================
// ============================================================================
// ============================================================================

#if UART_TCV_MODE == UART_TCV_MODE_P

// ============================================================================
// Persistent packet mode (with built-in ACKs) ================================
// ============================================================================

// Note: this one doesn't accept BLUETOOTH at the moment, at least not in the
// same form as the other two modes

#if UART_TCV > 1
#define	START_UART(a)	start_uart (a)
#else
#define	START_UART(a)	start_uart ()
#endif

static const byte ackc [2][2] = { 0x21, 0x10, 0x63, 0x30 };

#define	XM_LOOP		0
#define	XM_END		1
#define	XM_NEXT		2
#define	XM_WRCV		3
#define	XM_SXMT		4
#define	XM_OFF		5

strand (xmtuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    word stln;

    entry (XM_LOOP)

	LEDIU (1, 0);
	// Hold (never drain)
	if ((UA->v_flags & UAFLG_ROFF)) {
		when (UA->x_qevent, XM_LOOP);
		release;
	}

	// Needed for the interrupt handler
	UA->x_buffp = 0;
	if ((UA->v_flags & UAFLG_UNAC) == 0) {
		// No previous unacked message
		if (UA->x_buffer != NULL)
			// Release previous buffer, if any
			tcvphy_end (UA->x_buffer);
		if ((UA->x_buffer = tcvphy_get (UA->v_physid, &stln)) == NULL) {
			// Send ACK
			UA->x_buffh = (UA->v_flags & UAFLG_EMAB) | UAFLG_SMAB;
			if ((UA->v_flags & UAFLG_EMAB)) {
				UA->x_chk0 = ackc [1][0];
				UA->x_chk1 = ackc [1][1];
			} else {
				UA->x_chk0 = ackc [0][0];
				UA->x_chk1 = ackc [0][1];
			}
			// The message is empty
			UA->x_buffl = UA->x_buffc = 0;
		} else {
			UA->x_buffh = (UA->v_flags & (UAFLG_EMAB | UAFLG_SMAB));
			UA->x_buffc = stln;
			if ((stln & 1)) 
				// The transmitted length will be even anyway
				stln++;
			if ((UA->x_buffl = (byte) stln) > UA->r_buffl - 4)
				syserror (EREQPAR, "xmtu/length");
			// Calculate checksum; note that this assumes a
			// particular layout of uart_t!!!
			stln = w_chk ((address)(&(UA->x_buffh)), 1, 0);
			stln = w_chk (UA->x_buffer, UA->x_buffl >> 1, stln);
			UA->x_chk0 = ((byte*)(&stln)) [0];
			UA->x_chk1 = ((byte*)(&stln)) [1];
			// Mark it as unacknowledged
			UA->v_flags |= UAFLG_UNAC;
		}
	} else {
		// There is a previous unacknowledged message: header/trailer
		// are ready ...
		if (((UA->v_flags ^ UA->x_buffh) & UAFLG_EMAB)) {
			// ... unless the expected AB has changed ...
			UA->x_buffh ^= UAFLG_EMAB;
			// ... so must recalculate checksum
			stln = w_chk ((address)(&(UA->x_buffh)), 1, 0);
			stln = w_chk (UA->x_buffer, UA->x_buffl >> 1, stln);
			UA->x_chk0 = ((byte*)(&stln)) [0];
			UA->x_chk1 = ((byte*)(&stln)) [1];
		}
	}

	// Acknowledgement sent (one way or the other), clear the flag
	UA->v_flags &= ~UAFLG_SACK;

#ifdef UART_XMITTER_ON

    entry (XM_WRCV)

	// Half duplex, do not interfere with the receiver
	cli;
	if (UART_RCV_RUNNING) {
		when (RDYEVENT, XM_WRCV);
		sti;
		release;
	}
	UART_XMITTER_ON;
	sti;

	delay (UART_XMITTER_ON_DELAY, XM_SXMT);
	release;

    entry (XM_SXMT)

#endif
	LEDIU (1, 1);
	when (TXEVENT, XM_END);
	UART_START_XMITTER;
	release;

    entry (XM_END)

#ifdef UART_XMITTER_ON

	delay (UART_XMITTER_OFF_DELAY, XM_OFF);
	release;

    entry (XM_OFF)

	UART_XMITTER_OFF;
#endif
	if ((UA->v_flags & UAFLG_ROFF))
		// Switched off
		proceed (XM_LOOP);

    entry (XM_NEXT)

	if ((UA->v_flags & UAFLG_SACK))
		// Sending ACK
		proceed (XM_LOOP);

	if (UA->v_flags & UAFLG_UNAC) {
		// Don't look at another message until this one is ACK-ed
		delay (RETRTIME, XM_LOOP);
		when (OFFEVENT, XM_END);
		when (ACKEVENT, XM_NEXT);
		release;
	}

	if (UA->x_buffer != NULL) {
		// Release any previous buffer
		tcvphy_end (UA->x_buffer);
		UA->x_buffer = NULL;
	}

	if (tcvphy_top (UA->v_physid) != NULL)
		proceed (XM_LOOP);

	when (UA->x_qevent, XM_LOOP);
	// This will come from the receiver
	when (ACKEVENT, XM_NEXT);
	// Send periodic ACKs in active mode
	if ((UA->v_flags & UAFLG_PERS))
		delay (RETRTIME, XM_LOOP);
#undef	UA

endstrand

#define	RC_LOOP		0
#define	RC_START	1
#define	RC_RESET	2
#define	RC_END		3

strand (rcvuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    byte b;
    word stln;

    entry (RC_LOOP)

	LEDIU (2, 0);
	when (OFFEVENT, RC_LOOP);

	if ((UA->v_flags & UAFLG_ROFF)) {
		UART_STOP_RECEIVER;
		// Off
	} else {
		when (RSEVENT, RC_START);
		UART_START_RECEIVER;
	}
	release;

    entry (RC_START)

	LEDIU (2, 1);
	delay (RXTIME, RC_RESET);
	when (RXEVENT, RC_END);
	when (OFFEVENT, RC_LOOP);
	release;

    entry (RC_RESET)

	UART_STOP_RECEIVER;
	proceed (RC_LOOP);

    entry (RC_END)

	if (UA->r_buffer [0] == 0xffff) {
		// Length field error
		UART_STOP_RECEIVER;
		delay (RCVSPACE, RC_LOOP);
		when (OFFEVENT, RC_LOOP);
		release;
	}
	// Validate checksum
	if (w_chk (UA->r_buffer, (UA->r_buffp >> 1) + 1, 0))
		// Garbage
		proceed (RC_LOOP);

	b = ((byte*)(UA->r_buffer)) [0];
	stln = ((byte*)(UA->r_buffer)) [1];
	if (stln == 0 && (b & 1) == 0)
		// Ack with MAB == 0 is illegal
		proceed (RC_LOOP);

	if ((((b >> 1) ^ UA->v_flags) & UAFLG_SMAB)) {
		// Expected != current
		if ((UA->v_flags & UAFLG_UNAC)) {
			// Xmitter waiting just for that
			UA->v_flags &= ~UAFLG_UNAC;
			trigger (ACKEVENT);
		}
		// Flip the outgoing bit
		UA->v_flags ^= UAFLG_SMAB;
	}

	// Now for the message
	if (stln == 0)
		// No message, just ACK
		proceed (RC_LOOP);

	// Check if the message is expected
	if ((((b << 1) ^ UA->v_flags) & UAFLG_EMAB) == 0) {
		// Yup, it is, pass it up
		if (tcvphy_rcv (UA->v_physid, UA->r_buffer + 1, stln))
			// Message has been accepted, flip EMAB
			UA->v_flags ^= UAFLG_EMAB;
	}
	// Send ACK (for this or previous message)
	UA->v_flags |= UAFLG_SACK;
	trigger (ACKEVENT);
	proceed (RC_LOOP);

endstrand
	
#if UART_TCV > 1
static void ini_uart (int which) {
#define	WHICH	which
#else
static void ini_uart () {
#define	WHICH	0
#endif
/*
 * Initialize the device
 */
	diag ("UART-P %d, rate: %d00", WHICH, (int)(UART_RATE/100));
	// Note: diag should be disabled if it gets in the way
#undef	WHICH
}

#if UART_TCV > 1
static void start_uart (int which) {
#define	UA __pi_uart + which
#else
static void start_uart () {
#define	UA __pi_uart
#endif

	UA->r_prcs = runstrand (rcvuart, UA);
	UA->x_prcs = runstrand (xmtuart, UA);

	if (UA->r_prcs == 0 || UA->x_prcs == 0)
		syserror (ERESOURCE, "phys_uart");

	UA->v_flags = 0;

#undef	UA
}

void phys_uart (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet (payload) length (excluding checksum and header)
 * which - which uart (0 or 1)
 */

#if UART_TCV > 1
#define	UA	__pi_uart + which
#else
#define	UA	__pi_uart
#endif
	if ((word)which >= UART_TCV)
		syserror (EREQPAR, "phys_uart");

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if ((word)mbs > 254 - 4)
		syserror (EREQPAR, "phys_uart mbs");
	else if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;
	if ((mbs & 1))
		mbs++;

	// Two bytes for header (preamble + length) + two bytes for checksum
	mbs += 4;

	if ((UA->r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	// If not NULL, the buffer must be released to TCV
	UA->x_buffer = NULL;

	// Length in bytes (total)
	UA->r_buffl = mbs;

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy,
#if UART_TCV > 1
		UA == __pi_uart ? option0 : option1,
#else
		option,
#endif
			INFO_PHYS_UARTP + which);

	INI_UART (which);
	START_UART (which);

#undef	UA
}

#if UART_TCV > 1
static int option0 (int opt, address val) {
	return option (opt, val, __pi_uart + 0);
}
static int option1 (int opt, address val) {
	return option (opt, val, __pi_uart + 1);
}
static int option (int opt, address val, uart_t *UA) {
#else
static int option (int opt, address val) {
#define	UA __pi_uart
#endif	/* UART_TCV > 1 */
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | ((UA->v_flags & UAFLG_ROFF) == 0);

		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_ON:

		UA->v_flags &= ~UAFLG_ROFF;
Wake:
		trigger (UA->x_qevent);
		trigger (OFFEVENT);

		break;

	    case PHYSOPT_HOLD:
	    case PHYSOPT_OFF:

		UA->v_flags |= UAFLG_ROFF;
		goto Wake;

	    case PHYSOPT_TXON:

		// This is used for active mode
		UA->v_flags |= UAFLG_PERS;
		goto Wake;

	    case PHYSOPT_TXOFF:

		UA->v_flags &= ~UAFLG_PERS;
		goto Wake;

#if UART_RATE_SETTABLE

	    case PHYSOPT_SETRATE:

		if (__pi_uart_setrate (*val, UA)) {
			ret = *val;
			break;
		}
		syserror (EREQPAR, "phys_uart rate");

	    case PHYSOPT_GETRATE:

		ret = __pi_uart_getrate (UA);
		break;
#endif

	    case PHYSOPT_GETMAXPL:

		ret = UA->r_buffl - 4;
		break;

	    default:

		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}

#endif /* UART_TCV_MODE_P */

#if UART_TCV_MODE == UART_TCV_MODE_L

// ============================================================================
// Line mode ==================================================================
// ============================================================================

#if UART_TCV > 1
#define	START_UART(a)	start_uart (a)
#else
#define	START_UART(a)	start_uart ()
#endif

#define	RC_LOOP		0
#define	RC_END		3

strand (rcvuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    entry (RC_LOOP)

	LEDIU (2, 0);

	when (OFFEVENT, RC_LOOP);

	if ((UA->v_flags & UAFLG_ROFF) == 0) {
		// If not off
		when (RXEVENT, RC_END);
		UART_START_RECEIVER;
	}
	release;

    entry (RC_END)

	UART_STOP_RECEIVER;

	// Receive the packet; we know for a fact that the line is nonempty;
	// no checksum to verify, odd length should be OK, but make sure there
	// is a sentinel, so the line can be processed by scan

	((byte*)(UA->r_buffer)) [UA->r_buffp++] = '\0';
	tcvphy_rcv (UA->v_physid, UA->r_buffer, UA->r_buffp);
	proceed (RC_LOOP);

endstrand

#define	XM_LOOP		0
#define	XM_END		1
#define	XM_WRCV		2
#define	XM_SXMT		3
#define	XM_OFF		4

strand (xmtuart, uart_t*)

#if UART_TCV > 1
#define	UA data
#else
#define	UA __pi_uart
#endif

    word stln;

    entry (XM_LOOP)

	LEDIU (1, 0);

	if ((UA->x_buffer = tcvphy_get (UA->v_physid, &stln)) == NULL) {
		// Nothing to transmit
		when (UA->x_qevent, XM_LOOP);
		release;
	}

#if defined(blue_ready) && defined(BLUETOOTH_UART)
// Do not send the line to BT unless connected (or in non-transparent mode)
	if (UA
#if BLUETOOTH_UART == 0
	// On the first UART
		==
#else
		!=
#endif
		   __pi_uart && !blue_ready && (UA->v_flags & UAFLG_NOTR) == 0)

			goto Drop;

#endif /* blue_ready */

	// Empty line allowed here, even though an empty line cannot be received
	UA->x_buffl = stln;

#ifdef UART_XMITTER_ON

    entry (XM_WRCV)

	// Half duplex, do not interfere with the receiver
	cli;
	if (UART_RCV_RUNNING) {
		when (RDYEVENT, XM_WRCV);
		sti;
		release;
	}
	UART_XMITTER_ON;
	sti;

	delay (UART_XMITTER_ON_DELAY, XM_SXMT);
	release;

    entry (XM_SXMT)

#endif
	LEDIU (1, 1);
	when (TXEVENT, XM_END);
	UART_START_XMITTER;
	release;

    entry (XM_END)

#ifdef UART_XMITTER_ON

	delay (UART_XMITTER_OFF_DELAY, XM_OFF);
	release;

    entry (XM_OFF)

	UART_XMITTER_OFF;
#endif

Drop:
	tcvphy_end (UA->x_buffer);
	proceed (XM_LOOP);

#undef UA

endstrand;

#if UART_TCV > 1
static void ini_uart (uart_t *ua) {
#define	WHICH	(ua == __pi_uart ? 0 : 1)
#else
static void ini_uart () {
#define	WHICH	0
#endif
/*
 * Initialize the device
 */
	diag ("UART %d, rate: %d00", WHICH, (int)(UART_RATE/100));
#undef	WHICH
}

#if UART_TCV > 1
static void start_uart (uart_t *ua) {
#define	UA ua
#else
static void start_uart () {
#define	UA __pi_uart
#endif

	UA->x_prcs = runstrand (xmtuart, UA);
	UA->r_prcs = runstrand (rcvuart, UA);

	if (UA->x_prcs == 0 || UA->r_prcs == 0)
		syserror (ERESOURCE, "phys_uart");

	UA->v_flags = 0;

#undef UA
}

void phys_uart (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (no NetId, no checksum in line mode), including
 *	   the sentinel byte
 * which - which uart (0 or 1)
 */

#if UART_TCV > 1
#define	UA	(__pi_uart + which)
#else
#define	UA	__pi_uart
#endif

	if ((word)which >= UART_TCV)
		syserror (EREQPAR, "phys_uart");

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if (mbs < 0 || mbs > 255)
		syserror (EREQPAR, "phys_uart mbs");
	else if (mbs < 4)
		mbs = UART_DEF_BUF_LEN;

#if defined(BLUETOOTH_UART) && defined(blue_reset)
	if (which == BLUETOOTH_UART)
		blue_reset;
#endif

	if ((UA->r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	// Length in bytes; minus one to make sure that the NULL byte sentinel
	// always fits
	UA->r_buffl = mbs - 1;

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy,
#if UART_TCV > 1
		UA == __pi_uart ? option0 : option1,
#else
		option,
#endif

#ifdef BLUETOOTH_UART
			((which == BLUETOOTH_UART) ?
			  INFO_PHYS_UARTLB :
			    INFO_PHYS_UARTL)
#else
			INFO_PHYS_UARTL
#endif
				+ which);

	INI_UART (UA);
	START_UART (UA);

#undef	UA
}

#if UART_TCV > 1
static int option0 (int opt, address val) {
	return option (opt, val, __pi_uart + 0);
}
static int option1 (int opt, address val) {
	return option (opt, val, __pi_uart + 1);
}
static int option (int opt, address val, uart_t *UA) {
#else
static int option (int opt, address val) {
#define	UA __pi_uart
#endif	/* UART_TCV > 1 */
/*
 * Option processing
 */
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = 2 | ((UA->v_flags & UAFLG_ROFF) == 0);

		if (val != NULL)
			*val = ret;

	    case PHYSOPT_TXON:
	    case PHYSOPT_TXOFF:
	    case PHYSOPT_TXHOLD:

		break;

	    case PHYSOPT_ON:

		UA->v_flags &= ~UAFLG_ROFF;
TOffEv:
		trigger (OFFEVENT);
		break;

	    case PHYSOPT_OFF:

		UA->v_flags |= UAFLG_ROFF;
		goto TOffEv;

#if UART_RATE_SETTABLE

	    case PHYSOPT_SETRATE:

		if (__pi_uart_setrate (*val, UA)) {
			ret = *val;
			break;
		}
		syserror (EREQPAR, "phys_uart rate");

	    case PHYSOPT_GETRATE:

		ret = __pi_uart_getrate (UA);
		break;
#endif
	    case PHYSOPT_GETMAXPL:

		ret = UA->r_buffl + 1;
		break;

	    default:

		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}

#endif /* UART_TCV_MODE_L */

#if DIAG_MESSAGES
#if DIAG_IMPLEMENTATION == 1

// Hooks for DIAG

#if UART_TCV < 2
#define	UA __pi_uart
#define	DIAG_WAIT	diag_wait (a)
#define	DIAG_WCHAR(c)	diag_wchar (c, a)
#else

#define UA (__pi_uart + ua)

#define	DIAG_WAIT	do { \
				if (ua) \
					diag_wait (b); \
				else \
					diag_wait (a); \
			} while (0)

#define	DIAG_WCHAR(c)	do { \
				if (ua) \
					diag_wchar (c, b); \
				else \
					diag_wchar (c, a); \
			} while (0)
#endif

void __pi_diag_init (int ua) {
//
// Preempt the UART for a diag message
//

#if UART_TCV_MODE == UART_TCV_MODE_L

	if (UA->x_istate != IRQ_X_OFF) {
		// Transmitter running, abort it
		UART_STOP_XMITTER;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);
	}

#ifdef UART_XMITTER_ON

	else {
		UART_XMITTER_ON;
		mdelay (UART_XMITTER_ON_DELAY);
	}
#endif

	DIAG_WCHAR ('\r'); DIAG_WAIT;

#else
	word bc;

	if (UA->x_istate != IRQ_X_OFF) {
		// Transmitter running, abort it
		UART_STOP_XMITTER;
		if (UA->x_prcs != 0)
			p_trigger (UA->x_prcs, TXEVENT);
		bc = UA->r_buffl + 8;
	} else {
		// Transmitter stopped
		bc = 4;

#ifdef UART_XMITTER_ON
		UART_XMITTER_ON;
		mdelay (UART_XMITTER_ON_DELAY);
#endif
	}
	// Send that many DLE's
	while (bc--) {
		DIAG_WCHAR (0x10);
		DIAG_WAIT;
	}
#endif

}

void __pi_diag_stop (int ua) {

#ifdef UART_XMITTER_OFF
	mdelay (UART_XMITTER_OFF_DELAY);
	UART_XMITTER_OFF;
#endif

}

#undef	UA
#undef	DIAG_WAIT
#undef	DIAG_WCHAR
		
#endif	/* DIAG_MESSAGES */

#endif	/* MODE_N or MODE_P */
