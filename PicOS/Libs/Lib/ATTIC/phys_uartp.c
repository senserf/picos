/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "tcvphys.h"
#include "uart.h"

#include "checksum.h"

/* ========================================= */

#if UART_TCV > 1
static int option (int, address, uart_t*);
static int option0 (int, address);
static int option1 (int, address);
#define	INI_UART(a)	ini_uart (a)
#define	START_UART(a)	start_uart (a)
#else
static int option (int, address);
#define	INI_UART(a)	ini_uart ()
#define	START_UART(a)	start_uart ()
#endif

#if UART_RATE_SETTABLE
Boolean zz_uart_setrate (word, uart_t*);
word zz_uart_getrate (uart_t*);
#endif

static const byte ackc [2][2] = { 0x21, 0x10, 0x63, 0x30 };

#define	XM_LOOP		0
#define	XM_END		1
#define	XM_NEXT		2

strand (xmtuart, uart_t)

#if UART_TCV > 1
#define	UA data
#else
#define	UA zz_uart
#endif

    word stln;

    entry (XM_LOOP)

	if ((UA->v_flags & 0xc0)) {
		// Closing
		if (UA->x_buffer != NULL) {
			tcvphy_end (UA->x_buffer);
			UA->x_buffer = NULL;
		}
		if ((UA->v_flags & UAFLG_DRAI)) {
			// Draining
			UART_STOP_XMITTER;
			tcvphy_erase (UA->v_physid);
			wait (UA->x_qevent, XM_LOOP);
			release;
		}
		// UAFLG_HOLD: queue held, no activity
		UA->x_prcs = 0;
		finish;
	}

	UA->x_buffp = 0;
	if ((UA->v_flags & UAFLG_UNAC) == 0) {
		// No previous unacked message
		if (UA->x_buffer != NULL)
			// Release previous buffer, if any
			tcvphy_end (UA->x_buffer);
		if ((UA->x_buffer = tcvphy_get (UA->v_physid, &stln)) == NULL) {
			// Send ACK
			if ((UA->v_flags & UAFLG_EMAB)) {
				// Set the outgoing ACK AB to expected AB
				UA->v_flags |= UAFLG_OAAB;
				// Precalculated checksum
				UA->x_chk0 = ackc [1][0];
				UA->x_chk1 = ackc [1][1];
			} else {
				UA->v_flags &= ~UAFLG_OAAB;
				UA->x_chk0 = ackc [0][0];
				UA->x_chk1 = ackc [0][1];
			}
			// Outgoing message AB is always 1 for an ACK
			UA->v_flags |= UAFLG_OMAB;
			// The message is empty
			UA->x_buffl = 0;
		} else {
			// This is a new message; flip the outgoing AB
			if ((UA->v_flags & UAFLG_SMAB))
				UA->v_flags &= ~(UAFLG_SMAB | UAFLG_OMAB);
			else
				UA->v_flags |=  (UAFLG_SMAB | UAFLG_OMAB);
			UA->x_buffl = (byte) stln;
			if ((stln & 1)) {
				// We know that the buffer consists of an
				// entire number of words, so there is room
				// for one extra byte
				((byte*)(UA->x_buffer)) [stln] = 0xff;
			}
			// Set the outgoing ACK AB
			if ((UA->v_flags & UAFLG_EMAB))
				UA->v_flags |= UAFLG_OAAB;
			else
				UA->v_flags &= ~UAFLG_OAAB;
			// Calculate the checksum
			((byte*)(&stln)) [0] = (UA->v_flags & 0x03);
			((byte*)(&stln)) [1] = UA->x_buffl;
			stln = w_chk (&stln, 1, 0);
			stln = w_chk (UA->x_buffer, (UA->x_buffl + 1) >> 1,
				stln);
			UA->x_chk0 = ((byte*)(&stln)) [0];
			UA->x_chk1 = ((byte*)(&stln)) [1];
			// Mark it as unacknowledged
			UA->v_flags |= UAFLG_UNAC;
		}
	} else {
		// Previous unacknowledged message
		stln = 0;
		if ((UA->v_flags & UAFLG_EMAB)) {
			if ((UA->v_flags & UAFLG_OAAB) == 0) {
				// The expected AB has changed
				UA->v_flags |= UAFLG_OAAB;
				stln = 1;
			}
		} else {
			if ((UA->v_flags & UAFLG_OAAB)) {
				// The expected AB has changed
				UA->v_flags &= ~UAFLG_OAAB;
				stln = 1;
			}
		}
		if (stln) {
			// Must recalculate checksum
			((byte*)(&stln)) [0] = (UA->v_flags & 0x03);
			((byte*)(&stln)) [1] = UA->x_buffl;
			stln = w_chk (&stln, 1, 0);
			stln = w_chk (UA->x_buffer, (UA->x_buffl + 1) >> 1,
				stln);
			UA->x_chk0 = ((byte*)(&stln)) [0];
			UA->x_chk1 = ((byte*)(&stln)) [1];
		}
	}

	// Acknowledgement sent (one way or the other), clear the flag
	UA->v_flags &= ~UAFLG_SACK;
			
	wait (TXEVENT, XM_END);
	// Transmit
	UART_START_XMITTER;
	release;

    entry (XM_END)

	if ((UA->v_flags & 0xc0))
		// Switched off
		proceed (XM_LOOP);

	delay (XMTSPACE, XM_NEXT);
	wait (OFFEVENT, XM_END);

	release;

    entry (XM_NEXT)

	if ((UA->v_flags & UAFLG_SACK)) {
		// Sending ACK, delay a bit
		delay (XMTSPACE, XM_LOOP);
		release;
	}

	if ((UA->v_flags & UAFLG_UNAC)) {
		// Don't look at the next message until this one is acked
		delay (RETRTIME, XM_LOOP);
		wait (OFFEVENT, XM_END);
		wait (ACKEVENT, XM_NEXT);
		release;
	}

	if (UA->x_buffer != NULL) {
		// Release any previous buffer
		tcvphy_end (UA->x_buffer);
		UA->x_buffer = NULL;
	}

	if (tcvphy_top (UA->v_physid) != NULL)
		proceed (XM_LOOP);

	wait (UA->x_qevent, XM_LOOP);
	wait (ACKEVENT, XM_NEXT);

#undef	UA

endstrand

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

    byte b;
    word stln;

    entry (RC_LOOP)

	if ((UA->v_flags & 0xc0)) {
		UART_STOP_RECEIVER;
		// Off
		UA->r_prcs = 0;
		finish;
	}

	wait (OFFEVENT, RC_LOOP);
	wait (RSEVENT, RC_START);
	UART_START_RECEIVER;
	release;

    entry (RC_START)

	delay (RXTIME, RC_RESET);
	wait (RXEVENT, RC_END);
	wait (OFFEVENT, RC_LOOP);
	release;

    entry (RC_RESET)

	UART_STOP_RECEIVER;
	proceed (RC_LOOP);

    entry (RC_END)

	if (UA->r_buffer [0] == 0xffff) {
		UART_STOP_RECEIVER;
		delay (RCVSPACE, RC_LOOP);
		wait (OFFEVENT, RC_LOOP);
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

	if ((UA->v_flags & UAFLG_UNAC)) {
		// Look at the ACK bit
		if (((b & UAFLG_OAAB) && (UA->v_flags & UAFLG_OMAB) == 0) ||
		    ((b & UAFLG_OAAB) == 0 && (UA->v_flags & UAFLG_OMAB))) {
			// We are acked
			UA->v_flags &= ~UAFLG_UNAC;
			// Play the transmitter part and release the buffer
			trigger (ACKEVENT);
		}
	}

	// Now for the message
	if (stln == 0)
		// No message, just ACK
		proceed (RC_LOOP);

	// Check if the message is expected
	if (((b & UAFLG_OMAB) && (UA->v_flags & UAFLG_EMAB)) ||
	    ((b & UAFLG_OMAB) == 0 && (UA->v_flags & UAFLG_EMAB) == 0)) {
		// Yes, it is, pass it up
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
	diag ("UART-P %d initialized, rate: %d00, bits: %d, parity: %s",
		WHICH,
		UART_RATE/100, UART_BITS, (UART_BITS == 8) ? "none" :
		(UART_PARITY ? "odd" : "even"));
	// Note: diag should be disabled if it gets in the way
#undef	WHICH
}

#if UART_TCV > 1
static void start_uart (int which) {
#define	UA zz_uart + which
#else
static void start_uart () {
#define	UA zz_uart
#endif
	UA->v_flags &= ~(UAFLG_HOLD | UAFLG_DRAI);
	if (UA->r_prcs == 0)
		UA->r_prcs = runstrand (rcvuart, UA);
	if (UA->x_prcs == 0) {
		UA->x_prcs = runstrand (xmtuart, UA);
	}
	trigger (UA->x_qevent);
#undef	UA
}

void phys_uartp (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (including checksum and header)
 * which - which uart (0 or 1)
 */

#if UART_TCV > 1
#define	UA	zz_uart + which
#else
#define	UA	zz_uart
#endif
	if (which < 0 || which >= UART_TCV)
		syserror (EREQPAR, "phys_uartp");

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uartp");

	if (mbs < 0 || mbs > 256 - 4)
		syserror (EREQPAR, "phys_uartp mbs");
	else if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;

	mbs = (((mbs + 1) >> 1) << 1) + 4;

	if ((UA->r_buffer = umalloc (mbs)) == NULL)
		syserror (EMALLOC, "phys_uartp");

	// If not NULL, the buffer must be released to TCV
	UA->x_buffer = NULL;

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
			INFO_PHYS_UARTP + which);

	// This is flipped before first send
	UA->v_flags = UAFLG_SMAB;

	INI_UART (which);
	START_UART (which);

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

		ret = (UA->v_flags & 0xc0) >> 6;
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_ON:

		START_UART (UA);
		break;

	    case PHYSOPT_OFF:

		/* Drain */
		if ((UA->v_flags & (UAFLG_DRAI | UAFLG_HOLD)))
			break;

		UA->v_flags |= UAFLG_DRAI;
		trigger (OFFEVENT);
		break;

	    case PHYSOPT_HOLD:

		if ((UA->v_flags & (UAFLG_DRAI | UAFLG_HOLD)))
			break;

		UA->v_flags |= UAFLG_HOLD;
		trigger (OFFEVENT);
		break;

#if UART_RATE_SETTABLE

	    case PHYSOPT_SETRATE:

		if (zz_uart_setrate (*val, UA) {
			ret = *val;
			break;
		}
		syserror (EREQPAR, "phys_uartp rate");

	    case PHYSOPT_GETRATE:

		ret = zz_uart_getrate (UA);
		break;
#endif
	    default:

		syserror (EREQPAR, "phys_uartp option");

	}
	return ret;
}
