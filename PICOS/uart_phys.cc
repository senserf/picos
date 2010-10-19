#ifndef __uart_phys_cc__
#define	__uart_phys_cc__

#include "board.h"
#include "stdattr.h"
#include "tcvphys.h"
#include "ualeds.h"
#include "tcv.cc"
#include "checksum.cc"

static int unp_option (int, address);

#define	UART_EVP_RCV	UA		// Receiver event
#define	UART_EVP_XMT	(UA->x_qevent)	// Transmitter event
#define	UART_EVP_ACK	(UA->v_statid)	// ACK event (for P mode)

//
// UART flags
//
#define	UAFLG_SMAB		0x01		// AB to send
#define	UAFLG_EMAB		0x02		// Expected message AB
#define	UAFLG_PERS		0x04		// Persisted mode
#define	UAFLG_ROFF		0x08		// Receiver OFF
#define	UAFLG_UNAC		0x10		// Last out message unacked
#define	UAFLG_SACK		0x20		// Send ACK ASAP
#define	UAFLG_HOLD		0x40
#define	UAFLG_DRAI		0x80

#define	UART_PKT_RETRTIME	1024

// ============================================================================
// Non-persistent packet mode
// ============================================================================

static byte upkt_getbyte (uart_tcv_int_t *UA, int redo, int off) {
//
// Get one byte from UART
//
	byte b;

	if ((UA->v_flags & UAFLG_ROFF)) {
		// If switched off
		Timer->wait (0, off);
		sleep;
	}

	when (UART_EVP_RCV, redo);
	io (redo, 0, READ, (char*)(&b), 1);
	unwait ();
	return b;
}

static void pkt_rdbuff (uart_tcv_int_t *UA, int redo, int brk, int off) {
//
// Fill the buffer
//
	int k;

	while (UA->r_buffs) {
		if ((UA->v_flags & UAFLG_ROFF)) {
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

static void pkt_ignore (uart_tcv_int_t *UA, int redo, int on) {
//
// Ignore UART input until switched on
//
	int k;

	while (1) {
		if ((UA->v_flags & UAFLG_ROFF) == 0) {
			// Back to ON
			Timer->wait (0, on);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		io (redo, 0, READ, (char*)(UA->r_buffer), UA->r_buffl);
	}
}

static void pkt_rreset (uart_tcv_int_t *UA, int redo, int done) {
//
// Ignore UART input until reset time
//
	int k;

	while (Time < UA->r_rstime) {
		if ((UA->v_flags & UAFLG_ROFF)) {
			Timer->wait (0, done);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		Timer->wait (UA->r_rstime - Time, redo);
		io (redo, 0, READ, (char*)(UA->r_buffer), UA->r_buffl);
	}
}

// ============================================================================

p_uart_rcv_n::perform {

    byte b;
    word len;

    _pp_enter_ ();

    state RC_LOOP:

	LEDIU (2, 0);

    transient RC_PREAMBLE:

	b = upkt_getbyte (UA, RC_PREAMBLE, RC_OFFSTATE);

	if (b != 0x55)
		proceed RC_PREAMBLE;

    transient RC_WLEN:

	LEDIU (2, 1);
	// Wait for the length byte
	b = upkt_getbyte (UA, RC_WLEN, RC_OFFSTATE);

	if ((b & 1) != 0 || b > UA->r_buffl - 4) {
		// Length field error, ignore everything for a few msec
		UA->r_rstime = Time + etuToItu (50 * MILLISECOND);
		proceed RC_RESET;
	}

	UA->r_buffs = b + 4;
	UA->r_buffp = (byte*)(UA->r_buffer);

    transient RC_FILL:

	pkt_rdbuff (UA, RC_FILL, RC_LOOP, RC_OFFSTATE);

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
	proceed RC_LOOP;

    state RC_OFFSTATE:

	LEDIU (2, 0);

    transient RC_WOFF:

	// Ignore everything waiting for on
	pkt_ignore (UA, RC_WOFF, RC_LOOP);
	// No return from this

    state RC_RESET:

	LEDIU (2, 0);

    transient RC_WRST:

	pkt_rreset (UA, RC_WRST, RC_LOOP);
}

// ============================================================================

p_uart_xmt_n::perform {

    int stln;
    byte b;

    _pp_enter_ ();

    state XM_LOOP:

	LEDIU (1, 0);

	if ((UA->v_flags & UAFLG_HOLD)) {
		// The HOLD flag == OFF solid
		if ((UA->v_flags & UAFLG_DRAI)) {
			// Draining
Drain:
			tcvphy_erase (UA->v_physid);
		}
		// Queue held
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	if ((UA->x_buffer = tcvphy_get ((int)(UA->v_physid), &stln)) == NULL) {
		// Nothing to transmit
		if ((UA->v_flags & UAFLG_DRAI)) {
			UA->v_flags |= UAFLG_HOLD;
			goto Drain;
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
// P mode
// ============================================================================

p_uart_rcv_p::perform {

    word len;
    byte b, hdr;

    _pp_enter_ ();

    state RC_LOOP:

	LEDIU (2, 0);

    transient RC_PREAMBLE:

	b = upkt_getbyte (UA, RC_PREAMBLE, RC_OFFSTATE);

	if ((b & 0xfc) != 0)
		// Keep waiting
		proceed RC_PREAMBLE;

	UA->r_buffp = (byte*)(UA->r_buffer);
	// First the preamble
	*(UA->r_buffp)++ = b;

    transient RC_WLEN:

	LEDIU (2, 1);
	// Wait for the length byte
	b = upkt_getbyte (UA, RC_WLEN, RC_OFFSTATE);

	if (b > UA->r_buffl - 4) {
		// Length error, ignore everything for a short while
		UA->r_rstime = Time + etuToItu (50 * MILLISECOND);
		proceed RC_RESET;
	}

	// How many more bytes: the payload + CRC
	UA->r_buffs = b + 2;
	if ((b & 1))
		// Accept odd length, but make sure the packet is word-aligned
		(UA->r_buffs)++;

	*(UA->r_buffp)++ = b;

    transient RC_FILL:

	pkt_rdbuff (UA, RC_FILL, RC_LOOP, RC_OFFSTATE);

	// Validate checksum
	if (w_chk (UA->r_buffer, (address)(UA->r_buffp) - UA->r_buffer,
	    0)) {
		// Bad checksum
		proceed (RC_LOOP);
	}

	// Protocol header
	hdr = ((byte*)(UA->r_buffer)) [0];
	// Actual length
	len = ((byte*)(UA->r_buffer)) [1];

	if (len == 0 & (hdr & 1) == 0)
		// ACK with MAB == 0 is illegal
		proceed (RC_LOOP);

	if ((((hdr >> 1) ^ UA->v_flags) & UAFLG_SMAB)) {
		// Expected != current
		if ((UA->v_flags & UAFLG_UNAC)) {
			// Xmitter waiting just for that
			UA->v_flags &= ~UAFLG_UNAC;
			trigger (UART_EVP_ACK);
		}
		// Flip the outgoing bit
		UA->v_flags ^= UAFLG_SMAB;
	}

	// Now for the message
	if (len == 0)
		// No message, just ACK
		proceed (RC_LOOP);

	// Check if the message is expected
	if ((((hdr << 1) ^ UA->v_flags) & UAFLG_EMAB) == 0) {
		// Yup, it is, pass it up
		if (tcvphy_rcv (UA->v_physid, UA->r_buffer + 1, len))
			// Message has been accepted, flip EMAB
			UA->v_flags ^= UAFLG_EMAB;
	}
	// Send ACK (for this or previous message)
	UA->v_flags |= UAFLG_SACK;
	trigger (UART_EVP_ACK);
	proceed (RC_LOOP);

    state RC_OFFSTATE:

	LEDIU (2, 0);

    transient RC_WOFF:

	pkt_ignore (UA, RC_WOFF, RC_LOOP);
	// No return

    state RC_RESET:

	LEDIU (2, 0);

    transient RC_WRST:

	pkt_rreset (UA, RC_WRST, RC_LOOP);
}

//
// Two variants of pure acknowledgments
//
static const byte upk_ack0 [] = { 0x01, 0x00, 0x21, 0x10 };
static const byte upk_ack1 [] = { 0x03, 0x00, 0x63, 0x30 };

p_uart_xmt_p::perform {

    int stln;
    word chs;
    byte b;

    _pp_enter_ ();

    state XM_LOOP:

	LEDIU (1, 0);
	if ((UA->v_flags & UAFLG_ROFF)) {
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	if ((UA->v_flags & UAFLG_UNAC) == 0) {
		// No previous unacked message
		if (UA->x_buffer != NULL)
			// Release previous buffer, if any
			tcvphy_end (UA->x_buffer);
		if ((UA->x_buffer = tcvphy_get ((int)(UA->v_physid), &stln)) ==
		    NULL) {
			// Send ACK
			UA->x_buffp = (byte*) (((UA->v_flags & UAFLG_EMAB)) ?
				upk_ack1 : upk_ack0);
			sameas (XM_SACK);
		}

		Hdr [0] = (UA->v_flags & (UAFLG_EMAB | UAFLG_SMAB));
		Hdr [1] = (byte) stln;
		if ((stln & 1))
			// Even out the length
			stln++;
		UA->x_buffl = (word) stln;
		chs = w_chk ((address)Hdr, 1, 0);
		*((word*)Chk) = w_chk (UA->x_buffer, UA->x_buffl >> 1, chs);
		UA->v_flags |= UAFLG_UNAC;
	} else {
		// There is a previous unacknowledged message: header/trailer
		// are ready ...
		if (((UA->v_flags ^ Hdr [0]) & UAFLG_EMAB)) {
			// ... unless the expected AB has changed ...
			Hdr [0] ^= UAFLG_EMAB;
			// ... so must recalculate checksum
			chs = w_chk ((address)Hdr, 1, 0);
			*((word*)Chk) = w_chk (UA->x_buffer, UA->x_buffl >> 1,
				chs);
		}
	}

	// Acknowledgement will be sent (one way or the other), clear the flag
	UA->v_flags &= ~UAFLG_SACK;

    transient XM_HDR:

	io (XM_HDR, 0, WRITE, (char*) Hdr, 1);

    transient XM_LEN:

	io (XM_LEN, 0, WRITE, (char*) Hdr + 1, 1);
	UA->x_buffp = (byte*)(UA->x_buffer);
	// Cannot touch the original length as the packet may have to be
	// retransmitted
	UA->v_statid = UA->x_buffl;

    transient XM_SEND:

	stln = io (XM_SEND, 0, WRITE, (char*)(UA->x_buffp), UA->v_statid);
	if ((UA->v_statid -= stln) != 0) {
		UA->x_buffp += stln;
		proceed XM_SEND;
	}

    transient XM_CHK:

	io (XM_CHK, 0, WRITE, (char*) Chk, 1);

    transient XM_CHL:

	io (XM_CHL, 0, WRITE, (char*) Chk + 1, 1);

    transient XM_END:

	// End of transmission
	if ((UA->v_flags & UAFLG_ROFF))
		// Switched off
		proceed (XM_LOOP);

    transient XM_NEXT:

	if ((UA->v_flags & UAFLG_SACK))
		// Sending ACK
		proceed (XM_LOOP);

	if ((UA->v_flags & UAFLG_UNAC)) {
		// Don't look at another message until this one ACK-ed
		delay (UART_PKT_RETRTIME, XM_LOOP);
		when (UART_EVP_XMT, XM_END);
		when (UART_EVP_ACK, XM_NEXT);
		sleep;
	}

	if (UA->x_buffer != NULL) {
		// Release any previous buffer
		tcvphy_end (UA->x_buffer);
		UA->x_buffer = NULL;
	}

	if (tcvphy_top ((int)(UA->v_physid)) != NULL)
		proceed (XM_LOOP);

	when (UART_EVP_XMT, XM_LOOP);
	// This will come from the receiver
	when (UART_EVP_ACK, XM_NEXT);
	// Send periodic ACKs in active mode
	if ((UA->v_flags & UAFLG_PERS))
		delay (UART_PKT_RETRTIME, XM_LOOP);

    state XM_SACK:

	io (XM_SACK, 0, WRITE, (char*)(UA->x_buffp), 1);
	(UA->x_buffp)++;

    transient XM_SACL:

	io (XM_SACL, 0, WRITE, (char*)(UA->x_buffp), 1);
	(UA->x_buffp)++;

    transient XM_SACM:

	io (XM_SACM, 0, WRITE, (char*)(UA->x_buffp), 1);
	(UA->x_buffp)++;

    transient XM_SACN:

	io (XM_SACN, 0, WRITE, (char*)(UA->x_buffp), 1);

	sameas (XM_END);
}

// ============================================================================
// Line mode
// ============================================================================

void p_uart_rcv_l::rdbuff (int redo, int off) {
//
// Fill the buffer
//
	char c;

	while (UA->r_buffs) {
		if ((UA->v_flags & UAFLG_ROFF)) {
			Timer->wait (0, off);
			sleep;
		}
		when (UART_EVP_RCV, redo);
		io (redo, 0, READ, &c, 1);
		unwait ();
		if (c == '\r' || c == '\n')
			return;
		if (UA->r_buffs) {
			UA->r_buffs--;
			*(UA->r_buffp)++ = (byte) c;
		}
	}
}

p_uart_rcv_l::perform {

    byte b;

    _pp_enter_ ();

    state RC_LOOP:

	LEDIU (2, 0);

    transient RC_FIRST:

	b = upkt_getbyte (UA, RC_FIRST, RC_OFFSTATE);
	// First character: ignore everything < space
	LEDIU (2, 1);
	if (b < 0x20)
		proceed (RC_FIRST);

	((byte*)(UA->r_buffer)) [0] = b;
	UA->r_buffp = (byte*)(UA->r_buffer) + 1;
	UA->r_buffs = UA->r_buffl - 1;

    transient RC_MORE:

	rdbuff (RC_MORE, RC_OFFSTATE);
	// The sentinel, there is always room for it
	*(UA->r_buffp)++ = '\0';
	tcvphy_rcv (UA->v_physid, UA->r_buffer,
		UA->r_buffp - (byte*)(UA->r_buffer));
	proceed RC_LOOP;

    state RC_OFFSTATE:

	LEDIU (2, 0);

    transient RC_WOFF:

	pkt_ignore (UA, RC_WOFF, RC_LOOP);

endstrand

// ============================================================================

p_uart_xmt_l::perform {

    int n, stln;
    char c;

    _pp_enter_ ();

    state XM_LOOP:

	LEDIU (1, 0);

	if ((UA->v_flags & UAFLG_HOLD)) {
		// The HOLD flag == OFF solid
		if ((UA->v_flags & UAFLG_DRAI)) {
			// Bit 1 set == draining
Drain:
			tcvphy_erase (UA->v_physid);
		}
		// Queue held
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	if ((UA->x_buffer = tcvphy_get ((int)(UA->v_physid), &stln)) == NULL) {
		// Nothing to transmit
		if ((UA->v_flags & UAFLG_DRAI)) {
			// Draining
			UA->v_flags |= UAFLG_HOLD;
			goto Drain;
		}
		when (UART_EVP_XMT, XM_LOOP);
		sleep;
	}

	// Empty line allowed here, even though an empty line cannot be received

	LEDIU (1, 1);

	// Look up a NULL byte; if present, the first NULL byte will terminate
	// the string
	for (n = 0; n < stln; n++) {
		if (((char*)(UA->x_buffer)) [n] == '\0') {
			stln = n;
			break;
		}
	}
	
	UA->x_buffl = (word) stln;
	UA->x_buffp = (byte*)(UA->x_buffer);

    transient XM_SEND:

	stln = io (XM_SEND, 0, WRITE, (char*)(UA->x_buffp), UA->x_buffl);
	if ((UA->x_buffl -= stln) != 0) {
		UA->x_buffp += stln;
		proceed XM_SEND;
	}

    transient XM_EOL1:

	c = '\r';
	io (XM_EOL1, 0, WRITE, &c, 1);

    transient XM_EOL2:

	c = '\n';
	io (XM_EOL2, 0, WRITE, &c, 1);

	// Done
	tcvphy_end (UA->x_buffer);
		proceed (XM_LOOP);
}

// ============================================================================

__PUBLF (PicOSNode, void, phys_uart) (int phy, int mbs, int which) {
/*
 * phy   - interface number
 * mbs   - maximum packet length (including statid, excluding checksum)
 * which - which uart (0 or 1) (must be zero in this version)
 */
	uart_tcv_int_t *UA;
	int ok;
	byte IMode;

	if (which != 0)
		// UART0 only
		syserror (EREQPAR, "phys_uart");

	IMode = uart->IMode;

	UA = UART_INTF_P (uart);

	if (UA->r_buffer != NULL)
		/* We are allowed to do it only once */
		syserror (ETOOMANY, "phys_uart");

	if (mbs < 4)
		mbs = UART_DEF_BUF_LEN;

	UA->v_flags = 0;

	if (IMode == UART_IMODE_L) {
		// Line mode
		if (mbs < 0 || mbs > 255) {
MBS:
			syserror (EREQPAR, "phys_uart mbs");
		}

		// Sentinel
		mbs--;

	} else if (IMode == UART_IMODE_N) {

		// N-packet mode
		if ((mbs & 1) || (word)mbs > 252)
			goto MBS;
		// Make sure the checksum is extra
		mbs += 2;

	} else {

		// P-packet mode
		if ((word)mbs > 254 - 4)
			syserror (EREQPAR, "phys_uart mbs");
		else if (mbs == 0)
			mbs = UART_DEF_BUF_LEN;
		if ((mbs & 1))
			mbs++;

		// Two bytes for header + two bytes for checksum
		mbs += 4;

		// If not NULL, the buffer must be released to TCV
		UA->x_buffer = NULL;
		UA->v_flags = 0;
	}

	UA->r_buffl = mbs;

	if ((UA->r_buffer = (address) memAlloc (mbs, (word) mbs)) == NULL)
		syserror (EMALLOC, "phys_uart");

	UA->v_physid = phy;

	/* Register the phy */
	UA->x_qevent = tcvphy_reg (phy, unp_option, INFO_PHYS_UART);

	// They are never killed, once started
	if (IMode == UART_IMODE_L)
		ok = runthread (p_uart_rcv_l) && runthread (p_uart_xmt_l);
	else if (IMode == UART_IMODE_N)
		ok = runthread (p_uart_rcv_n) && runthread (p_uart_xmt_n);
	else
		ok = runthread (p_uart_rcv_p) && runthread (p_uart_xmt_p);

	if (!ok)
		syserror (ERESOURCE, "phys_uart");
}

static int unp_option (int opt, address val) {
/*
 * Option processing
 */
	uart_tcv_int_t *UA;
	int ret;
	byte IMode;

	IMode = TheNode->uart->IMode;
	UA = UART_INTF_P (TheNode->uart);
	ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		if (IMode == UART_IMODE_P)
			ret = (UA->v_flags & UAFLG_ROFF) >> 3;
		else
			ret = (((UA->v_flags & (UAFLG_HOLD + UAFLG_DRAI)) != 0)
				<< 1) | ((UA->v_flags &  UAFLG_ROFF) != 0);

		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		if (IMode == UART_IMODE_P)
			// This is used for active (persistent) mode
			UA->v_flags |= UAFLG_PERS;
		else
			UA->v_flags &= ~(UAFLG_HOLD + UAFLG_DRAI);

		trigger (UART_EVP_XMT);
		break;

	    case PHYSOPT_RXON:

		// Used as global ON for P-mode
		UA->v_flags &= ~UAFLG_ROFF;
		if (IMode == UART_IMODE_P)
			trigger (UART_EVP_XMT);
		trigger (UART_EVP_RCV);
		break;

	    case PHYSOPT_TXOFF:

		if (IMode == UART_IMODE_P) {
			// This is used for active (persistent) mode
			UA->v_flags &= ~UAFLG_PERS;
		} else {
			if ((UA->v_flags & (UAFLG_DRAI + UAFLG_HOLD)))
				break;

			UA->v_flags |= UAFLG_DRAI;
		}
		trigger (UART_EVP_XMT);
		break;

	    case PHYSOPT_TXHOLD:

		if (IMode == UART_IMODE_P)
			goto RxOff;

		if ((UA->v_flags & (UAFLG_DRAI + UAFLG_HOLD)))
			break;

		UA->v_flags |= UAFLG_HOLD;
		trigger (UART_EVP_XMT);
		break;

	    case PHYSOPT_RXOFF:

		if (IMode == UART_IMODE_P) {
RxOff:
			if ((UA->v_flags & UAFLG_ROFF))
				break;

			UA->v_flags |= UAFLG_ROFF;
			trigger (UART_EVP_XMT);
		} else {
			UA->v_flags |= UAFLG_ROFF;
		}
		trigger (UART_EVP_RCV);
		break;

	    case PHYSOPT_SETRATE:

		TheNode->uart->U->setRate (*val);
		break;

	    case PHYSOPT_GETRATE:

		ret = TheNode->uart->U->getRate ();
		break;

	    case PHYSOPT_SETSID:

		if (IMode == UART_IMODE_L)
			goto Bad;
		UA->v_statid = (val == NULL) ? 0 : *val;
		break;

            case PHYSOPT_GETSID:

		if (IMode == UART_IMODE_L)
			goto Bad;
		ret = (int) (UA->v_statid);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_GETMAXPL:

		ret = UA->r_buffl;
		if (IMode == UART_IMODE_L)
			ret += 1;
		else if (IMode == UART_IMODE_P)
			ret -= 4;
		else
			ret -= 2;
		break;

	    default:
Bad:
		syserror (EREQPAR, "phys_uart option");

	}
	return ret;
}

#endif
