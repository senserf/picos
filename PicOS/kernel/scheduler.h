/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
{
	/* Set the return context for release */
	SET_RELEASE_POINT;
Redo:

#ifdef	MONITOR_PIN_SCHED
	_PVS (MONITOR_PIN_SCHED, 1);
#endif
	/* Timer service */
	update_n_wake (MAX_UINT);

	for_all_tasks (zz_curr) {
		if (zz_curr->code == NULL)
			// PCB unused
			continue;
		if (!waiting (zz_curr)) {
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
			goto Redo;
		}
	}

#ifdef	MONITOR_PIN_SCHED
			_PVS (MONITOR_PIN_SCHED, 0);
#endif
	/* No process to run */

#if SPIN_WHEN_HALTED
	/* Keep spinning the CPU */
#if ENTROPY_COLLECTION
	entropy++;
#endif

#else	/* NOT SPIN_WHEN_HALTED */

	SLEEP;

#endif	/* SPIN_WHEN_HALTED */

	goto Redo;
}
