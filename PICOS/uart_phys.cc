#ifndef __uart_phys_cc__
#define	__uart_phys_cc__

#include "board.h"
#include "stdattr.h"
#include "tcvphys.h"
#include "ualeds.h"
#include "tcv.cc"
#include "checksum.cc"

static int unp_option (int, address);

// ============================================================================
// Non-persistent packet mode (this is the only option implemented in VUEE) ===
// ============================================================================

#define	UART_EVP_RCV	UA		// Receiver event
#define	UART_EVP_XMT	(UA->x_qevent)	// Transmitter event

byte p_uart_rcv_p::getbyte (int redo, int off) {
//
// Get one byte from UART
//
	byte b;

	if (UA->rx_off) {
		// If switched off
		Timer->wait (0, off);
		sleep;
	}

	when (UART_EVP_RCV, redo);
	io (redo, 0, READ, (char*)(&b), 1);
	unwait ();
	return b;
}

void p_uart_rcv_p::rdbuff (int redo, int brk, int off) {
//
// Fill the buffer
//
	int k;

	while (UA->r_buffs) {
		if (UA->rx_off) {
			Timer->wait (0, off);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		delay (1024, brk);
		k = io (redo, 0, READ, (char*) (UA->r_buffp), UA->r_buffs);
		unwait ();
		UA->r_buffs -= k;
		UA->r_buffp += k;
	}
}

void p_uart_rcv_p::ignore (int redo, int on) {
//
// Ignore UART input until switched on
//
	int k;

	while (1) {
		if (UA->rx_off == 0) {
			// Back to ON
			Timer->wait (0, on);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		io (redo, 0, READ, (char*)(UA->r_buffer), UA->r_buffl);
	}
}

void p_uart_rcv_p::rreset (int redo, int done) {
//
// Ignore UART input until reset time
//
	int k;

	while (Time < UA->r_rstime) {
		if (UA->rx_off) {
			Timer->wait (0, done);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		Timer->wait (UA->r_rstime - Time, redo);
		io (redo, 0, READ, (char*)(UA->r_buffer), UA->r_buffl);
	}
}

// ============================================================================

p_uart_rcv_p::perform {

    byte b;
    word len;

    state RC_LOOP:

	LEDIU (2, 0);

	b = getbyte (RC_LOOP, RC_OFFSTATE);

	if (b != 0x55)
		proceed RC_LOOP;

    transient RC_WLEN:

	LEDIU (2, 1);
	// Wait for the length byte
	b = getbyte (RC_WLEN, RC_OFFSTATE);

	if ((b & 1) != 0 || b > UA->r_buffl - 4) {
		// Length field error, ignore everything for a few msec
		UA->r_rstime = Time +
			etuToItu ((40 + toss (32)) * MILLISECOND);
		proceed RC_RESET;
	}

	UA->r_buffs = b + 4;
	UA->r_buffp = (byte*)(UA->r_buffer);

    transient RC_FILL:

	rdbuff (RC_FILL, RC_LOOP, RC_OFFSTATE);

	if (UA->v_statid != 0 && UA->v_statid != 0xffff) {
		// Admit only packets with agreeable statid
		if (UA->r_buffer [0] != 0 && UA->r_buffer [0] != UA->v_statid)
			// Drop
			proceed (RC_LOOP);
	}

	// This length is in bytes and is even
	len = UA->r_buffp - (byte*)(UA->r_buffer);

	if (w_chk (UA->r_buffer, len >> 1, 0)) {
		// Wrong
		proceed (RC_LOOP);
	}

	// Receive the packet
	tcvphy_rcv (UA->v_physid, UA->r_buffer, len);
	proceed (RC_LOOP);

    state RC_OFFSTATE:

	LEDIU (2, 0);

    transient RC_WOFF:

	// Ignore everything waiting for on
	ignore (RC_WOFF, RC_LOOP);
	// No return from this

    state RC_RESET:

	LEDIU (2, 0);

    transient RC_WRST:

	rreset (RC_WRST, RC_LOOP);
}

// ============================================================================

p_uart_xmt_p::perform {

    int stln;
    byte b;

    state XM_LOOP:

	LEDIU (1, 0);

	if ((UA->tx_off & 1)) {
		// The HOLD flag == OFF solid
		if (UA->tx_off > 1) {
			// Bit 1 set == draining
			tcvphy_erase (UA->v_physid);
		}
		// Queue held
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	if ((UA->x_buffer = tcvphy_get ((int)(UA->v_physid), &stln)) == NULL) {
		// Nothing to transmit
		if (UA->tx_off > 1) {
			// Draining
			UA->tx_off = 3;	// OFF
			proceed XM_LOOP;
		}
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	if (stln < 4 || (stln & 1) != 0)
		syserror (EREQPAR, "xmtu/length");

	LEDIU (1, 1);

	// In bytes
	UA->x_buffl = (word) stln;
	// In words
	stln >>= 1;
	// Checksum
	if (UA->v_statid != 0xffff)
		UA->x_buffer [0] = UA->v_statid;

	UA->x_buffer [stln - 1] = w_chk (UA->x_buffer, (word) stln - 1, 0);

    transient XM_PRE:

	// Write the preamble
	b = 0x55;
	io (XM_PRE, 0, WRITE, (char*)(&b), 1);

    transient XM_LEN:

	// The length byte
	b = (byte)(UA->x_buffl - 4);
	io (XM_LEN, 0, WRITE, (char*)(&b), 1);
	UA->x_buffp = (byte*)(UA->x_buffer);

    transient XM_SEND:

	stln = io (XM_SEND, 0, WRITE, (char*)(UA->x_buffp), UA->x_buffl);
	if ((UA->x_buffl -= stln) == 0) {
		tcvphy_end (UA->x_buffer);
		proceed (XM_LOOP);
	}
	UA->x_buffp += stln;
	proceed XM_SEND;
}

// ============================================================================

__PUBLF (PicOSNode, void, phys_uart) (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (including statid, excluding checksum)
 * which - which uart (0 or 1) (must be zero in this version)
 */
	uart_tcv_int_t *UA;

	if (which != 0)
		// UART0 only
		syserror (EREQPAR, "phys_uart");

	UA = UART_INTF_P (uart);

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if ((mbs & 1) || (word)mbs > 252)
		syserror (EREQPAR, "phys_uart mbs");
	else if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;

	// Make sure the checksum is extra
	mbs += 2;

	if ((UA->r_buffer = (address) memAlloc (mbs, (word) mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	// Length in bytes
	UA->r_buffl = mbs;

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy, unp_option, INFO_PHYS_UART);

	// Start in the OFF state
	UA->rx_off = 1;
	UA->tx_off = 3;

	// Start the processes
	if (!tally_in_pcs () || !tally_in_pcs ())
		syserror (ERESOURCE, "phys_uart");

	// They are never killed, once started
	create p_uart_rcv_p;
	create p_uart_xmt_p;
}

static int unp_option (int opt, address val) {
/*
 * Option processing
 */
	uart_tcv_int_t *UA;
	int ret;

	UA = UART_INTF_P (TheNode->uart);
	ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((UA->tx_off != 0) << 1) | (UA->rx_off != 0);

		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (UA->tx_off) {
			UA->tx_off = 0;
			trigger (UART_EVP_XMT);
		}
		break;

	    case PHYSOPT_RXON:

		if (UA->rx_off) {
			UA->rx_off = 0;
			trigger (UART_EVP_RCV);
		}
		break;

	    case PHYSOPT_TXOFF:

		if (UA->tx_off == 0) {
			// Drain
			UA->tx_off = 2;
			trigger (UART_EVP_XMT);
		}
		break;

	    case PHYSOPT_TXHOLD:

		if (UA->tx_off == 0) {
			// Hold
			UA->tx_off = 1;
			trigger (UART_EVP_XMT);
		}
		break;

	    case PHYSOPT_RXOFF:

		if (UA->rx_off == 0) {
			UA->rx_off = 1;
			trigger (UART_EVP_RCV);
		}

	    case PHYSOPT_SETRATE:

		TheNode->uart->U->setRate (*val);
		break;

	    case PHYSOPT_GETRATE:

		ret = TheNode->uart->U->getRate ();
		break;

	    case PHYSOPT_SETSID:

		UA->v_statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		ret = (int) (UA->v_statid);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_GETMAXPL:

		ret = UA->r_buffl;
		break;

	    default:

		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}

#endif
