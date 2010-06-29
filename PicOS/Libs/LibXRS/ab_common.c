#ifndef	__ab_common_c__
#define	__ab_common_c__

#include "sysio.h"
#include "ab_params.h"
#include "ab_modes.h"

#ifdef	__SMURPH__

threadhdr (ab_xrs_drvr, PicOSNode) {

	void *data;

	states { AB_LOOP, AB_RCV };

	void setup (void *d) { data = d; };

	perform;
};

#else

#define	__dcx_def__
#include "ab_data.h"
#undef	__dcx_def__

#define	AB_LOOP		0
#define	AB_RCV		1

#endif

// ============================================================================

static Boolean ab_xrs_send (int sid) {

	word ln;

	if (((ln = ab_xrs_sln) & 1))
		// Packet length must always be even; we assume that
		// otherwise ab_xrs_sln in within limits (was checked
		// by the API function
		ln++;

	// Note that ln can be zero - if we have nothing to send
	if ((ab_xrs_pac = tcv_wnp (WNONE, sid, ln + AB_MINPL)) == NULL)
		return NO;

	AB_PF_MAG = AB_PMAGIC;
	AB_PF_LEN = ab_xrs_sln;
	AB_PF_CUR = ab_xrs_cur;
	AB_PF_EXP = ab_xrs_exp;

	// Note: this does nothing if ab_xrs_sln == 0
	memcpy (AB_PF_PAY, ab_xrs_cou, ab_xrs_sln);
	tcv_endp (ab_xrs_pac);
	return YES;
}

static void ab_xrs_recv () {

	word ln, fl;

	if (AB_PF_MAG != AB_PMAGIC) {
		// Not for us
		tcv_endp (ab_xrs_pac);
		return;
	}

	if (ab_xrs_sln) {
		// We do have an outgoing message
		if (AB_PF_EXP != ab_xrs_cur) {
			// Expected != our current, we are done with this
			// message
			ufree (ab_xrs_cou);
			ab_xrs_sln = 0;
			ab_xrs_new = 0;
			trigger (AB_EVENT_OUT);
			ab_xrs_cur = AB_PF_EXP;
		} else {
			// Expecting our current
			if (ab_xrs_new == 0)
				ab_xrs_new = 1;
			trigger (AB_EVENT_RUN);
		}
	} else {
		// This is what they expect
		ab_xrs_cur = AB_PF_EXP;
	}

	// Should we receive?
	if ((ln = tcv_left (ab_xrs_pac)) <= AB_MINPL)
		// No message, pure ACK
		goto Done;

	if (ab_xrs_new == 0)
		// Send immediately an ACK or NACK
		ab_xrs_new = 1;

	trigger (AB_EVENT_RUN);

	if (AB_PF_CUR != ab_xrs_exp)
		// Ignore
		goto Done;

	// Effective length
	fl = AB_PF_LEN;
	if (fl == 0 || fl > (ln - AB_MINPL))
		// Consistency check fails
		goto Done;

	// Receive it
	if (ab_xrs_cin != NULL)
		// New message overwrites old one
		ufree (ab_xrs_cin);

	// If this is a string, the sentinel is included in fl
	if ((ab_xrs_cin = (char*) umalloc (fl)) == NULL)
		// Just ignore for now, it will arrive later again
		goto Done;

	memcpy (ab_xrs_cin, AB_PF_PAY, fl);
	ab_xrs_rln = (byte) fl;
	ab_xrs_exp++;

	trigger (AB_EVENT_IN);
Done:
	tcv_endp (ab_xrs_pac);
}

// ============================================================================

strand (ab_xrs_drvr, void)

#define	SID ((int)data)

	entry (AB_LOOP)

		switch (ab_xrs_mod) {

			case AB_MODE_PASSIVE:

				// Do not send unless have something to send
				if (ab_xrs_new) {
					if (ab_xrs_send (SID)) {
						if (--ab_xrs_new)
							delay (AB_DELAY_LONG,
								AB_LOOP);
					} else 
						// Send failed, keep retrying
						delay (AB_DELAY_SHORT, AB_LOOP);
				}

				break;

			case AB_MODE_ACTIVE:

				// Keep polling 
				if (ab_xrs_send (SID))
					delay (AB_DELAY_LONG, AB_LOOP);
				else
					delay (AB_DELAY_SHORT, AB_LOOP);

				if (ab_xrs_new)
					ab_xrs_new--;

				break;

			// The default is OFF meaning be quiet
		}

		// Wait for run events
GetIt:
		when (AB_EVENT_RUN, AB_LOOP);
		// Try to receive
		ab_xrs_pac = tcv_rnp (AB_RCV, SID);
		ab_xrs_recv ();
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
	if ((ab_xrs_han = runstrand (ab_xrs_drvr, (void*)sid)) == 0)
		syserror (ERESOURCE, "ab_xrs_drvr");

	// Maximum payload length (note that AB_MINPL includes checksum)
	ab_xrs_max = tcv_control (sid, PHYSOPT_GETMAXPL, NULL) - AB_MINPL + 2;
}

void ab_mode (byte mode) {
//
// Set the mode
//
	ab_xrs_mod = (mode > 2) ? 2 : mode;
	ptrigger (ab_xrs_han, AB_EVENT_RUN);
	trigger (AB_EVENT_OUT);
}

#endif
