/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#ifndef __SMURPH__
#include "cc1100.h"
#endif
#include "phys_cc1100.h"
#include "plug_null.h"
#include "hold.h"

#include "form.h"

#if UART_TCV
// UART present
#include "phys_uart.h"
#endif

#ifdef	CHRONOS
#include "ez430_lcd.h"
#include "rtc_cc430.h"
#endif

#ifdef TLDEBUG
#include "tldebug.h"
#endif

#include "app_data.h"

#if	NUMBER_OF_SENSORS > 0
#include "sensors.h"
#endif

#define	MAX_PACKET_LENGTH	CC1100_MAXPLEN

heapmem { 50, 50 };

// ============================================================================

lword	g_lrs;

#ifndef CHRONOS
int	g_fd_uart = -1;
#endif

int	g_fd_rf = -1, g_snd_opl,
	g_pkt_minpl = MIN_MES_PACKET_LENGTH,
	g_pkt_maxpl = MAX_PACKET_LENGTH;

word	g_pkt_mindel = 1024, g_pkt_maxdel = 1024,
	g_snd_count, g_snd_rnode, g_snd_rcode, g_chsec, g_pcmd_del, g_snd_left,
	g_snd_sernum = 1, g_snd_rtries, g_flags = 0;

byte	g_last_rssi, g_last_qual, g_snd_urgent;

word	g_pat_peer, g_pat_cnt, g_pat_cntr;
lword	g_pat_acc;
byte	g_pat_cset [] = { 0x3e, 0, 255 }, g_pat_cred;

word	g_pat_nlqi, g_pat_maxlqi, g_pat_drop;

char	*g_snd_rcmd, *g_pcmd_cmd;

#ifndef	__SMURPH__
#ifdef	CC1100_OLD_DRIVER
byte	*g_reg_suppl;
#else
const byte g_patable [] = CC1100_PATABLE;
#endif
#endif

address	g_rcv_ackrp;

noded_t	g_rep_nodes [MAX_NODES];

// ============================================================================

#ifdef CHRONOS
// ============================================================================
// CHRONOS interface ==========================================================
// ============================================================================

static char c_msg [6];

static void msg_lcd (const char *txt, word fr, word to) {
//
// Displays characters on the LCD
//
	char c;

	while (1) {
		if ((c = *txt) != '\0') {
			if (c >= 'a' && c <= 'z')
				c -= ('a' - 'A');
			ezlcd_item (fr, (word)c | LCD_MODE_SET);
			txt++;
		} else {
			ezlcd_item (fr, LCD_MODE_CLEAR);
		}
		if (fr == to)
			return;
		if (fr > to)
			fr--;
		else
			fr++;
	}
}

static void msg_hi (const char *txt) {
	msg_lcd (txt, LCD_SEG_L1_3, LCD_SEG_L1_0);
}

void msg_lo (const char *txt) {
	msg_lcd (txt, LCD_SEG_L2_4, LCD_SEG_L2_0);
}

static char h_code (v) {

	v &= 0xF;
	return v < 10 ? '0' + v : ('a' + (v - 10));
}

static void err_msg (word st, word v) {

	if (v != 0 && v != WNONE) {
		c_msg [0] = 'E';
		c_msg [1] = 'R';
		c_msg [2] = v > 11 ? 'u' : h_code (v);
		c_msg [3] = '\0';
		msg_hi (c_msg);
	}
}

static void enc_dec (word n, word s) {

	char *m;

	m = c_msg + 4;
	do {
		*m-- = (char) ('0' + (n % 10));
		s--;
		n /= 10;
	} while (s);
}

static void enc_hex (word n, word s) {

	char *m;

	s--;
	m = c_msg + s;

	while (1) {
		*m = h_code (n);
		if (s == 0)
			break;
		m--;
		s--;
		n >>= 4;
	}
}

static byte m_busy = 0;

fsm thread_m_busy {

	state MB_START:

		delay (512, MB_CLEAR);
		release;

	state MB_CLEAR:

		m_busy = 0;
		trigger (&m_busy);
		finish;
}

static void m_out (word st, const char *t) {

	if (m_busy) {
		if (st == WNONE)
			return;
		when (&m_busy, st);
		release;
	}

	msg_lo (t);

	if (runfsm thread_m_busy)
		m_busy = 1;
}

// ============================================================================
// UART interface =============================================================
// ============================================================================

#else

static char *emess (word estat) {

	switch (estat) {

		case 0:
		case WNONE:	return NULL;

		case 1:	return (char*)"Too much data"; break;
		case 2:	return (char*)"Cannot alloc packet"; break;
		case 3:	return (char*)"Busy with previous"; break;
		case 4:	return (char*)"Format error"; break;
		case 5:	return (char*)"No memory"; break;
		case 6:	return (char*)"Too many threads"; break;
		case 7:	return (char*)"No such command"; break;
		case 8: return (char*)"Already in progress"; break;
		case 9: return (char*)"Command must be remote"; break;
		case 10: return (char*)"Illegal arguments"; break;
		case 11: return (char*)"Illegal remotely"; break;

		default:
			return (char*)"Unknown error";
	}
}

#define	err_msg(st,v)	do { \
				char *msg = emess (v); \
				if (msg != NULL) \
					uart_out (st, msg); \
			} while (0)

void uart_outf (word st, const char *fm, ...) {
//
// Formatted output over L-mode UART
//
	va_list ap;
	address packet;
	word ln;

	va_start (ap, fm);

	if ((ln = vfsize (fm, ap)) > UART_LINE_LENGTH - 1)
		syserror (EREQPAR, "uart_outf");

	packet = tcv_wnp (st, g_fd_uart, ln + 1);
	if (packet == NULL)
		return;

	vform ((char*)packet, fm, ap);

	tcv_endp (packet);
}

void uart_out (word st, const char *ms) {
//
// Unformatted output over L-mode UART
//
	address packet;
	word ln;

	if ((ln = strlen (ms)) > UART_LINE_LENGTH - 1)
		syserror (EREQPAR, "uart_out");

	packet = tcv_wnp (st, g_fd_uart, ln + 1);
	if (packet == NULL)
		return;
	memcpy (packet, ms, ln + 1);
	tcv_endp (packet);
}

#endif /* CHRONOS or UART */

// ============================================================================

static const char *skip_blk (const char *cb) {
//
// Skip blanks
//
	while (*cb == ' ' || *cb == '\t' || *cb == ',') cb++;
	return cb;
}

static const char *skip_del (const char *cb) {
//
// Skip through a delimiter
//
	while (*cb != ' ' && *cb != '\t' && *cb != '\0' && *cb != ',') cb++;
	return skip_blk (cb);
}
	
void confirm (word sender, word sernum, word code) {
//
// Send an ACK
//
	address packet;

	if ((packet = tcv_wnp (WNONE, g_fd_rf, MIN_ACK_LENGTH)) != NULL) {
		packet [POFF_RCV] = HOST_ID;
		packet [POFF_SND] = sender;
		packet [POFF_SER] = sernum;
		packet [POFF_FLG] = 0;		// This is not used by ACK
		packet [POFF_CMD] = code;
		tcv_endp (packet);
	}
}

void handle_ack (address buf, word pl) {
//
// Handles a received ACK; note that we are passed a packet buffer, which we
// are expected to deallocate
//
	// The length is in words
	if (pl > MIN_ACK_LENGTH/2) {
		// This looks like a report
		if (g_rcv_ackrp) {
			// Still doing the previous one
			tcv_endp (buf);
#ifdef CHRONOS
			m_out (WNONE, "AIRS");
#else
			uart_out (WNONE, "Incoming report skipped");
#endif
			return;
		}

		if (pl < POFF_NTAB + 1) {
			// At least this much needed to make sense
Bad_length:
			tcv_endp (buf);
#ifdef CHRONOS
			m_out (WNONE, "ABAD");
#else
			uart_out (WNONE, "Bad report");
#endif
			return;
		}

		if (pl < buf [POFF_NNOD] * 2 + POFF_NTAB + 1)
			goto Bad_length;


		if (runfsm thread_rreporter == 0) {
			tcv_endp (buf);
#ifdef CHRONOS
			m_out (WNONE, "ACAF");
#else
			uart_out (WNONE, "Can't fork");
#endif
			return;
		}

		g_rcv_ackrp = buf;
		g_snd_rcode = 0;
		goto End;
	}

	g_snd_rcode = buf [POFF_CMD];
	tcv_endp (buf);
End:
	g_snd_rtries = WNONE;
	trigger (&g_snd_rnode);
}

word do_command (const char *cb, word sender, word sernum) {
//
// Handle a local (sender == 0) or remote command arriving as a character
// string
//
	switch (*cb) {

	    case 'r': {

		// Report
		word clear;

		clear = 0;
		scan (cb + 1, "%u", &clear);

		if (sender) {

			// Create a report packet and send it once

			int ps;
			word i, rs;
			address packet;

			// Calculate packet length (the last word is for CHS)
			ps = (POFF_NTAB + (rs = report_size ()) * 2 + 1) * 2;

			if (ps > MAX_PACKET_LENGTH) {
#ifdef CHRONOS
				m_out (WNONE, "ERTL");
#else
				uart_out (WNONE, "Report too large");
#endif
				return 1;
			}

			packet = tcv_wnp (WNONE, g_fd_rf, ps);

			if (packet == NULL)
				return 2;	// Busy

			// Fill in
			packet [POFF_RCV] = HOST_ID;
			packet [POFF_SND] = sender;
			packet [POFF_SER] = sernum;

			packet [POFF_SENT] = g_snd_count;
			tcv_control (g_fd_rf, PHYSOPT_ERROR, packet+POFF_RERR);
			if (clear)
				tcv_control (g_fd_rf, PHYSOPT_ERROR, NULL);

			packet [POFF_NNOD] = rs;

			ps = POFF_NTAB;

			for (i = 0; i < rs; i++) {
				packet [ps++] = g_rep_nodes [i] . Node;
				packet [ps++] = g_rep_nodes [i] . Count;
			}

			tcv_endp (packet);
			// This means: OK, we have responded
			return WNONE;
		}

		// Local report via UART

		if (!running (thread_ureporter)) {
			runfsm thread_ureporter (clear);
			return 0;
		}

		return 3;	// Busy
	    }

	    case 'S': {

		// Command to a remote node

		word sl;

		if (g_snd_rcmd != NULL || g_snd_rnode != 0)
			// Busy
			return 3;

		cb = skip_blk (++cb);
		// This must be a digit
		if (*cb < '0' || *cb > '9')
			return 4;

		// The node (note that g_snd_rnode == 0 at this moment)
		scan (cb, "%u", &g_snd_rnode);
		if (g_snd_rnode == 0)
			return 4;	// Format error

		// Skip the number
		cb = skip_del (cb);

		if ((sl = strlen (cb)) == 0) {
			g_snd_rnode = 0;
			return 4;	// Empty command
		}

		if (sl > MAX_RF_CMD_LENGTH) {
			g_snd_rnode = 0;
			return 1;
		}

		if ((g_snd_rcmd = (char*) umalloc (sl + 1)) == NULL) {
			g_snd_rnode = 0;
			return 5;	// No memory
		}

		strcpy (g_snd_rcmd, cb);

		if (runfsm thread_rfcmd == 0) {
			ufree (g_snd_rcmd);
			g_snd_rcmd = NULL;
			g_snd_rnode = 0;
			return 6;	// No more threads
		}
		return 0;
	    }

	    case 'l': {

		// Packet parameters

		word mide, made, mipl, mapl;

		// Assume previous values
		mide = g_pkt_mindel;
		made = g_pkt_maxdel;
		mipl = (word) g_pkt_minpl;
		mapl = (word) g_pkt_maxpl;

		scan (cb + 1, "%u %u %u %u", &mide, &made, &mipl, &mapl);

		if (mide < MIN_SEND_INTERVAL)
			mide = MIN_SEND_INTERVAL;

		if (made < mide)
			made = mide;

		if (mipl < 8)
			mipl = 8;

		if (mapl < mipl)
			mapl = mipl;

		if (mapl > MAX_PACKET_LENGTH)
			mapl = MAX_PACKET_LENGTH;

		g_pkt_mindel = mide;
		g_pkt_maxdel = made;
		g_pkt_minpl = (int) mipl;
		g_pkt_maxpl = (int) mapl;

		return 0;
	    }

	    case 's': {

		// Start/restart the sender
		word np, ur, to, dl;

		// Default = no limit, non urgent
		np = ur = to = dl = 0;

		scan (cb + 1, "%u %u %u %u", &np, &ur, &to, &dl);

		g_snd_left = np;
		g_snd_urgent = (ur != 0);

		killall (thread_sender);

		if (to != 0) {
			killall (thread_rxbackon);
			tcv_control (g_fd_rf, PHYSOPT_RXOFF, NULL);
			if  (runfsm thread_rxbackon (to) == 0)
				return 6;
		}

		if (runfsm thread_sender (dl) == 0) {
			return 6;
		}

		return 0;
	    }

	    case 'f': {

		word wormode;

		if (sender)
			return 11;

		wormode = 0;

		scan (cb + 1, "%u", &wormode);

		killall (thread_rxbackon);
		tcv_control (g_fd_rf, PHYSOPT_RXOFF, &wormode);
		return 0;
	    }

	    case 'g':

		killall (thread_rxbackon);
		tcv_control (g_fd_rf, PHYSOPT_RXON, NULL);
		return 0;

	    case 'p': {

		// Set power level
	
		word plev;

		plev = WNONE;
		scan (cb + 1, "%u", &plev);

		if (plev > 7)
			return 4;

		tcv_control (g_fd_rf, PHYSOPT_SETPOWER, &plev);
		return 0;
	    }

	    case 'q':

		// Stop transmitting
		killall (thread_sender);
		// To terminate the PATABLE thread, if running
		g_pat_cnt = 0;
		trigger (&g_pat_cred);
		return 0;

	    case 'd':

		// Power down mode
		powerdown ();
		g_flags |= 0x8000;
		return 0;

	    case 'u':

		// Power up mode
		powerup ();
		g_flags &= ~0x8000;
		return 0;

	    case 'c':

		// Clear counters
		reset_count ();
		return 0;

	    case 'v':
		g_flags |= 0x4000;
		return 0;

	    case 'n':

		g_flags &= ~0x4000;
		return 0;

#ifndef __SMURPH__

	    case 't':

		// PATABLE test
		if (running (thread_patable))
			// One at a time
			return 8;

		g_pat_cnt = 16;
		scan (cb + 1, "%u %u", &g_pat_peer, &g_pat_cnt);

		if (g_pat_cnt == 0)
			g_pat_cnt = 1;

		if (runfsm thread_patable == 0)
			return 6;
		return 0;

#if !defined(CC1100_OLD_DRIVER) && RADIO_WOR_MODE

	    case 'w': {

		// Set WOR parameters
		word n, v [7];

		v [0] = RADIO_WOR_PREAMBLE_TIME / 1024;
		v [1] = RADIO_WOR_IDLE_TIMEOUT / 1024;
		v [2] = WOR_EVT0_TIME >> 8;
		v [3] = WOR_RX_TIME;
		v [4] = WOR_PQ_THR;
		v [5] = WOR_RSSI_THR;
		v [6] = WOR_EVT1_TIME;

		scan (cb + 1, "%u %u %u %u %u %u %u",
			v + 0, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6);

		for (n = 0; n < 7; n++)
			((byte*)v) [n] = (byte) v [n];

		tcv_control (g_fd_rf, PHYSOPT_SETPARAMS, v);

		return 0;
	    }

#endif
	    case 'm': {

		// Modify CC1100 register

		word n, v;
		const char *cq;
		byte *rs;

		// Count the values
		n = 0;
		cq = cb + 1;

		while (1) {
			cq = skip_blk (cq);
			if (*cq == '\0')
				break;
			n++;
			cq = skip_del (cq);
		}

		if (n == 0) {
			// This means remove previous table; note with the new
			// driver, the command works differently, i.e., the
			// registers are modified in place and there is no
			// supplementary table, so the command without
			// arguments resets the chip
#ifdef CC1100_OLD_DRIVER
			if (g_reg_suppl) {
				rs = NULL;
CDiff:
				// ufree (NULL) is legal
				ufree (g_reg_suppl);
				g_reg_suppl = rs;
				tcv_control (g_fd_rf, PHYSOPT_RESET,
					(address)g_reg_suppl);
			}
#else
			// Do reset
			tcv_control (g_fd_rf, PHYSOPT_RESET, NULL);
#endif
		} else {
			if (n & 1)
				return 4;
			if ((rs = (byte*) umalloc (n + 1)) == NULL)
				return 5;
			// Collect the numbers
			cq = cb + 1;
			n = 0;
			while (1) {
				cq = skip_blk (cq);
				if (*cq == '\0')
					break;
				v = WNONE;
				scan (cq, "%x", &v);
				if (v > 0xff) {
					ufree (rs);
					return 4;
				}
				rs [n++] = (byte) v;
				cq = skip_del (cq);
			}

			rs [n] = 0xff;
#ifdef CC1100_OLD_DRIVER
			// Check if this is a different table
			if (!g_reg_suppl)
				// Different, the previous one was empty
				goto CDiff;

			n = 0;
			while (1) {
				if (g_reg_suppl [n] != rs [n])
					// Different
					goto CDiff;
				n++;
				if (g_reg_suppl [n] != rs [n])
					goto CDiff;
				if (rs [n] == 0xff)
					// The end
					break;
				n++;
			}
			// Same
			ufree (rs);
#else
			tcv_control (g_fd_rf, PHYSOPT_RESET, (address) rs);
			ufree (rs);
#endif
		}
		// Do nothing
		return 0;
	    }

#endif	/* __SMURPH__ */

	    case 'C': {

		// Switch channel for the prescribed number of seconds
		word ch, sc;

		ch = WNONE;
		sc = 10;

		scan (cb + 1, "%u %u", &ch, &sc);

		if (ch > 255) 
			return 10;

		if (ch == 0 || !sender) {
			// Ground channel or local command
			killall (thread_chguard);
		} else {
			if (!running (thread_chguard))
				if (runfsm thread_chguard == 0)
					return 6;
			g_chsec = sc;
		}

		tcv_control (g_fd_rf, PHYSOPT_SETCHANNEL, &ch);

		// Never confirm
		return WNONE;
	    }

	    case 'P': {

		// Issue a command periodically

		word sl;

		if (running (thread_pcmnder))
			// Busy
			return 3;

		ufree (g_pcmd_cmd);
		g_pcmd_cmd = NULL;

		cb = skip_blk (++cb);
		if (*cb < '0' || *cb > '9')
			return 4;

		g_pcmd_del = 10;
		scan (cb, "%u", &g_pcmd_del);
		if (g_pcmd_del == 0)
			g_pcmd_del = 1;

		cb = skip_del (cb);

		if ((sl = strlen (cb)) == 0)
			return 4;

		if ((g_pcmd_cmd = (char*) umalloc (sl + 1)) == NULL)
			return 5;

		strcpy (g_pcmd_cmd, cb);

		if (runfsm thread_pcmnder == 0) {
			ufree (g_pcmd_cmd);
			g_pcmd_cmd = NULL;
			return 6;
		}
		return 0;
	    }

	    case 'Q': {

		killall (thread_pcmnder);
		ufree (g_pcmd_cmd);
		g_pcmd_cmd = NULL;
		return 0;
	    }
			
#ifndef __SMURPH__
	    // Two special commands used by PATABLE tester

	    case '+': {

		address packet;

		// Respond with RSSI reading

		if (sender == 0)
			// Must be remote
			return 9;

		// Command size (fixed)
		packet = tcv_wnp (WNONE, g_fd_rf, (POFF_CMD + 4 + 1) * 2);
		if (packet == NULL)
			// Quietly ignore
			return WNONE;

		packet [POFF_RCV] = sender;
		packet [POFF_SND] = HOST_ID;
		packet [POFF_SER] = sernum;	// identifies the setting
		form ((char*)&(packet [POFF_CMD]), "- %x",
		       ((word)g_last_rssi << 8) + g_last_qual);
		tcv_endp (packet);

		return WNONE;
	    }

	    case '-': {

		sint pid;
		word val;

		if ((pid = running (thread_patable)) == 0)
			// No PATABLE thread
			return WNONE;

		if ((byte)sernum != g_pat_cset [1])
			// Obsolete reading
			return WNONE;

		val = WNONE;
		cb = skip_blk (++cb);
		scan (cb, "%x", &val);
#if 0
		if (val > 255)
			// Illegal
			return WNONE;
#endif
		g_pat_cred = (byte) (val >> 8);

		if ((val &= 0x00ff) != 0) {
			g_pat_nlqi++;
			if (g_pat_maxlqi < val)
				g_pat_maxlqi = val;
		}

		ptrigger (pid, &g_pat_cred);
		return WNONE;
	    }

#endif /* __SMURPH__ */

#ifndef	CHRONOS

	    case 'z': {
		// Misc commands for debugging and stuff
		cb++;

		switch (*cb) {

		    case 't' : {
			// Time the CPU
			lword sec;
			word arg, cnt, i;
			Boolean u;
			cb++;
			if (*cb == 'u') {
				u = YES;
			} else if (*cb == 'm') {
				u = NO;
			} else {
				return 4;
			}
			arg = 1;
			cnt = 1;
			scan (cb + 1, "%u %u", &arg, &cnt);
			sec = seconds ();
			i = cnt;
			if (u) {
				while (i--)
					udelay (arg);
			} else {
				while (i--)
					mdelay (arg);
			}
			sec = seconds () - sec;
			uart_outf (WNONE, "ZT%c: %u, %u -> %u",
				u ? 'U' : 'M',
				arg, cnt, (word)sec);
			return WNONE;

		    }

		    case 'r': {
			// Memory read
			Boolean b;
			volatile word a, c;
			cb++;
			b = YES;
			if (*cb == 'w') {
				b = NO;
				cb++;
			} else if (*cb == 'b') {
				cb++;
			}
			// Read the address in hex
			a = 0;
			scan (cb + 1, "%x", &a);
			if (a == 0)
				return 10;
			if (b)
				c = *((byte*)a);
			else
				c = *((word*)a);
			uart_outf (WNONE, "ZR: %c(%x) => %x <%u>",
				b ? 'B' : 'W',
				a, c, c);
			return WNONE;
		    }

		    case 'w' : {
			// Memory write
			Boolean b;
			volatile word a, c;
			cb++;
			b = YES;
			if (*cb == 'w') {
				b = NO;
				cb++;
			} else if (*cb == 'b') {
				cb++;
			}
			// Read the address and content in hex
			a = c = 0;
			scan (cb + 1, "%x %x", &a, &c);
			if (a == 0)
				return 10;
			if (b)
				*((byte*)a) = (byte)c;
			else
				*((word*)a) = c;
			uart_outf (WNONE, "ZW: %x <%u> => %c(%x)",
				c, c,
				b ? 'B' : 'W',
				a);
			return WNONE;
		    }
		}
	    }
#endif /* ndef CHRONOS */
	}
	return 7;
}

// ============================================================================

word gen_send_params () {

	if (g_pkt_minpl >= g_pkt_maxpl)
		g_snd_opl = g_pkt_minpl;
	else
		g_snd_opl = ((word)(rnd () % (g_pkt_maxpl - g_pkt_minpl + 1)) +
			g_pkt_minpl);

	if ((g_snd_opl &= 0xfe) < MIN_MES_PACKET_LENGTH)
		g_snd_opl = MIN_MES_PACKET_LENGTH;

	if (g_pkt_mindel >= g_pkt_maxdel)
		return g_pkt_mindel;

	if (g_pkt_mindel == g_pkt_maxdel)
		return g_pkt_maxdel;

	return (word)(rnd () % (g_pkt_maxdel - g_pkt_mindel + 1)) +
		g_pkt_mindel;
}

void update_count (word node) {

	word i;

	for (i = 0; i < MAX_NODES; i++) {
		if (g_rep_nodes [i] . Node == node) {
			g_rep_nodes [i] . Count ++;
			return;
		}
		if (g_rep_nodes [i] . Node == 0) {
			g_rep_nodes [i] . Node = node;
			g_rep_nodes [i] . Count = 1;
			return;
		}
	}
	// Ignore if overflowed
}

void reset_count () {

	memset (g_rep_nodes,  0, sizeof (g_rep_nodes));
	g_snd_count = 0;
	tcv_control (g_fd_rf, PHYSOPT_ERROR, NULL);
}

word report_size () {

	word i;

	for (i = 0; i < MAX_NODES; i++)
		if (g_rep_nodes [i] . Node == 0)
			break;

	return i;
}

void view_packet (address p, word pl) {
//
// View (nonaggressively, i.e., never block) a measurement packet
//
	address packet;
	word ns;

	if (pl <= POFF_SEN + 1)
		ns = 0;
	else
		ns = pl - POFF_SEN - 1;

	if (ns < NUMBER_OF_SENSORS)
		// Something wrong
		return;

#ifdef CHRONOS

	c_msg [0] = 'R';
	enc_dec (p [POFF_SER], 4);
	m_out (WNONE, c_msg);

#else
	uart_outf (WNONE, "S:%u, D:%x, N:%u, L:%u, P:%u, R:%u, Q:%u, M%c"
#if NUMBER_OF_SENSORS > 0
		", V:%u"
#if NUMBER_OF_SENSORS > 1
		",%u"
#if NUMBER_OF_SENSORS > 2
		",%u"
#if NUMBER_OF_SENSORS > 3
		",%u"
#if NUMBER_OF_SENSORS > 4
		",%u"
#endif
#endif
#endif
#endif
#endif
		, p [POFF_SND], p [POFF_DRI], p [POFF_SER],
		  	pl << 1, p [POFF_FLG] & 0x07,
			g_last_rssi, g_last_qual,
			(p [POFF_FLG] & 0x8000) ? 'D' : 'U'

#if NUMBER_OF_SENSORS > 0
		, p [POFF_SEN + 0]
#if NUMBER_OF_SENSORS > 1
		, p [POFF_SEN + 1]
#if NUMBER_OF_SENSORS > 2
		, p [POFF_SEN + 2]
#if NUMBER_OF_SENSORS > 3
		, p [POFF_SEN + 3]
#if NUMBER_OF_SENSORS > 4
		, p [POFF_SEN + 4]
#endif
#endif
#endif
#endif
#endif
		);
#endif	/* ndef CHRONOS */

}

// ============================================================================

fsm thread_rxbackon (word offtime) {
//
// Turn RX on after a delay
//
	entry RF_START:

		delay (offtime, RF_BACKON);
		release;

	entry RF_BACKON:

		tcv_control (g_fd_rf, PHYSOPT_RXON, NULL);
		finish;
}

fsm thread_pcmnder {

  lword until;
  word estat;

  entry PC_START:

	until = seconds () + g_pcmd_del;

  entry PC_WAIT:

	hold (PC_WAIT, until);

  entry PC_NEXT:

#ifdef CHRONOS
	c_msg [0] = 'C';
	enc_dec ((word) seconds (), 4);
	m_out (PC_NEXT, c_msg);
#else

	uart_outf (PC_NEXT, "CMD: [%lu] %s", seconds (), g_pcmd_cmd);
#endif
	estat = do_command (g_pcmd_cmd, 0, 0);

  entry PC_MSG:

	err_msg (PC_MSG, estat);
	proceed PC_START;
}

// ============================================================================

fsm thread_chguard {

  entry CG_START:

	if (seconds () > g_lrs + g_chsec)
		reset ();

	delay (5 * 1024, CG_START);
}

// ============================================================================

#ifndef	__SMURPH__

fsm thread_patable {

  word ntries, spow;

  entry PA_START:

	// Preserve the original power setting
	tcv_control (g_fd_rf, PHYSOPT_GETPOWER, &spow);
	ntries = 0;
	// Select PATABLE [0]
	tcv_control (g_fd_rf, PHYSOPT_SETPOWER, &ntries);
	// Starting value
	g_pat_cset [1] = 0;

  entry PA_CSET:

	tcv_control (g_fd_rf, PHYSOPT_RESET, (address)&g_pat_cset);
	g_pat_cntr = 0;
	g_pat_acc = 0;
	g_pat_nlqi = g_pat_maxlqi = g_pat_drop = 0;
	delay (10, PA_RUN);
	ntries = 0;
	release;

  entry PA_RUN:

  	address packet;

	// Issue a command
	if (g_pat_cnt == 0)
		// Aborted
		proceed PA_DONE;
		
	packet = tcv_wnp (PA_RUN, g_fd_rf, (POFF_CMD + 1 + 1) * 2);
	packet [POFF_RCV] = g_pat_peer;
	packet [POFF_SND] = HOST_ID;
	packet [POFF_SER] = (word) (g_pat_cset [1]);
	packet [POFF_FLG] = 0;
	strcpy ((char*)(packet+POFF_CMD), "+");
	tcv_endp (packet);

	// Wait for a signal or a timeout
	delay (PATABLE_REPLY_DELAY, PA_TMOUT);
	when (&g_pat_cred, PA_NEWVAL);
	release;

  entry PA_TMOUT:
#if 0
	if (ntries > PATABLE_MAX_TRIES) {
		g_pat_cntr -= g_pat_drop;
		proceed PA_NEXT;
	}

	ntries++;
	proceed PA_RUN;
#endif
	g_pat_drop++;
	g_pat_cred = 0;

  entry PA_NEWVAL:

	if (g_pat_cnt == 0)
		// Aborted
		proceed PA_DONE;

	g_pat_acc += g_pat_cred;
	g_pat_cntr++;

	if (g_pat_cntr < g_pat_cnt) {
		// Need more samples
		ntries = 0;
		proceed PA_RUN;
	}

	g_pat_cntr -= g_pat_drop;

  entry PA_NEXT:

#ifdef CHRONOS

	enc_hex (g_pat_cset [1], 2);
	enc_dec ((word) (g_pat_cntr  > 1 ? g_pat_acc / g_pat_cntr : g_pat_acc),
		3);

	m_out (PA_NEXT, c_msg);
#else
	uart_outf (PA_NEXT, "0x%x, %d, %d, %u, %u", g_pat_cset [1],
		(word) (g_pat_cntr  > 1 ? g_pat_acc / g_pat_cntr : g_pat_acc),
			g_pat_cntr, g_pat_nlqi, g_pat_maxlqi);
#endif
	// Next value
	if (g_pat_cset [1] != 255) {
		g_pat_cset [1] ++;
		proceed PA_CSET;
	}

	// All done

  entry PA_DONE:

#ifdef CHRONOS
	m_out (PA_DONE, "DONE");
#else
	uart_out (PA_DONE, "All done");
#endif
	// Resume previous setting
#ifdef CC1100_OLD_DRIVER
	tcv_control (g_fd_rf, PHYSOPT_RESET, (address)g_reg_suppl);
#else
	g_pat_cset [1] = g_patable [0];
	tcv_control (g_fd_rf, PHYSOPT_RESET, (address)&g_pat_cset);
#endif
	tcv_control (g_fd_rf, PHYSOPT_SETPOWER, &spow);
	finish;
}

#endif /* __SMURPH__ */

// ============================================================================

fsm thread_rfcmd {

  entry RC_START:

	g_snd_rtries = 0;
	g_snd_sernum++;

  entry RC_SEND:

  	word ln;
  	address packet;

	if (g_snd_rtries >= RF_COMMAND_RETRIES) {
		if (g_snd_rcmd != NULL) {
			ufree (g_snd_rcmd);
			g_snd_rcmd = NULL;
		}
		if (g_snd_rtries > RF_COMMAND_RETRIES) {
			// Response
#ifdef CHRONOS
			if (g_snd_rcode)
				m_out (WNONE, "RERR");
			else
				m_out (WNONE, "ROK");
#else
			if (g_snd_rcode)
				uart_outf (WNONE, "Error %u from %u",
					g_snd_rcode, g_snd_rnode);
			else
				uart_outf (WNONE, "OK from %u", g_snd_rnode);
#endif
		} else {
			// Timeout
#ifdef CHRONOS
			m_out (WNONE, "RTMO");
#else
			uart_outf (WNONE, "Timeout reaching %u", g_snd_rnode);
#endif
		}
		g_snd_rnode = 0;
		finish;
	}

	if ((ln = strlen (g_snd_rcmd) + ((POFF_CMD + 1) * 2) + 1) & 1)
		// Make sure the length is even
		ln++;

	when (&g_snd_rnode, RC_SEND);

	packet = tcv_wnp (RC_SEND, g_fd_rf, ln);

	packet [POFF_RCV] = g_snd_rnode;
	packet [POFF_SND] = HOST_ID;
	packet [POFF_FLG] = 0;
	packet [POFF_SER] = g_snd_sernum;

	strcpy ((char*)(packet + POFF_CMD), g_snd_rcmd);

	tcv_endp (packet);

	g_snd_rtries++;

	delay (RF_COMMAND_SPACING, RC_SEND);
}

// ============================================================================

fsm thread_rreporter {

#ifndef CHRONOS

  word nn, nx;

  entry UF_START:

	uart_outf (UF_START, "Stats of node %u:", g_rcv_ackrp [POFF_RCV]);
#endif

  entry UF_FIXED:

#ifdef CHRONOS
	enc_dec (g_rcv_ackrp [POFF_SENT], 4);
	c_msg [0] = 'S';
	m_out (UF_FIXED, c_msg);
#else
	uart_outf (UF_FIXED, "Sent: %u", g_rcv_ackrp [POFF_SENT]);

	nn = 0;
	nx = POFF_NTAB;

  entry UF_NEXT:

	if (nn < g_rcv_ackrp [POFF_NNOD]) {
		uart_outf (UF_NEXT, "Received from %u: %u",
			g_rcv_ackrp [nx], g_rcv_ackrp [nx + 1]);
		nn ++;
		nx += 2;
		proceed UF_NEXT;
	}

  entry UF_DRIV:

	uart_outf (UF_DRIV, "Driver stats: %u, %u, %u, %u, %u, %u",
			g_rcv_ackrp [POFF_RERR  ],
			g_rcv_ackrp [POFF_RERR+1],
			g_rcv_ackrp [POFF_RERR+2],
			g_rcv_ackrp [POFF_RERR+3],
			g_rcv_ackrp [POFF_RERR+4],
			g_rcv_ackrp [POFF_RERR+5] );
#endif
	tcv_endp (g_rcv_ackrp);
	g_rcv_ackrp = NULL;

	finish;
}

// ============================================================================

fsm thread_ureporter (word clear) {

#ifndef CHRONOS

  word cnt;

  entry RE_START:

	tcv_control (g_fd_rf, PHYSOPT_GETCHANNEL, &cnt);
	uart_outf (RE_START, "Stats (local) ch = %u):", cnt);

  entry RE_MEM:

  	word vals [3];

	vals [0] = memfree (0, vals + 1);
	vals [2] = stackfree ();
	uart_outf (RE_MEM, "Mem: %u %u %u" , vals [0], vals [1], vals [2]);
#endif

  entry RE_FIXED:

#ifdef CHRONOS
	enc_dec (g_snd_count, 4);
	c_msg [0] = 'S';
	m_out (RE_FIXED, c_msg);
#else
	uart_outf (RE_FIXED, "Sent: %u", g_snd_count);
	cnt = 0;

  entry RE_NEXT:

	if (cnt < MAX_NODES && g_rep_nodes [cnt] . Node != 0) {
		uart_outf (RE_NEXT, "Received from %u: %u",
			g_rep_nodes [cnt] . Node,
			g_rep_nodes [cnt] . Count);
		cnt++;
		proceed RE_NEXT;
	}

  entry RE_DRIV:

	word errs [6];

	tcv_control (g_fd_rf, PHYSOPT_ERROR, errs);
#endif
	if (clear)
		tcv_control (g_fd_rf, PHYSOPT_ERROR, NULL);

#ifndef CHRONOS
	uart_outf (RE_DRIV, "Driver stats: %u, %u, %u, %u, %u, %u",
		errs [0], errs [1], errs [2], errs [3], errs [4], errs [5]);
#endif
	finish;
}

// ============================================================================

fsm thread_listener {

  entry LI_WAIT:

  	address packet;
  	word pl, tr;

	packet = tcv_rnp (LI_WAIT, g_fd_rf);
	g_lrs = seconds ();
	// Packet length in words
	pl = (tcv_left (packet) >> 1);
	tr = packet [pl - 1];
	g_last_rssi = (byte)(tr >> 8);
	g_last_qual = (byte) tr      ;

	if (pl >= MIN_ANY_PACKET_LENGTH/2) {

		if (packet [POFF_RCV] == 0) {
			// This is one of the packets to be counted
			update_count (packet [POFF_SND]);
			if ((g_flags & 0x4000))
				view_packet (packet, pl);
		} else if (packet [POFF_RCV] == HOST_ID &&
			    packet [POFF_SND] != 0) {
				// This is a command addressed to us
				pl = do_command ((char*)(packet + POFF_CMD),
					packet [POFF_SND], packet [POFF_SER]);
				if (pl != WNONE)
					confirm (packet [POFF_SND],
						 packet [POFF_SER], pl);
		} else if (packet [POFF_SND] == HOST_ID &&
		    packet [POFF_RCV] == g_snd_rnode &&
		    packet [POFF_SER] == g_snd_sernum) {
			// An ack for our remote command
			handle_ack (packet, pl);
			// handle_ack is responsible for deallocating
			// the packet
			proceed LI_WAIT;
		}
	}

	tcv_endp (packet);
	proceed LI_WAIT;
}

// ============================================================================

fsm thread_sender (word dl) {

  word scnt, sernum;
  address spkt;

#if NUMBER_OF_SENSORS > 0

  word sval [NUMBER_OF_SENSORS];

#endif

  entry SN_INIT:

	// A delay at the beginning, so any threaded preprocessing
	// has a chance to complete
	sernum = 0;
	delay (dl, SN_START);
	release;

  entry SN_START:

	// A special case for the first packet which goes right away
	gen_send_params ();
	scnt = 0;
	proceed SN_NSEN;

  entry SN_DELAY:

	delay (gen_send_params (), SN_NSEN);
	scnt = 0;
	release;

  entry SN_NSEN:

#if NUMBER_OF_SENSORS > 0

	if (scnt < NUMBER_OF_SENSORS) {
		read_sensor (SN_NSEN, scnt, sval + scnt);
		scnt++;
		proceed SN_NSEN;
	}

#endif

  entry SN_NEXT:

	spkt = tcv_wnps (SN_NEXT, g_fd_rf, g_snd_opl, g_snd_urgent);
	spkt [POFF_RCV] = 0;
	spkt [POFF_SND] = HOST_ID;
	spkt [POFF_SER] = sernum++;
	spkt [POFF_FLG] = tcv_control (g_fd_rf, PHYSOPT_GETPOWER, NULL) |
		g_flags;

#if NUMBER_OF_SENSORS > 0

	memcpy (spkt + POFF_SEN, sval, NUMBER_OF_SENSORS * sizeof (word));

#endif

	scnt += POFF_SEN;

	// Turn into word count and remove checksum
	g_snd_opl = (g_snd_opl - 2) >> 1;

	while (scnt < g_snd_opl)
		spkt [scnt++] = (word) rnd ();

	tcv_endp (spkt);
	g_snd_count++;

	if (g_snd_left && --g_snd_left == 0)
		// Zero means infinite
		finish;
		
	proceed SN_DELAY;
}

#ifdef CHRONOS

// ============================================================================
// CHRONOS ====================================================================
// ============================================================================

static byte rcv_stat = 0, lcd_stat = 0;

static void buttons (word but) {

	word estat;

	estat = 13;

	switch (but) {

	    case 0:

		if (running (thread_sender)) {
			estat = do_command ("q", 0, 0);
			msg_hi ("STOP");
		} else {
			estat = do_command ("s", 0, 0);
			msg_hi ("SEND");
		}
		break;

	    case 1:

		if (g_flags & 0x4000) {
			estat = do_command ("n", 0, 0);
			msg_hi ("TERS");
		} else {
			estat = do_command ("v", 0, 0);
			msg_hi ("VERB");
		}
		break;

	    case 2:

		if (g_flags & 0x8000) {
			estat = do_command ("u", 0, 0);
			msg_hi ("PWUP");
		} else {
			estat = do_command ("d", 0, 0);
			msg_hi ("PWDN");
		}
		break;

	    case 3:

		if (rcv_stat == 0) {
			// OFF
			rcv_stat = 1;
			estat = do_command ("f", 0, 0);
			msg_hi ("RXOF");
		} else if (rcv_stat == 1) {
			rcv_stat = 2;
			estat = do_command ("f 1", 0, 0);
			msg_hi ("RXWO");
		} else {
			rcv_stat = 0;
			estat = do_command ("g", 0, 0);
		}
		break;

	    case 4:

		if (lcd_stat) {
			// Turn on
			lcd_stat = 0;
			ezlcd_on ();
		} else {
			lcd_stat = 1;
			ezlcd_off ();
		}
		break;
	}

	err_msg (WNONE, estat);
}

fsm root {

  word estat;

  entry RS_INIT:

	word scr;

#ifdef TLDEBUG
	tld_init (20, 2, 64);
#endif
	ezlcd_init ();
	ezlcd_on ();

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	g_fd_rf = tcv_open (WNONE, 0, 0);	// NULL plug on CC1100

	if (g_fd_rf < 0)
		syserror (ERESOURCE, "app desc");

	g_flags = HOST_FLAGS;

	//
	// d a z z z p p p c c c c c c c c
	//
	// d 	- power down flag
	// a    - active flag (sender initially on)
	// zzz  - last three bits of network ID
	// ppp  - default power level for xmitter
	// c..c - the channel
	//

	scr = NETWORK_ID | ((g_flags >> 11) & 0x7);
	tcv_control (g_fd_rf, PHYSOPT_SETSID, &scr);
	scr = (g_flags >> 8) & 0x7;
	tcv_control (g_fd_rf, PHYSOPT_SETPOWER, &scr);
	tcv_control (g_fd_rf, PHYSOPT_SETCHANNEL, &g_flags);
	tcv_control (g_fd_rf, PHYSOPT_RXON, NULL);

	runfsm thread_listener;

	if (g_flags & 0x4000)
		runfsm thread_sender (0);

	// Only this one stays
	g_flags &= 0x8000;

	if (g_flags)
		powerdown ();

	msg_hi ("RFTS");
	msg_lo ("READY");

	buttons_action (buttons);
	finish;
}
	
#else

// ============================================================================
// UART =======================================================================
// ============================================================================

fsm root {

  word estat;

  entry RS_INIT:

#ifdef TLDEBUG
	tld_init (20, 2, 64);
#endif

	// Packet length for the PHY doesn't cover the checksum
	phys_cc1100 (0, MAX_PACKET_LENGTH);
	phys_uart (1, UART_LINE_LENGTH, 0);
	tcv_plug (0, &plug_null);
	g_fd_rf = tcv_open (WNONE, 0, 0);	// NULL plug on CC1100
	g_fd_uart = tcv_open (WNONE, 1, 0);	// NULL plug on UART

	if (g_fd_rf < 0 || g_fd_uart < 0)
		syserror (ERESOURCE, "app desc");


	g_flags = HOST_FLAGS;

	//
	// d a z z z p p p c c c c c c c c
	//
	// d 	- power down flag
	// a    - active flag (sender initially on)
	// zzz  - last three bits of network ID
	// ppp  - default power level for xmitter
	// c..c - the channel
	//

  entry RS_BANNER:

  	word scr;

	scr = NETWORK_ID | ((g_flags >> 11) & 0x7);
	uart_outf (RS_BANNER,
		"Node: %u, NetId: %x, Channel: %u, XPower: %u, Power%s, %s",
			HOST_ID, scr, g_flags & 0xff, (g_flags >> 8) & 0x7,
				(g_flags & 0x8000) ? "down" : "up",
				(g_flags & 0x4000) ? "Active" : "Passive");

	tcv_control (g_fd_rf, PHYSOPT_SETSID, &scr);
	scr = (g_flags >> 8) & 0x7;
	tcv_control (g_fd_rf, PHYSOPT_SETPOWER, &scr);
	tcv_control (g_fd_rf, PHYSOPT_SETCHANNEL, &g_flags);

	// tcv_control (g_fd_rf, PHYSOPT_TXON, NULL);
	tcv_control (g_fd_rf, PHYSOPT_RXON, NULL);
	runfsm thread_listener;

	if (g_flags & 0x4000)
		runfsm thread_sender (0);

	// Only this one stays
	g_flags &= 0x8000;

	if (g_flags)
		powerdown ();

  entry RS_ON:

	uart_outf (RS_ON, "Ready %u:", HOST_ID);

  entry RS_READ:

	address packet;

	// Read a command from input
	packet = tcv_rnp (RS_READ, g_fd_uart);

	// Process the command
	estat = do_command ((char*)packet, 0, 0);

	tcv_endp (packet);

  entry RS_MSG:

	err_msg (RS_MSG, estat);
	proceed RS_ON;
}

#endif /* CHRONOS or UART */

// ============================================================================
