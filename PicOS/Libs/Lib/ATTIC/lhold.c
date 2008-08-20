/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "lhold.h"

void lhold (word st, lword *nsec) {
/*
 * Holds the process for the specified number of seconds, as in:
 *
 *      lword del;
 *      ...
 *      ...
 *        del = 77865;
 * 
 *      entry (HOLD_ME);
 *        lhold (HOLD_ME, &del);
 *        ...
 *        ... will get here after 77865 seconds ...
 */
	word ns, nm;

	if (*nsec == 0)
		return;

	if (*nsec < 64 || (*nsec & 0x80000000) != 0) {
		// Residual wait for seconds
		delay (((word) *nsec) << 10, st);
		*nsec = 0;
		release;
	}

	// Elapsed seconds of this minute
	ns = SECONDS_IN_MINUTE - sectomin () +
	// Add residual seconds of the requested delay
		(*nsec % SECONDS_IN_MINUTE);
	// FIXME: check how this assembles for 64 spm

	// Turn the requested delay into full minutes
	nm = (*nsec / SECONDS_IN_MINUTE);

	// Correction
	if (ns >= SECONDS_IN_MINUTE) {
		ns -= SECONDS_IN_MINUTE;
		nm++;
	}

	if (ns) {
		// There are residual seconds
		if (nm) {
			// And non-zero minutes
			*nsec = (lword) ns | 0x80000000;
			ldelay (nm, st);
		} else {
			// Zero minutes, just the seconds
			*nsec = 0;
			delay (ns << 10, st);
		}
		release;
	}

	// No residual seconds
	*nsec = 0;
	if (nm) {
		// minutes
		ldelay (nm, st);
		release;
	}
	// Just return
}

lword lhleft (int pid, lword *nsec) {

	word nm, ns;

	// First check if we are waiting on long delay
	if ((nm = ldleft (pid, &ns)) != MAX_UINT) {
		if ((*nsec & 0x80000000) != 0)
			ns += (word) *nsec;
		return ((lword) nm * SECONDS_IN_MINUTE) + ns;
	} else {
		if ((ns = dleft (pid)) != MAX_UINT)
			return (lword) (ns >> 10);
	}
	// Neither one nor the other
	return *nsec & 0x7fffffff;
}
