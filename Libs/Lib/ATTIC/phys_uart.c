#include "sysio.h"
#include "tcvphys.h"
#include "phys_uart.h"
/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

/* ============================ */
/* Special (framing) characters */
/* ============================ */
#define	CHR_SYN		0x16
#define	CHR_DLE		0x10
#define	CHR_STX		0x02
#define	CHR_ETX		0x03

#define	N_SYNS		4		/* The number of leading SYN's */

typedef	struct {

	byte	uart,		/* 0/1 */
		physid,
		mode,
		rxoff,
		txoff;

	word	qevent;

	/* ================== */
	/* Control parameters */
	/* ================== */
	union {
		struct {
			word	delmnrcv,
				delmxrcv,
				deltmrcv,
				delmnbkf,
				delbsbkf,
				delxmsen,
				backoff,
				rcvdelay;
		} mode0;
		struct {
			int	tlen, rlen;
			address	tpacket;
			char	*tptr, *rptr;
			char 	hdr [N_SYNS+2], trl [2];
		} mode1;
	} m;

	int	buflen;
	/* This will be dynamic */
	word	buffer [1];

} uartblock_t;

static int option0 (int, address);
static int option1 (int, address);

static	uartblock_t *uart [2] = { NULL, NULL };

/* Any distinct addresses will do for these ones */
#define	rxevent(u)	((word)&((u)->m.mode0.delmnrcv))
#define	txevent(u)	((word)&((u)->m.mode0.delmxrcv))

#define	ADD	YES
#define	DELETE	NO

static word gbackoff (uartblock_t *ub) {
/* ============================================ */
/* Generate a backoff after sensing an activity */
/* ============================================ */
	static word seed = 12345;

	seed = (seed + 1) * 6789;
	return ub->m.mode0.delmnbkf + (seed & ub->m.mode0.delbsbkf);
}

static	int rx (uartblock_t *ub, word t) {

	int len; word tmt; char c;
	char *buf = (char*)(ub->buffer);

	utimer (&tmt, ADD);
	len = 0;

	/* Wait for the first character */
    Retry:

	tmt = t;
	while (tmt && ion (ub->uart, READ, &c, 1) <= 0);

	if (tmt == 0) {
		/* We have failed */
		utimer (&tmt, DELETE);
		/* This means silence */
		return 0;
	}

	/*
	 * We are waiting for DLE STX. This makes it theoretically
	 * possible to mistake an escaped sequence like DLE DLE STX
	 * for a packet boundary, but let us take the risk.
	 */
	if (c != CHR_DLE)
		goto Retry;

	/*
	 * Second character of the pair
	 */
	tmt = UART_CHAR_TIMEOUT;
	while (tmt && ion (ub->uart, READ, &c, 1) <= 0);
	if (tmt == 0) {
		/* No more stuff */
		utimer (&tmt, DELETE);
		/* Signal an activity */
		return 1;
	}
	if (c != CHR_STX)
		goto Retry;
	/*
	 * Sniffing handled by a separate fuction. No hard timeouts here.
	 */
	while (len < ub->buflen) {
		tmt = UART_CHAR_TIMEOUT;
		while (ion (ub->uart, READ, buf + len, 1) <= 0) {
			if (tmt == 0) {
				utimer (&tmt, DELETE);
				/*
				 * If the packet ends on a character timeout
				 * as opposed to the correct end frame sequence,
				 * we have to conclude that we haven't received
				 * anything of value. However, we return 1 to
				 * indicate an activity.
				 */
				return 1;
			}
		}
		/* Handling DLE */
		if (buf [len] == CHR_DLE) {
			/*
			 * Repeating this looks like a waste of code, but it
			 * saves on states and variables.
			 */
			tmt = UART_CHAR_TIMEOUT;
			while (ion (ub->uart, READ, buf + len, 1) <= 0) {
				if (tmt == 0) {
					utimer (&tmt, DELETE);
					return 1;
				}
			}
			if (buf [len] != CHR_DLE) {
				/* Assume blindly this is ETX, what else? */
				utimer (&tmt, DELETE);
				tcvphy_rcv (ub->physid, ub->buffer, len);
				return 1;
			}
		}
		len++;
	}

	/* Length limit reached, it is a reception */
	utimer (&tmt, DELETE);
	tcvphy_rcv (ub->physid, ub->buffer, len);
	return 1;
}

static void tx (uartblock_t *ub, address buff, int len) {

	int k;
	char c, *buf = (char*)buff;

	for (c = CHR_SYN, k = 0; k < N_SYNS; k++) {
		while (ion (ub->uart, WRITE, &c, 1) <= 0);
	}
	c = CHR_DLE;
	while (ion (ub->uart, WRITE, &c, 1) <= 0);
	c = CHR_STX;
	while (ion (ub->uart, WRITE, &c, 1) <= 0);

	while (len) {
		if (*buf == CHR_DLE) {
			/* Escape */
			c = CHR_DLE;
			while (ion (ub->uart, WRITE, &c, 1) <= 0);
		}
		while (ion (ub->uart, WRITE, buf, 1) <= 0);
		buf++;
		len--;
	}

	c = CHR_DLE;
	while (ion (ub->uart, WRITE, &c, 1) <= 0);
	c = CHR_ETX;
	while (ion (ub->uart, WRITE, &c, 1) <= 0);
}

static	int sniff (uartblock_t *ub, word t) {

	char c;
	word tmt = t;	/* Make sure the timer is not in SDRAM */

	utimer (&tmt, ADD);

	/* Wait for the first character */
	while (tmt && ion (ub->uart, READ, &c, 1) <= 0);
	utimer (&tmt, DELETE);
	return (tmt != 0);
}

static process (xmtuart1, uartblock_t)

    int len;
    address packet;

    entry (0)

	if (data->txoff) {
		/* We are off */
		if (data->txoff == 3) {
			/* Drain */
			tcvphy_erase (data->physid);
			wait (data->qevent, 0);
			release;
		} else if (data->txoff == 1) {
			/* Queue held, transmitter off */
			data->m.mode0.backoff = 0;
			finish;
		}
	}

	if (data->m.mode0.backoff) {
		if (tcvphy_top (data->physid) > 1) {
			/* Urgent packet already pending, transmit it */
			data->m.mode0.backoff = 0;
		} else  {
			delay (data->m.mode0.backoff, 0);
			data->m.mode0.backoff = 0;
			/* Transmit an urgent packet when it shows up */
			wait (data->qevent, 1);
			release;
		}
	}

    ForceXmt:

	if ((packet = tcvphy_get (data->physid, &len)) != NULL) {
		if (data->rxoff == 0 ? rx (data, data->m.mode0.delxmsen) :
		    sniff (data, data->m.mode0.delxmsen)) {
			/* Activity, we backoff even if the packet was urgent */
			data->m.mode0.rcvdelay = data->m.mode0.delmnrcv;
			/* Delay blindly */
			delay (gbackoff (data), 0);
			trigger (rxevent (data));
			release;
		}
		/* Transmit */
		tx (data, packet, len);
		tcvphy_end (packet);
		/* Packet space obeyed blindly */
		delay (UART_PACKET_SPACE, 0);
		release;
	}

	if (data->txoff == 2) {
		/* Draining; stop xmt if the output queue is empty */
		data->txoff = 3;
		proceed (0);
	}

	wait (data->qevent, 0);
	wait (txevent (data), 0);
	release;

    entry (1)

	/* Urgent packet while obeying CAV */
	if (tcvphy_top (data->physid) > 1)
		goto ForceXmt;
	wait (data->qevent, 1);
	snooze (0);

endprocess (2)

static process (rcvuart1, uartblock_t)

    entry (1)

	if (rx (data, data->m.mode0.deltmrcv)) {
		data->m.mode0.backoff = gbackoff (data);
		trigger (txevent (data));
		data->m.mode0.rcvdelay = data->m.mode0.delmnrcv;
	} else {
		if (data->m.mode0.rcvdelay < data->m.mode0.delmxrcv)
			data->m.mode0.rcvdelay++;
	}

    entry (0)

	/* Receive event */
	if (data->rxoff) {
		data->m.mode0.rcvdelay = data->m.mode0.delmnrcv;
		finish;
	}
	delay (data->m.mode0.rcvdelay, 1);
	wait (rxevent (data), 0);
	release;

endprocess (2)

static process (xmtuart0, uartblock_t)

#define	sendchars(s)	entry (s) \
			    while (1) { \
				int k; \
				k = io (s, data->uart, WRITE, \
					data->m.mode1.tptr, \
					data->m.mode1.tlen); \
				if ((data->m.mode1.tlen -= k) <= 0) \
					break; \
				data->m.mode1.tptr += k; \
			    }
    entry (0)

	if (data->txoff) {
		switch (data->txoff) {
			case 1:
				/* Off, queue held */
				finish;
			case 2:
				/* Draining */
				if (tcvphy_top (data->physid) != 0)
					/* Process packet */
					break;
				/* Drained */
				data->txoff = 3;
			default:
				/* Drain */
				tcvphy_erase (data->physid);
				wait (data->qevent, 0);
				release;
		}
	} else {
		/* Keep going */
		if (tcvphy_top (data->physid) == 0) {
			wait (data->qevent, 0);
			release;
		}
	}

	data->m.mode1.tptr = data->m.mode1.hdr;
	data->m.mode1.tlen = N_SYNS+2;

    sendchars (1)

	data->m.mode1.tptr =
		(char*) (data->m.mode1.tpacket =
			tcvphy_get (data->physid, &data->m.mode1.tlen));
	sysassert (data->m.mode1.tpacket != NULL, "xmtuart0");

    sendchars (2)

	tcvphy_end (data->m.mode1.tpacket);

	data->m.mode1.tptr = data->m.mode1.trl;
	data->m.mode1.tlen = 2;

    sendchars (3)

	proceed (0);

#undef	sendchars

endprocess (2)

static process (rcvuart0, uartblock_t)

    char c;

    entry (0)

	if (data->rxoff) {
		finish;
	}

	do {
		io (0, data->uart, READ, &c, 1);
	} while (c != CHR_SYN);

    entry (1)

	do {
		io (1, data->uart, READ, &c, 1);
	} while (c == CHR_SYN);

	if (c != CHR_DLE)
		proceed (0);

    entry (2)

	io (2, data->uart, READ, &c, 1);
	if (c != CHR_STX)
		proceed (0);

	data->m.mode1.rlen = 0;

    entry (3)

	while (1) {
		io (3, data->uart, READ, &c, 1);
		if (c == CHR_DLE)
			break;
	    Cont:
		((char*)(data->buffer)) [data->m.mode1.rlen] = c;
		if (++(data->m.mode1.rlen) >= data->buflen) {
			tcvphy_rcv (data->physid, data->buffer,
				data->m.mode1.rlen);
			proceed (0);
		}
	}

    entry (4)

	io (4, data->uart, READ, &c, 1);
	if (c == CHR_DLE)
		/* Escape */
		goto Cont;

	/* End of packet, as DLE can only be followed by ETX */
	tcvphy_rcv (data->physid, data->buffer, data->m.mode1.rlen);
	proceed (0);

endprocess (2)

void phys_uart (int phy, int ua, int mode, int mbs) {
/*
 * This function must be called by the application to initialize the
 * interface. The first argument assigns a number to the interface, the
 * second identifies the UART (0, 1), the third describes the mode, and
 * the last one specifies the maximum size of a packet to be received.
 * Two modes are possible: 1 - emulating simple radio interface with
 * persistent transmission and recepetion, and 0 - smooth interrupt-driven
 * operation.
 */
	uartblock_t *ub;
	int k;

	if (ua < 0 || ua > 1 || mode < 0 || mode > 1)
		/* This must be a UART number */
		syserror (EREQPAR, "phys_uart uart");

	if (uart [ua] != NULL)
		/* We are allowed to do it only once per uart */
		syserror (ETOOMANY, "phys_uart");

	if (mbs == 0)
		mbs = UART_DEF_BUF_LEN;
	else if (mbs < 1)
		syserror (EREQPAR, "phys_uart mbs");

	if ((uart [ua] = ub = (uartblock_t*) umalloc (sizeof (uartblock_t) +
		mbs - 2)) == NULL)
			syserror (EMALLOC, "phys_uart");
	ub->buflen = mbs;
	ub->uart = ua;
	ub->physid = phy;
	if ((ub->mode = mode) == 0) {
		/* Set up the header and trailer */
		for (k = 0; k < N_SYNS; k++)
			ub->m.mode1.hdr [k] = CHR_SYN;
		ub->m.mode1.hdr [k++] = CHR_DLE;
		ub->m.mode1.hdr [k  ] = CHR_STX;
		ub->m.mode1.trl [0  ] = CHR_DLE;
		ub->m.mode1.trl [1  ] = CHR_ETX;
	} else {
		/*
		 * Default values of control parameters: they are irrelevant
		 * if mode is 1.
		 */
		ub->m.mode0.rcvdelay = ub->m.mode0.delmnrcv = UART_DEF_MNRCVINT;
		ub->m.mode0.delmxrcv = UART_DEF_MXRCVINT;
		ub->m.mode0.deltmrcv = UART_DEF_RCVTMT;
		ub->m.mode0.delmnbkf = UART_DEF_MNBACKOFF;
		ub->m.mode0.delbsbkf = UART_DEF_BSBACKOFF;
		ub->m.mode0.delxmsen = UART_DEF_TCVSENSE;
	}
	/* Put the UART into the proper state */
	ion (ua, CONTROL, (char*)&(ub->mode), UART_CNTRL_LCK);
	/* Register the phy */
	ub->qevent = tcvphy_reg (phy, ua ? option1 : option0,
		INFO_PHYS_UART | (mode << 4) | (ua));

	/* Start the driver processes */
	ub->rxoff = ub->txoff = 0;
	if (mode) {
		ub->m.mode0.backoff = 0;
		fork (xmtuart1, ub);
		fork (rcvuart1, ub);
	} else {
		fork (xmtuart0, ub);
		fork (rcvuart0, ub);
	}
}

static int option (uartblock_t *ub, int opt, address val) {
/*
 * Option processing
 */
	code_t cd;
	int ret = 0;

	switch (opt) {

	    case PHYSOPT_STATUS:

		ret = ((ub->txoff == 0) << 1) | (ub->rxoff == 0);
		if (val != NULL)
			*val = ret;
		break;

	    case PHYSOPT_TXON:

		ub->txoff = 0;
		cd = ub->mode ? xmtuart1 : xmtuart0;
		if (!find (cd, ub))
			fork (cd, ub);
		trigger (txevent (ub));
		break;

	    case PHYSOPT_RXON:

		ub->rxoff = 0;
		cd = ub->mode ? rcvuart1 : rcvuart0;
		if (!find (cd, ub))
			fork (cd, ub);
		trigger (rxevent (ub));
		break;

	    case PHYSOPT_TXOFF:

		/* Drain */
		ub->txoff = 2;
TxOff:
		trigger (txevent (ub));
		trigger (ub->qevent);
		break;

	    case PHYSOPT_TXHOLD:

		ub->txoff = 1;
		goto TxOff;

	    case PHYSOPT_RXOFF:

		ub->rxoff = 1;
		trigger (rxevent (ub));
		break;

	    case PHYSOPT_CAV:

		/* Force an explicit backoff */
		if (ub->mode == 0)
			/* Ignore in mode 0 */
			return 0;
		if (val == NULL)
			ub->m.mode0.backoff = 0;
		else
			ub->m.mode0.backoff = *val;
		trigger (txevent (ub));
		break;

	    case PHYSOPT_SENSE:

		if (ub->mode)
			ret = sniff (ub, ub->m.mode0.delxmsen);
		break;

	    case PHYSOPT_SETPARAM:

	      if (ub->mode) {
#define	pinx	(*val)
		/*
		 * This is the parameter index. The parameters are numbered:
		 *
		 *    0 - minimum inter-receive delay (min = 1 msec)
		 *    1 - maximum inter-receive delay (>= min, max = 32767)
		 *    2 - minimum backoff (min = 0 msec)
		 *    3 - backoff mask bits (from 1 to 15)
		 *    4 - sense time before xmit (min = 0, max = 1024 msec)
		 *    5 - receive attempt persistence (min = 1, max = 4096 msec)
		 */
#define pval	(*(val + 1))
		/*
		 * This is the value. We do some checking here and make sure
		 * that the values are within range.
		 */
		switch (pinx) {
			case 0:
				if (pval < 1)
					pval = 1;
				else if (pval > 32767)
					pval = 32767;
				ub->m.mode0.delmnrcv = pval;
				if (ub->m.mode0.delmxrcv < pval)
					ub->m.mode0.delmxrcv = pval;
				break;
			case 1:
				if (pval < 1)
					pval = 1;
				else if (pval > 32767)
					pval = 32767;
				ub->m.mode0.delmxrcv = pval;
				if (ub->m.mode0.delmnrcv > pval)
					ub->m.mode0.delmnrcv = pval;
				break;
			case 2:
				if (pval > 32767)
					pval = 32767;
				ub->m.mode0.delmnbkf = pval;
				break;
			case 3:
				if (pval > 15)
					pval = 15;
				if (pval)
					pval = (1 << pval) - 1;
				ub->m.mode0.delbsbkf = pval;
				break;
			case 4:
				if (pval > 1024)
					pval = 1024;
				ub->m.mode0.delxmsen = pval;
				break;
			case 5:
				if (pval < 1)
					pval = 1;
				else if (pval > 4096)
					pval = 4096;
				ub->m.mode0.deltmrcv = pval;
				break;
			default:
				syserror (EREQPAR, "options uart param index");
		}
#undef	pinx
#undef	pval
 	      }
	      ret = 1;
	      break;
	}
	return ret;
}

static int option0 (int opt, address val) {
	return option (uart [0], opt, val);
}

static int option1 (int opt, address val) {
	return option (uart [1], opt, val);
}
