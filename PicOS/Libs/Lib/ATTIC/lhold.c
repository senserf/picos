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
		// Residual wait (seconds correction)
		delay (((word) *nsec) << 10, st);
		*nsec = 0;
		release;
	}

	// Note that nseconds is updated in tservice; thus, we need no lock
	// to ensure its consistency between this reading and actual ldelay

	// Seconds of this minute already gone
	ns = (word) seconds () & 0x3f;

	// Add residual seconds from the requested delay, such that we are
	// left with full minutes
	ns += ((word) *nsec & 63);

	// Make the actual delay in minutes
	nm = *nsec >> 6;

	if (ns >= 64) {
		ns -= 64;
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
