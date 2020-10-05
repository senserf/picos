/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "kernel.h"
#include "tcvphys.h"
#include "cc3000.h"
#include "phys_cc3000.h"

#ifdef	cc3000_xc_ready

// ============================================================================
// SPI mode ===================================================================
// ============================================================================

#define	CC3000_SPI_MODE		1

static byte get_byte () {

	// Send a dummy byte of zeros; we are the master so we have to
	// keep the clock ticking
	cc3000_put (0);
	while (!cc3000_xc_ready);
#if CC3000_DEBUG > 3
	volatile byte b;
	b = cc3000_get;
	diag ("G: %x", b);
	return b;
#else
	return cc3000_get;
#endif
}

static void put_byte (byte b) {

	volatile byte s;
#if CC3000_DEBUG > 3
	diag ("P: %x", b);
#endif
	// udelay (200);
	cc3000_put (b);
	while (!cc3000_xc_ready);
	s = cc3000_get;
}

#else

// ============================================================================
// Direct pin mode ============================================================
// ============================================================================

#define	CC3000_SPI_MODE		0

static byte get_byte () {

	register int i;
	register byte b;

	for (b = 0, i = 0; i < 8; i++) {
		b <<= 1;
		cc3000_clkh;
		if (cc3000_inp)
			b |= 1;
		cc3000_clkl;
	}
#if CC3000_DEBUG > 3
	diag ("G: %x", b);
#endif
	return b;
}

static void put_byte (byte b) {

	register int i;

#if CC3000_DEBUG > 3
	diag ("P: %x", b);
#endif
	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			cc3000_outh;
		else
			cc3000_outl;
		cc3000_clkh;
		cc3000_clkl;
		b <<= 1;
	}
}

#endif

// ============================================================================

sint cc3000_event_thread = 0;		// must be global, used by the IRQ

static cc3000_server_params_t *sparams;	// server params (port + IP)
static cc3000_wlan_params_t   *wparams;	// network params (SSID and stuff)

static word physid, maxplen, buflen, keepalcnt;
static word pollint = CC3000_POLLINT_MIN;
#if WIFI_OPTIONS & WIFI_OPTION_NETID
static word statid = 0;
#endif
#if WIFI_KAL_INTERVAL
static word kalcnt = WIFI_KAL_INTERVAL;
#endif
static byte *buffer;
static byte socket;
static byte nbuffers, freebuffers;
static byte flags;
static byte dstate = CC3000_STATE_DEAD;

#define	DEVENT	((word)&dstate)

// ============================================================================
// ============================================================================
// Discovering the protocol:
//
// Cold start:
//
//	01 00 05 00 (del 50us) 00 01 00 40 01 01
//      op pl pl --            -- cm op op al ar
//
//	- SPI op == WRITE
//	- pl     == payload length (following the 5-byte header) MSB first
//	- --     == wait bytes (ignored)
//	- cm     == HCI message type (command)
//	- HCI op == word LSB first (operation simple link start)
//	- al     == arguments length
//	- ar     == argument (load patches)
//
// Ask for size of buffer pool:
//
//	01 00 05 00 00 01 0B 40 00 00
//
//	- write 5 hcicmd 400B noargs pad (must be even total)
//
// Set connect policy:
//
//	01 00 11 00 00 01 04 00 0C xx 00 00 00 yy 00 00 00 xx 00 00 00 00
//                                 0/1         0/1         0/1         pad
// Data write:
//
//	01 pl pl 00 00 02 op al pa pa ...
//
//	- pl (MSB first) applies to everything after the 5-byte hdr, incl pad
//	- 02 == HCITYPE_DATA
//	- op == opcode (single byte)
//	- al == args length
//	- pa == HCI payload length (LSB first), includes args

static void put_zero (word n) {

	while (n--)
		put_byte (0);
}

static void put_byteaslong (byte b) {

	put_byte (b);
	put_zero (3);
}

static void put_word (word w) {

	put_byte ((byte)w);
	put_byte ((byte)(w >> 8));
}

static void put_wordaslong (word w) {

	put_word (w);
	put_zero (2);
}

static void spi_wri (word length) {
//
// Start write operation; the argument is HCI length "rounded" up to an odd
// value
//
	cc3000_select;
	while (!cc3000_irq_pending);
	put_byte (CC3000_SPIOP_WRITE);
	// Here MSB goes first for some reason; not sure if we ever use the
	// more significant byte, but let us give it benefit of the doubt
	put_byte ((byte)(length >> 8));
	put_byte ((byte)length);
	put_zero (2);
}

static word spi_rea () {

	word len, tln;
	byte b;

	cc3000_select;

	put_byte (CC3000_SPIOP_READ);
	put_zero (2);

	len = get_byte ();
	len = (len << 8) | get_byte ();

	for (tln = 0; tln < len; tln++) {
		b = get_byte ();
		if (tln < buflen)
			buffer [tln] = b;
	}

	cc3000_unselect;

	return len;
}

void hci_cmd_start (word opcode, byte argslen) {
	
	spi_wri (argslen + 4 + ((argslen & 1) == 0));

	put_byte (CC3000_HCITYPE_CMD);

	// Here LSB goes first, to make it more interesting
	put_word (opcode);
	put_byte (argslen);
}

static void hci_cmd_end (argslen) {

	if ((argslen & 1) == 0)
		put_byte (0);
	cc3000_unselect;
}

static void hci_data_start (byte opcode, byte argslen, word datalen) {

	word paylen;

	paylen = argslen + datalen;

	spi_wri (paylen + 5 + (paylen & 1));
	put_byte (CC3000_HCITYPE_DATA);
	put_byte (opcode);
	put_byte (argslen);
	put_word (paylen);
#if CC3000_DEBUG > 2
	diag ("HCD: %x %u %u", opcode, argslen, paylen);
#endif
}

static void hci_data_end (word tlen) {

	if ((tlen & 1))
		put_byte (0);
	cc3000_unselect;
#if CC3000_DEBUG > 2
	diag ("HCE %u", tlen);
#endif
}

static void deadstart () {

	cc3000_unselect;
	cc3000_disable;
	cc3000_irq_disable;

	mdelay (200);

	while (cc3000_irq_pending);
	cc3000_enable;
	while (!cc3000_irq_pending);
	cc3000_select;
	put_byte (CC3000_SPIOP_WRITE);
	put_byte (0);
	put_byte (5);
	put_byte (0);
	udelay (60);
	put_byte (0);
	put_byte (CC3000_HCITYPE_CMD);
	put_word (CC3000_HCICMD_SLS);
	put_byte (1);
	put_byte (0);
	cc3000_unselect;
}

static void get_buffer_count () {

	hci_cmd_start (CC3000_HCICMD_RBS, 0);
	hci_cmd_end (0);
#if CC3000_DEBUG > 1
	diag ("GBF");
#endif
}

static void set_connect_policy () {

	byte policy;

	policy = (wparams != NULL) ? wparams->policy : CC3000_POLICY_OLD;

	hci_cmd_start (CC3000_HCICMD_SCP, 3*4);
	put_byteaslong (policy == CC3000_POLICY_ANY);
	put_byteaslong (policy == CC3000_POLICY_LAST);
	put_byteaslong (policy == CC3000_POLICY_PROFILE);
	hci_cmd_end (3*4);
#if CC3000_DEBUG
	diag ("POL %d", policy);
#endif
}

static void connect_to_ap () {

	byte alen, slen;
	char *sp;

	// This is the sum of SSID length and KEY length
	alen = strlen (sp = wparams->ssid_n_key);
#if CC3000_DEBUG
	diag ("CON %s %d %d %d", sp, alen, wparams->ssid_length,
		wparams->security_mode);
#endif
	hci_cmd_start (CC3000_HCICMD_WCN, 28 + alen);
	put_byteaslong (0x1c);	// Magic
	put_byteaslong (slen = wparams->ssid_length);
	put_byteaslong (wparams->security_mode);
	put_byteaslong (wparams->ssid_length + 0x10);
	// This is the key length
	put_byteaslong (alen - wparams->ssid_length);
	// More magic (and some stupid one, to make sure), i.e., 2 zero bytes
	// + 6 bytes of BSSID
	put_zero (2+6);
	// SSID + key
	while (*sp != '\0')
		put_byte ((byte)(*sp++));
	
	hci_cmd_end (alen);
}

static void create_socket () {

	hci_cmd_start (CC3000_HCICMD_SOK, 3*4);
	put_byteaslong (CC3000_AF_INET);
	put_byteaslong (CC3000_SOCKTYPE);
	put_byteaslong (CC3000_IPPROTO_IP);
	hci_cmd_end (3*4);
#if CC3000_DEBUG
	diag ("CSO");
#endif
}

// ============================================================================
#if WIFI_OPTIONS & WIFI_OPTION_TCP
// ============================================================================

static void connect_to_server () {

	word i;

	hci_cmd_start (CC3000_HCICMD_CON, 20);
	put_byteaslong (socket);
	put_byteaslong (8);		// Magic
	put_byteaslong (8);		// Address length
	put_word (CC3000_AF_INET);	// This goes as a 16-bit word
#if LITTLE_ENDIAN
	// Make sure the port is htons
	put_byte ((byte)(sparams->port >> 8));
	put_byte ((byte)(sparams->port     ));
#else
	put_word (sparams->port);
#endif
	for (i = 0; i < 4; i++)
		put_byte (sparams->ip [i]);
	hci_cmd_end (20);
#if CC3000_DEBUG
	diag ("SER %u %u %u.%u.%u.%u", socket, sparams->port,
		sparams->ip [0],
		sparams->ip [1],
		sparams->ip [2],
		sparams->ip [3]);
#endif
}

static void send (byte *buf, word len) {

	word pad;

	sysassert (len <= maxplen, "cc30 py");

	freebuffers--;

	hci_data_start (CC3000_HCICMD_SND, 16, len);
	put_byteaslong (socket);
	put_byteaslong (16-4);
	put_wordaslong (len);
	// Flags
	put_zero (4);

	for (pad = len; pad; pad--)
		put_byte (*buf++);

	hci_data_end (len);
#if CC3000_DEBUG
	diag ("SND: %u %d", len, freebuffers);
#endif
}

#else	/* UDP */

static void send (byte *buf, word len) {

	word i;

#if CC3000_DEBUG > 0
	diag ("S: %s", buf);
#endif
	sysassert (len <= maxplen, "cc30 py");

	freebuffers--;

	hci_data_start (CC3000_HCICMD_STO, 24, len + 16);
	put_byteaslong (socket);
	put_byteaslong (24-4);
	put_wordaslong (len);
	// Flags
	put_zero (4);
	put_wordaslong (len + 8);
	put_byteaslong (8);

	for (i = len; i; i--)
		put_byte (*buf++);

#if LITTLE_ENDIAN
	// Make sure these are htons
	put_byte ((byte)(CC3000_AF_INET >> 8));
	put_byte ((byte)(CC3000_AF_INET     ));
	put_byte ((byte)(sparams->port  >> 8));
	put_byte ((byte)(sparams->port      ));
#else
	put_word (CC3000_AF_INET);
	put_word (sparams->port);
#endif
	for (i = 0; i < 4; i++)
		put_byte (sparams->ip [i]);
	// Make it up to 16
	put_zero (8);
		
	hci_data_end (len);
#if CC3000_DEBUG > 1
	diag ("STO: %u %d", len, freebuffers);
#endif
}

// ============================================================================
#endif /* TCP or UDP */
// ============================================================================

static void read_version () {

	hci_cmd_start (CC3000_HCICMD_VER, 0);
	hci_cmd_end (0);
}

static void simple_select () {

	hci_cmd_start (CC3000_HCICMD_SEL, 44);
	put_byteaslong (1);	// nfds
	put_byteaslong (0x14);	// some stupid magic
	put_byteaslong (0x14);	// more stupid magic
	put_byteaslong (0x14);	// even more some stupid magic
	put_byteaslong (0x14);	// yet some more stupid magic
	put_zero (4);		// this is a binary flag (blocking)
	put_byteaslong (1);
	put_zero (8+6);
	// Now for the timeout
	// put_zero (4);	// 0 seconds and 0 milliseconds
	// put_zero (2);
	put_word (pollint);	// and this many time 64K usecs
	hci_cmd_end (44);
#if CC3000_DEBUG
	diag ("SEL");
#endif
}

static void close_socket () {

	hci_cmd_start (CC3000_HCICMD_CLO, 4);
	put_byteaslong (socket);
	hci_cmd_end (4);
#if CC3000_DEBUG
	diag ("CLO");
#endif
}

// ============================================================================

static void receive () {

	hci_cmd_start (CC3000_RCVCMND, 3*4);
	put_byteaslong (socket);
	put_wordaslong (maxplen);
	put_zero (4);
	hci_cmd_end (3*4);
#if CC3000_DEBUG
	diag ("REC");
#endif
}

// ============================================================================

#if WIFI_KAL_INTERVAL

static void send_kal () {

	word kp [2];

#if WIFI_OPTIONS & WIFI_OPTION_NETID

	kp [0] = statid;
	kp [1] = (word) host_id;
#else
	kp [0] = kp [1] = 0;
#endif

	send ((byte*)kp, 4);
}

#endif	/* KAL */

// ============================================================================
// ============================================================================

#define	EV_WAIT		0
#define EV_DONE		1

thread (cc3000_events)

    word argl, payl, len, off;

    entry (EV_WAIT)

#if CC3000_DEBUG > 2
	diag ("IRQ");
#endif

Redo:
	// Make it a loop to keep going until we are sure there are no more
	// events
	if (dstate == CC3000_STATE_DEAD) {
		// We shouldn't have come here, just make sure it won't happen
		// again
		cc3000_irq_disable;
		when (&cc3000_event_thread, EV_WAIT);
#if CC3000_DEBUG > 1
		diag ("ED!!!");
#endif
		release;
	}

	if (!cc3000_irq_pending) {
		// The IRQ line is down, so it is a false alarm
#if CC3000_DEBUG
		diag ("FA!!");
#endif
End:
		when (&cc3000_event_thread, EV_WAIT);
		cc3000_irq_clear;
		cc3000_irq_enable;
		release;
	}

	if ((len = spi_rea ()) == 0) {
		// This cannot happen, if IRQ is pending
#if CC3000_DEBUG > 0
		diag ("EE!");
#endif
		goto End;
	}

	if (len > buflen) {
		// Again, this should be impossible even with the minimum
		// buffer size
#if CC3000_DEBUG > 0
		diag ("OV!! %u", len);
#endif
		goto Done;
	}

	// We've got something to handle

#if CC3000_DEBUG > 2
	diag ("EV %u %u", len, buffer [0]);
#endif
	if (buffer [0] == CC3000_HCITYPE_EVENT) {

#if CC3000_DEBUG > 1
		diag ("HE %x", ((word)(buffer [2]) << 8) | buffer [1]);
#endif
#if CC3000_DEBUG > 2
		// Arguments
		argl = ((word)(buffer [4]) << 8) | buffer [3];
		for (off = 0; off < argl; off++)
			diag (" A: %x", buffer [5 + off]);
#endif
		// The opcode
		switch (((word)(buffer [2]) << 8) | buffer [1]) {

		    case CC3000_HCIEV_KEEPALIVE:

			keepalcnt++;
#if CC3000_DEBUG > 2
			diag ("KA %u", keepalcnt);
#endif
			goto Done;

		    // Here we have two connect events (WUCN is unsolicited),
		    // but we use UDHCP (IP address assignment) to mark network
		    // connection
		    // case CC3000_HCIEV_WCN:
		    // case CC3000_HCIEV_WUCN:

		    case CC3000_HCIEV_UDHCP:

			// WLAN connection
#if CC3000_DEBUG
			diag ("WC %u", dstate);
#endif
			if (dstate == CC3000_STATE_RCON ||
			    dstate == CC3000_STATE_POLI) {
				dstate = CC3000_STATE_APCN;
Trig:
				trigger (DEVENT);
			}
Done:
			proceed (EV_DONE);

		    case CC3000_HCIEV_WUDCN:

			// WLAN disconnection
#if CC3000_DEBUG
			diag ("WD %u", dstate);
#endif
			if (dstate >= CC3000_STATE_POLI) {
				// To incur short delay before reset
				dstate = CC3000_STATE_INIT;
				goto Trig;
			}

			// Ignore otherwise as being irrelevant
			goto Done;

		    case CC3000_HCIEV_SOK:

			// Socket created
#if CC3000_DEBUG
			diag ("SO %u %u", dstate, buffer [CC3000_HCIPO_SD]);
#endif
			if (dstate == CC3000_STATE_APCN) {
				socket = buffer [CC3000_HCIPO_SD];
				dstate = CC3000_STATE_SOKA;
				goto Trig;
			}

			goto Done;

// ============================================================================
#if WIFI_OPTIONS & WIFI_OPTION_TCP
// ============================================================================

		    case CC3000_HCIEV_CON:

			// Connected to server
#if CC3000_DEBUG
			diag ("SC %u", dstate);
#endif
			if (dstate == CC3000_STATE_SOKA) {
				dstate = CC3000_STATE_ESTB;
				goto Trig;
			}

			goto Done;

// ============================================================================
#endif	/* TCP */
// ============================================================================

		    case CC3000_HCIEV_SLS:

			// Simple link started
#if CC3000_DEBUG
			diag ("II %u", dstate);
#endif
			if (dstate == CC3000_STATE_INIT) {
				dstate = CC3000_STATE_WBIN;
				goto Trig;
			}

			goto Done;

		    case CC3000_HCIEV_RBS:

			// Buffer count obtained
#if CC3000_DEBUG
			diag ("BS %u %u %u", dstate, buffer [5],
				buffer [6] | ((word)(buffer [7]) << 8));
#endif
			if (dstate == CC3000_STATE_WBIN) {
				dstate = CC3000_STATE_POLI;
				freebuffers = nbuffers = buffer [5];
				// Buffer length (buffer [6-7]) ignored for now
				goto Trig;
			}

			goto Done;

		    case CC3000_HCIEV_SCP:

			// Connection policy set
#if CC3000_DEBUG
			diag ("SP %u", dstate);
#endif
			if (dstate == CC3000_STATE_POLI) {
				dstate = CC3000_STATE_RCON;
				goto Trig;
			}

			goto Done;

		    case CC3000_HCIEV_FREEBUF:

			// Free buffer

			argl = buffer [5];
			payl = 0;
			while (argl) {
				argl--;
				off = 5+2+2+argl+argl;
				if (off >= len) {
#if CC3000_DEBUG
					// Error, out of range
					diag ("FB rng: %u %u", len, off);
#endif
					continue;
				}
				payl += buffer [off];
			}
			if (payl + freebuffers > nbuffers) {
#if CC3000_DEBUG
				// Error, out of range
				diag ("FB big: %u %u", payl, freebuffers);
#endif
				goto Done;
			}

			freebuffers += (byte) payl;
#if CC3000_DEBUG
			diag ("FB %u %u", dstate, freebuffers);
#endif
			// goto Done;

		   case CC3000_EVNTSND:

#if CC3000_DEBUG
			diag ("TX %u", dstate);
#endif
			if (dstate == CC3000_STATE_SENT) {
				dstate = CC3000_STATE_ESTB;
				goto Trig;
			}

			goto Done;

		    case CC3000_HCIEV_RCV:

#if CC3000_DEBUG
			diag ("RC %u", dstate);
#endif
			// Should we handle this (e.g., to detect zero-length
			// packets)
			goto Done;

// ============================================================================
#if WIFI_OPTIONS & WIFI_OPTION_TCP
// ============================================================================
		    case CC3000_HCIEV_CLOSEWAIT:

			// Sever-side disconnect
#if CC3000_DEBUG
			diag ("CL %u", dstate);
#endif
			if (dstate >= CC3000_STATE_ESTB) {
				// Force OFF, this is for now, we will have to
				// provide some (friendlier) operations for
				// reconnection
				dstate = CC3000_STATE_DEAD;
				goto Trig;
			}

			goto Done;
// ============================================================================
#endif	/* TCP */
// ============================================================================

		    case CC3000_HCIEV_SEL:

			// Select done
#if CC3000_DEBUG > 1
			diag ("SE %u", dstate);
#endif
			if (dstate == CC3000_STATE_RECV) {
#if WIFI_KAL_INTERVAL
				kalcnt += pollint;
#endif
				if (buffer [CC3000_HCIPO_SELRD])
					// Wake up and receive
					dstate = CC3000_STATE_READ;
				else
					// Try to send
					dstate = CC3000_STATE_ESTB;

				goto Trig;
			}
			goto Done;

// ============================================================================
#if WIFI_OPTIONS & WIFI_OPTION_TCP
// ============================================================================

		    case CC3000_HCIEV_CLO:

			// Closed socket
#if CC3000_DEBUG
			diag ("CS %u", dstate);
#endif
			if (dstate == CC3000_STATE_CSNG) {
				dstate = CC3000_STATE_DEAD;
				goto Trig;
			}

			goto Done;

// ============================================================================
#endif	/* TCP */
// ============================================================================

#if CC3000_DEBUG > 0
		    default:

			diag ("UN %x", ((word)(buffer [2]) << 8) | buffer [1]);
#endif
		}
		// ============================================================
		// Done with events
		// ============================================================
		// goto Done;

	} else if (buffer [0] == CC3000_HCITYPE_DATA) {

		// Apparently not needed
		// if (buffer [1] == CC3000_DATA_RCV) {

		// Packet reception
		argl = buffer [2];
		payl = buffer [4];
		payl = (payl << 8) + buffer [3];
		if (payl > 5 + buflen || argl > payl) {
#if CC3000_DEBUG
			// Consistency check
			diag ("BAD DATA %1d %1d", payl, argl);
#endif
			goto Done;
		}
		// Packet length
		if ((payl -= argl) == 0)
			// Ignore zero length packets?
			goto Done;

		argl += 5;
#if CC3000_DEBUG > 1
		diag ("RHDR: %u %u", argl, payl);
#endif
#if WIFI_OPTIONS & WIFI_OPTION_NETID
		if (payl >= 4 && (statid == 0 || statid == 0xffff ||
		    ((address)(buffer + argl)) [0] == statid))
			// Only receive if statid agreeable
#endif
			tcvphy_rcv (physid, (address)(buffer + argl), payl);

		if (dstate == CC3000_STATE_READ) {
			dstate = CC3000_STATE_ESTB;
			goto Trig;
		}

		// Closing the unneeded if
		// }
	}

    entry (EV_DONE)

	delay (CC3000_DELAY_EVENT, EV_WAIT);

endthread

// ============================================================================
// ============================================================================

#define	DR_LOOP		0
#define	DR_FCLOSE	1

thread (cc3000_driver)

    word len;
    address buf;

    entry (DR_LOOP)

Loop:
	cc3000_irq_disable;

#if CC3000_DEBUG > 1
	diag ("Lo %u", dstate);
#endif
	if (dstate == CC3000_STATE_DEAD) {
		// Switched off
		if (!(flags & CC3000_FLAG_OFF)) {
			// We go on
			dstate = CC3000_STATE_INIT;
#if CC3000_DEBUG
			diag ("DS!");
#endif
			deadstart ();
			goto Loop;
		}
		cc3000_disable;
		when (DEVENT, DR_LOOP);
		release;
	}

	if (flags & CC3000_FLAG_OFF) {
		// Switching off
		if (dstate >= CC3000_STATE_CSNG) {
			// Connected to server, need to close socket
			if (dstate == CC3000_STATE_ESTB) {
				dstate = CC3000_STATE_CSNG;
				close_socket ();
			}
STimer:
			delay (CC3000_TIMEOUT_SHORT, DR_FCLOSE);
WState:
			when (DEVENT, DR_LOOP);
			cc3000_irq_enable;
			release;
		}
FClose:
		dstate = CC3000_STATE_DEAD;
		// Just turn it off
		goto Loop;
	}

	switch (dstate) {

		case CC3000_STATE_WBIN:

			// Ask for buffers
			// read_version ();
			get_buffer_count ();

		case CC3000_STATE_INIT:
		case CC3000_STATE_RECV:
		case CC3000_STATE_SENT:

			// In these states we just wait for something to
			// happen

			goto STimer;

		case CC3000_STATE_POLI:

			if (wparams->policy < CC3000_POLICY_OLD) {
				// We have to change the policy
				set_connect_policy (wparams->policy);
				goto STimer;
			}

			// Fall through
			dstate = CC3000_STATE_RCON;

		case CC3000_STATE_RCON:

			// Connect
			if (wparams->policy != CC3000_POLICY_DONT)
				connect_to_ap ();
			// I guess this is supposed to happen automatically
			// while you wait
CTimer:
			delay (CC3000_TIMEOUT_LONG, DR_FCLOSE);
			goto WState;

		case CC3000_STATE_APCN:

			// Connected to AP, now connect to the socket
			create_socket ();
			goto STimer;

		case CC3000_STATE_SOKA:

			if (socket == BNONE)
				// This is an error, probably caused by a
				// connection failure
				goto FClose;

#if WIFI_OPTIONS & WIFI_OPTION_TCP

			// Connect to the client
			connect_to_server ();
			// FIXME: detect connection refused; CTIMER is for a
			// hard errors (no response)
			goto CTimer;
#else
			dstate = CC3000_STATE_ESTB;
			// Fall through
#endif
		case CC3000_STATE_ESTB:

			if (freebuffers) {
				if ((buf = tcvphy_get (physid, &len))) {
					// regular packet to send
#if WIFI_OPTIONS & WIFI_OPTION_NETID
					sysassert (len >= 4, "cc30 pl");
					if (statid != 0xffff)
						buf [0] = statid;
#endif
					send ((byte*)buf, len);
					tcvphy_end (buf);
					dstate = CC3000_STATE_SENT;
#if WIFI_KAL_INTERVAL
					kalcnt = 0;
#endif
					goto DecInt;
				}
#if WIFI_KAL_INTERVAL
				if ((kalcnt >= WIFI_KAL_INTERVAL) &&
				    freebuffers == nbuffers) {
					send_kal ();
					dstate = CC3000_STATE_SENT;
					kalcnt = 0;
					goto STimer;
				}
#endif
			}
					
			dstate = CC3000_STATE_RECV;
			simple_select ();
			// Increase poll interval
			if (pollint < CC3000_POLLINT_MAX)
				pollint++;
			goto STimer;

		case CC3000_STATE_READ:

			receive ();
DecInt:
			// Decrease poll interval
			pollint = CC3000_POLLINT_MIN;
			goto STimer;
	}

	goto WState;

    entry (DR_FCLOSE)

#if CC3000_DEBUG
	diag ("TO!!!");
#endif
	goto FClose;

endthread

// ============================================================================

static int option (int opt, address val) {

	int ret = 0;

	switch (opt) {

		case PHYSOPT_STATUS:

			ret = 2 | ((flags & CC3000_FLAG_OFF) != 0);
RVal:
			if (val != NULL) {
				((cc3000_phy_status_t*)val) -> dstate = dstate;
				((cc3000_phy_status_t*)val) -> freebuffers =
					freebuffers;
				((cc3000_phy_status_t*)val) -> mkalcnt =
					keepalcnt;
#if WIFI_KAL_INTERVAL
				((cc3000_phy_status_t*)val) -> dkalcnt = kalcnt;
#endif
			}
RRet:
			return ret;

		case PHYSOPT_RXON:

			if ((flags & CC3000_FLAG_OFF)) {
#if CC3000_DEBUG
				diag ("RXON");
#endif
				_BIC (flags, CC3000_FLAG_OFF);
				if (dstate == CC3000_STATE_DEAD)
					trigger (DEVENT);
			}

			goto RRet;

		case PHYSOPT_RXOFF:

			if ((flags & CC3000_FLAG_OFF) == 0) {
#if CC3000_DEBUG
				diag ("RXOFF");
#endif
				_BIS (flags, CC3000_FLAG_OFF);
				// No trigger, don't mess with solicited
				// events
			}

			goto RRet;

		case PHYSOPT_GETMAXPL:

			ret = maxplen;
			goto RVal;

#if WIFI_OPTIONS & WIFI_OPTION_NETID

		case PHYSOPT_SETSID:

			statid = (val == NULL) ? 0 : *val;
			goto RRet;

		case PHYSOPT_GETSID:

			ret = (int) statid;
			goto RVal;
#endif

	}

	syserror (EREQPAR, "cc30");
}

void phys_cc3000 (int phy, int mbs, cc3000_server_params_t *sp,
	cc3000_wlan_params_t *wp) {

	// Make these things conditional later to save code
	if (buffer)
		syserror (ETOOMANY, "cc30");

	maxplen = (mbs == 0) ? CC3000_DEFPLEN : mbs;
	buflen = maxplen + CC3000_BHDRLEN;

	if ((buffer = (byte*)umalloc (buflen)) == NULL)
		syserror (EMALLOC, "cc30");

	physid = phy;
	flags = CC3000_FLAG_OFF;

	sparams = sp;
	wparams = wp;

	tcvphy_reg (physid = phy, option, INFO_PHYS_CC3000);

	// Start the driver threads
	if (runthread (cc3000_driver) == 0 || (cc3000_event_thread =
	    runthread (cc3000_events)) == 0)
		syserror (ERESOURCE, "cc30");
}


// To do:
//
//	- closing properly; what to do about the XMIT quueue!
