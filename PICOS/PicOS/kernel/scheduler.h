/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
{
	// Set the return context for release
	SET_RELEASE_POINT;
Redo:
	// Catch up with time
	update_n_wake (MAX_WORD, NO);

	// Run the first ready process
	for_all_tasks (__pi_curr) {
		if (!waiting (__pi_curr)) {
			// Entry used and process ready
			(__pi_curr->code) (tstate (__pi_curr));
			goto Redo;
		}
	}

	// No process is ready

#if SPIN_WHEN_HALTED
	// Keep spinning the CPU (this exotic feature was requested for GENESIS)
#if ENTROPY_COLLECTION
	entropy++;
#endif

#else	/* NOT SPIN_WHEN_HALTED */

	__SLEEP;

#endif	/* SPIN_WHEN_HALTED */

	goto Redo;
}
