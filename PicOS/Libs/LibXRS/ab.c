#ifndef	__ab_c__
#define	__ab_c__

#include "sysio.h"
#include "ab.h"
#include "form.h"

#ifndef	AB_PL_CUR
#define	AB_PL_CUR	0
#endif

#ifndef	AB_PL_EXP
#define	AB_PL_EXP	1
#endif

#ifndef	AB_PL_MAG
#define	AB_PL_MAG	2
#endif

#ifndef	AB_PL_LEN
#define	AB_PL_LEN	3
#endif

#ifndef	AB_PL_PAY
#define	AB_PL_PAY	4
#endif

#ifndef	AB_PMAGIC
#define	AB_PMAGIC	0xAB
#endif

#define	AB_PF_CUR	(((byte*)ab_packet)[AB_PL_CUR])
#define	AB_PF_EXP	(((byte*)ab_packet)[AB_PL_EXP])
#define	AB_PF_MAG	(((byte*)ab_packet)[AB_PL_MAG])
#define	AB_PF_LEN	(((byte*)ab_packet)[AB_PL_LEN])
#define	AB_PF_PAY	(((byte*)ab_packet)+AB_PL_PAY )

#ifndef	AB_MINPL
#define	AB_MINPL	(4+2)	// Header + checksum
#endif

#ifndef	AB_DELAY_SHORT
#define	AB_DELAY_SHORT	512
#endif

#ifndef	AB_DELAY_LONG
#define	AB_DELAY_LONG	2048
#endif

#ifndef	AB_XTRIES
#define	AB_XTRIES	3
#endif

#define	AB_EVENT_IN	((void*)(&ab_cin))
#define	AB_EVENT_OUT	((void*)(&ab_cout))
#define	AB_EVENT_RUN	((void*)(&ab_new))

#ifdef	__SMURPH__

process ab_driver (PicOSNode) {

	void *data;

	states { AB_LOOP, AB_RCV };

	void setup (void *d) { data = d; };

	perform;
};

#define	ab_new		_dap (ab_new)
#define	ab_md		_dap (ab_md)
#define	ab_cur		_dap (ab_cur)
#define	ab_exp		_dap (ab_exp)
#define	ab_cout		_dap (ab_cout)
#define	ab_cin		_dap (ab_cin)
#define	ab_handler	_dap (ab_handler)
#define	ab_maxpay	_dap (ab_maxpay)
#define	ab_packet	_dap (ab_packet)

#else

#include "ab_node_data.h"

#define	AB_LOOP		0
#define	AB_RCV		1

#endif

// ============================================================================

static Boolean ab_send (int sid) {

	word ln;

	if (ab_cout) {
		// There is an outgoing message
		if ((ln = strlen (ab_cout)) > ab_maxpay)
			ln = ab_maxpay;
	} else
		ln = 0;

	if ((ab_packet = tcv_wnp (WNONE, sid,
		// Make sure the length is always even
		ln + ((ln & 1) ? AB_MINPL+1 : AB_MINPL))) == NULL)
			return NO;

	AB_PF_MAG = AB_PMAGIC;
	AB_PF_LEN = (byte) ln;
	AB_PF_CUR = ab_cur;
	AB_PF_EXP = ab_exp;

	memcpy (AB_PF_PAY, ab_cout, ln);
	tcv_endp (ab_packet);
	return YES;
}

static void ab_receive () {

	word ln, fl;

	if (AB_PF_MAG != AB_PMAGIC) {
		// Not for us
		tcv_endp (ab_packet);
		return;
	}

	if (ab_cout) {
		// We do have an outgoing message
		if (AB_PF_EXP != ab_cur) {
			// Expected != our current, we are done with this
			// message
			ufree (ab_cout);
			ab_cout = NULL;
			ab_new = 0;
			trigger (AB_EVENT_OUT);
			ab_cur = AB_PF_EXP;
		} else {
			// Expecting our current
			if (ab_new == 0)
				ab_new = 1;
			trigger (AB_EVENT_RUN);
		}
	} else {
		// This is what they expect
		ab_cur = AB_PF_EXP;
	}

	// Should we receive
	if ((ln = tcv_left (ab_packet)) <= AB_MINPL)
		// No message, pure ACK
		goto Done;

	if (ab_new == 0)
		// Send immediately an ACK or NACK
		ab_new = 1;

	trigger (AB_EVENT_RUN);

	if (AB_PF_CUR != ab_exp)
		// Ignore
		goto Done;

	fl = AB_PF_LEN;
	if (fl == 0 || fl > (ln - AB_MINPL))
		// Consistency check
		goto Done;

	// Receive it
	if (ab_cin != NULL)
		// New message overwrites old one
		ufree (ab_cin);

	if ((ab_cin = (char*) umalloc (fl + 1)) == NULL)
		goto Done;

	memcpy (ab_cin, AB_PF_PAY, fl);
	ab_cin [fl] = '\0';
	ab_exp++;

	trigger (AB_EVENT_IN);
Done:
	tcv_endp (ab_packet);
}

// ============================================================================

strand (ab_driver, void)

#define	SID	((int)data)

	entry (AB_LOOP)

		switch (ab_md) {

			case AB_MODE_PASSIVE:

				// Do not send unless have something to send
				if (ab_new) {
					if (ab_send (SID)) {
						if (--ab_new)
							delay (AB_DELAY_LONG,
								AB_LOOP);
					} else 
						// Send failed, keep retrying
						delay (AB_DELAY_SHORT, AB_LOOP);
				}

				break;

			case AB_MODE_ACTIVE:

				// Keep polling 
				if (ab_send (SID))
					delay (AB_DELAY_LONG, AB_LOOP);
				else
					delay (AB_DELAY_SHORT, AB_LOOP);

				if (ab_new)
					ab_new--;

				break;

			// The default is OFF meaning be quiet
		}

		// Wait for run events
GetIt:
		when (AB_EVENT_RUN, AB_LOOP);
		// Try to receive
		ab_packet = tcv_rnp (AB_RCV, SID);
		ab_receive ();
		proceed (AB_LOOP);


	entry (AB_RCV)

		// In case we get hung
		delay (AB_DELAY_SHORT, AB_LOOP);
		goto GetIt;
#undef	SID

endstrand

// ============================================================================

void ab_init (int sid) {
//
// Start the protocol
//
	if ((ab_handler = runstrand (ab_driver, (void*)sid)) == 0)
		syserror (ERESOURCE, "ab_driver");

	// Maximum payload length 
	ab_maxpay = tcv_control (sid, PHYSOPT_GETMAXPL, NULL) - AB_MINPL + 2;
}

void ab_mode (byte mode) {
//
// Set the mode
//
	ab_md = (mode > 2) ? 2 : mode;
	ptrigger (ab_handler, AB_EVENT_RUN);
	trigger (AB_EVENT_OUT);
}

// ============================================================================

void ab_outf (word st, const char *fm, ...) {
//
// Send a message
//
	va_list ap;

	if (ab_cout || ab_md == 0) {
		// Off or busy
		when (AB_EVENT_OUT, st);
		release;
	}

	va_start (ap, fm);

	if ((ab_cout = vform (NULL, fm, ap)) == NULL) {
		// Out of memory
		umwait (st);
		release;
	}
	ab_new = AB_XTRIES;
	ptrigger (ab_handler, AB_EVENT_RUN);
}

void ab_out (word st, char *str) {
//
// Send a formatted message; the string is assumed to have been malloc'ed
//
	if (ab_cout || ab_md == 0) {
		when (AB_EVENT_OUT, st);
		release;
	}

	ab_cout = str;
	ab_new = AB_XTRIES;
	ptrigger (ab_handler, AB_EVENT_RUN);
}

int ab_inf (word st, const char *fm, ...) {
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

char *ab_in (word st) {
//
// Raw receive (have to clean up yourself)
//
	char *res;

	if (ab_cin == NULL) {
		when (AB_EVENT_IN, st);
		release;
	}

	res = ab_cin;
	ab_cin = NULL;
	ptrigger (ab_handler, AB_EVENT_RUN);
	return res;
}

#endif
