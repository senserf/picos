#ifndef	__xrs_cc__
#define __xrs_cc__

#include "board.h"
#include "stdattr.h"

#define	AB_DELAY_SHORT	512
#define	AB_DELAY_LONG	2048

#define	AB_EVENT_IN	((void*)(&(UA->ab_cin)))
#define	AB_EVENT_OUT	((void*)(&(UA->ab_cout)))
#define	AB_EVENT_RUN	((void*)(&(UA->ab_new)))

#define	AB_MINPL	(4+2)	// Header + checksum

#define	AB_PL_CUR	0
#define	AB_PL_EXP	1
#define	AB_PL_MAG	2
#define	AB_PL_LEN	3
#define	AB_PL_PAY	4

#define	AB_PMAGIC	0xAB

#define	AB_PF_CUR	(((byte*)packet)[AB_PL_CUR])
#define	AB_PF_EXP	(((byte*)packet)[AB_PL_EXP])
#define	AB_PF_MAG	(((byte*)packet)[AB_PL_MAG])
#define	AB_PF_LEN	(((byte*)packet)[AB_PL_LEN])
#define	AB_PF_PAY	(((byte*)packet)+AB_PL_PAY )

#define	AB_MAXPAY	(UA->r_buffl - AB_MINPL + 2)

#define	AB_XTRIES	3

ab_driver_p::perform {

	TIME t;

	state AB_LOOP:

		switch (UA->ab_md) {

			case AB_MODE_PASSIVE:
	
				// Do not talk unless have something to say
				if (UA->ab_new) {
					if (ab_send (SID)) {
						if (--(UA->ab_new))
							delay (AB_DELAY_LONG,
								AB_LOOP);
					} else
						delay (AB_DELAY_SHORT, AB_LOOP);
				}

				break;

			case AB_MODE_ACTIVE:

				// Keep polling even if nothing to send
				if (ab_send (SID))
					delay (AB_DELAY_LONG, AB_LOOP);
				else
					delay (AB_DELAY_SHORT, AB_LOOP);

				if (UA->ab_new)
					// Redundant in this mode
					UA->ab_new--;

				break;
	
			// The default is OFF, meaning do nothing for this part
		}

		// Wait for RUN events
GetIt:
		when (AB_EVENT_RUN, AB_LOOP);

		// Try to receive
		packet = tcv_rnp (AB_RCV, SID);
		ab_receive ();
		proceed AB_LOOP;

	state AB_RCV:

		// In case we get hung on RCV
		delay (AB_DELAY_SHORT, AB_LOOP);
		goto GetIt;
}

Boolean ab_driver_p::ab_send (int sid) {

	word ln;

	if (UA->ab_cout) {
		// There is an outgoing message
		if ((ln = strlen (UA->ab_cout)) > AB_MAXPAY)
			ln = AB_MAXPAY;
	} else
		ln = 0;

	if ((packet = tcv_wnp (WNONE, sid,
		// Make sure the length is always even
		ln + ((ln & 1) ? AB_MINPL+1 : AB_MINPL))) == NULL)
			return NO;

	AB_PF_MAG = AB_PMAGIC;
	AB_PF_LEN = (byte) ln;
	AB_PF_CUR = ab_cur;
	AB_PF_EXP = ab_exp;

	memcpy (AB_PF_PAY, UA->ab_cout, ln);
	tcv_endp (packet);
	return YES;
}

void ab_driver_p::ab_receive () {

	word ln, fl;

	if (AB_PF_MAG != AB_PMAGIC) {
		// This packet does not belong to us
		tcv_endp (packet);
		return;
	}

	if (UA->ab_cout) {
		// We do have an outgoing message
		if (AB_PF_EXP != ab_cur) {
			// Expected != our current, we are done with this
			// message
			ufree (UA->ab_cout);
			UA->ab_cout = NULL;
			UA->ab_new = 0;
			trigger (AB_EVENT_OUT);
			ab_cur = AB_PF_EXP;
		} else {
			if (UA->ab_new == 0)
				UA->ab_new = 1;
			trigger (AB_EVENT_RUN);
		}
	} else
		// This is what they expect
		ab_cur = AB_PF_EXP;

	// Should we receive
	if ((ln = tcv_left (packet)) <= AB_MINPL)
		// No message, pure ACK
		goto Done;

	// We will ACK (or NACK) this
	if (UA->ab_new == 0)
		UA->ab_new = 1;

	trigger (AB_EVENT_RUN);

	if (AB_PF_CUR != ab_exp)
		// Ignore
		goto Done;

	fl = AB_PF_LEN;
	if (fl == 0 || fl > (ln - AB_MINPL))
		// Consistency check
		goto Done;

	// Receive it
	if (UA->ab_cin != NULL)
		// New message overwrites old one
		ufree (UA->ab_cin);

	if ((UA->ab_cin = (char*) umalloc (fl + 1)) == NULL)
		goto Done;

	memcpy (UA->ab_cin, AB_PF_PAY, fl);
	UA->ab_cin [fl] = '\0';
	ab_exp++;

	trigger (AB_EVENT_IN);
Done:
	tcv_endp (packet);
}

// ============================================================================

__PUBLF (PicOSNode, void, ab_init) (int sid) {
//
// Start the protocol
//
	uart_tcv_int_t *UA;

	UA = UART_INTF_P (uart);

	if (tally_in_pcs ())
		create ab_driver_p (UA, sid);
	else
		syserror (ERESOURCE, "ab_init fork");
}

void ab_driver_p::setup (uart_tcv_int_t *ua, int sid) {

	UA = ua;
	ab_cur = ab_exp = 0;
	UA->ab_md = AB_MODE_PASSIVE;
	SID = sid;
}

__PUBLF (PicOSNode, void, ab_mode) (byte mode) {

	uart_tcv_int_t *UA;

	UA = UART_INTF_P (uart);

	UA->ab_md = (mode > 2) ? 2 : mode;

	trigger (AB_EVENT_RUN);
	trigger (AB_EVENT_OUT);
}

// ============================================================================

__PUBLF (PicOSNode, void, ab_outf) (word st, const char *fm, ...) {
//
// Send a message
//
	va_list ap;
	uart_tcv_int_t *UA;

	UA = UART_INTF_P (uart);

	if (UA->ab_cout || !UA->ab_md) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		sleep;
	}

	va_start (ap, fm);

	if ((UA->ab_cout = vform (NULL, fm, ap)) == NULL) {
		// Out of memory
		umwait (st);
		sleep;
	}
	UA->ab_new = AB_XTRIES;
	trigger (AB_EVENT_RUN);
}

__PUBLF (PicOSNode, void, ab_out) (word st, char *str) {
//
// Send a formatted message; the string is assumed to have been malloc'ed
//
	uart_tcv_int_t *UA;

	UA = UART_INTF_P (uart);

	if (UA->ab_cout || !UA->ab_md) {
		when (AB_EVENT_OUT, st);
		sleep;
	}

	UA->ab_cout = str;
	UA->ab_new = AB_XTRIES;
	trigger (AB_EVENT_RUN);
}

__PUBLF (PicOSNode, int, ab_inf) (word st, const char *fm, ...) {
//
// Receive a message
//
	va_list ap;
	char *lin;
	int res;

	lin = ab_in (st);
	va_start (ap, fm);
	res = vscan (lin, fm, ap);
	ufree (lin);
	return res;
}

__PUBLF (PicOSNode, char*, ab_in) (word st) {
//
// Raw receive (have to clean up yourself)
//
	char *res;
	uart_tcv_int_t *UA;

	UA = UART_INTF_P (uart);

	if (UA->ab_cin == NULL) {
		when (AB_EVENT_IN, st);
		sleep;
	}

	res = UA->ab_cin;
	UA->ab_cin = NULL;
	trigger (AB_EVENT_RUN);
	return res;
}

#endif
