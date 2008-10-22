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

#define	AB_PF_CUR	(((byte*)packet)[AB_PL_CUR])
#define	AB_PF_EXP	(((byte*)packet)[AB_PL_EXP])
#define	AB_PF_MAG	(((byte*)packet)[AB_PL_MAG])
#define	AB_PF_LEN	(((byte*)packet)[AB_PL_LEN])
#define	AB_PF_PAY	(((byte*)packet)+AB_PL_PAY )

#ifndef	AB_MINPL
#define	AB_MINPL	(4+2)	// Header + checksum
#endif

#ifndef	AB_DELAY_SHORT
#define	AB_DELAY_SHORT	512
#endif

#ifndef	AB_DELAY_LONG
#define	AB_DELAY_LONG	2048
#endif

static Boolean ab_new = NO, ab_running = NO;
static byte ab_cur = 0, ab_exp = 0;
static char *ab_cout = NULL, *ab_cin = NULL;

static int ab_handler = 0;
static word ab_delay = AB_DELAY_SHORT, ab_left = MAX_UINT, ab_maxpay = 0;
static address packet;

#define	AB_EVENT_IN	((void*)(&ab_cin))
#define	AB_EVENT_OUT	((void*)(&ab_cout))
#define	AB_EVENT_RUN	((void*)(&ab_running))

// ============================================================================

static Boolean ab_send (int sid) {

	word ln;

	if (ab_cout) {
		// There is an outgoing message
		if ((ln = strlen (ab_cout)) > ab_maxpay)
			ln = ab_maxpay;
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

	memcpy (AB_PF_PAY, ab_cout, ln);
	tcv_endp (packet);
	return YES;
}

static void ab_receive () {

	word ln, fl;

	if (AB_PF_MAG != AB_PMAGIC)
		goto Done;

	if (ab_cout) {
		// We do have an outgoing message
		if (AB_PF_EXP != ab_cur) {
			// Expected != our current, we are done with this
			// message
			ufree (ab_cout);
			ab_cout = NULL;
			trigger (AB_EVENT_OUT);
			ab_cur = AB_PF_EXP;
		} else
			ab_delay = AB_DELAY_SHORT;
	} else
		ab_cur = AB_PF_EXP;

	// Should we receive
	if ((ln = tcv_left (packet)) <= AB_MINPL)
		// No message
		goto Done;

	if (AB_PF_CUR != ab_exp) {
		// Ignore
		ab_delay = AB_DELAY_SHORT;
		goto Done;
	}

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
	// Force an ACK
	ab_new = YES;
Done:
	tcv_endp (packet);
}

// ============================================================================

strand (ab_driver, void)

#define	AB_LOOP		0
#define	AB_TIMER	1

#define	SID	((int)data)

	entry (AB_LOOP)

		if (ab_running) {
			// Try to send
			ab_left = dleft (0);
			if (ab_new) {
				// New outgoing message
Send:
				if (!ab_send (SID))
					// Failed
					ab_delay = AB_DELAY_SHORT;
				ab_new = NO;
			}
			// Timer
			if (ab_left < ab_delay)
				ab_delay = ab_left;
			delay (ab_delay, AB_TIMER);
			ab_delay = AB_DELAY_LONG;
		}
Off:
		// Wait for RUN events
		when (AB_EVENT_RUN, AB_LOOP);
		// Receive (also if we are off)
		packet = tcv_rnp (AB_LOOP, SID);
		ab_receive ();
		proceed (AB_LOOP);

	entry (AB_TIMER)

		// Actual timer going off
		ab_left = MAX_UINT;
		if (ab_running)
			goto Send;

		goto Off;
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
	ab_running = YES;
}

void ab_on () {

	ab_running = YES;
	ptrigger (ab_handler, AB_EVENT_RUN);
	trigger (AB_EVENT_OUT);
}

void ab_off () {

	ab_running = NO;
	ptrigger (ab_handler, AB_EVENT_RUN);
}

// ============================================================================

void ab_outf (word st, const char *fm, ...) {
//
// Send a message
//
	va_list ap;

	if (ab_cout || !ab_running) {
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
	ab_new = YES;
	ptrigger (ab_handler, AB_EVENT_RUN);
}

void ab_out (word st, char *str) {
//
// Send a formatted message; the string is assumed to have been malloc'ed
//
	if (ab_cout || !ab_running) {
		when (AB_EVENT_OUT, st);
		release;
	}

	ab_cout = str;
	ab_new = YES;
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
