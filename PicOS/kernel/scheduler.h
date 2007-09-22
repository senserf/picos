/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
{
#if SCHED_PRIO
	static int       maxprio;
	static pcb_t	*maxpriocurrent;
#endif

#ifdef	MONITOR_PIN_SCHED
	_PVS (MONITOR_PIN_SCHED, 1);
#endif
	/* Set the return context for release */
	SET_RELEASE_POINT;
Redo:
	/* Timer service */
	zzz_tservice ();

#if SCHED_PRIO

	maxprio = MAX_PRIO;

	for_all_tasks (zz_curr) {
		if (zz_curr->code == NULL)
			// PCB unused
			continue;
		if (!waiting (zz_curr)) {
		        if (zz_curr->prio < maxprio) {
			  	maxprio = zz_curr->prio;
			  	maxpriocurrent = zz_curr;
			}
		}
		if (twaiting (zz_curr) && zz_curr->Timer == 0) {
			zz_curr->Status &= 0xfff0;
			if (zz_curr->prio < maxprio) {
				maxprio = zz_curr->prio;
				maxpriocurrent = zz_curr;
			}
		}
	}

	if (maxprio != MAX_PRIO) {
		zz_curr = maxpriocurrent;
#ifdef	MONITOR_PIN_SCHED
		_PVS (MONITOR_PIN_SCHED, 0);
#endif
		(zz_curr->code) (tstate (zz_curr), zz_curr->data);
		goto Redo;
	} else {

#else	/* NO PRIORITIES */

	for_all_tasks (zz_curr) {
		if (zz_curr->code == NULL)
			// PCB unused
			continue;
		if (!waiting (zz_curr)) {
#ifdef	MONITOR_PIN_SCHED
			_PVS (MONITOR_PIN_SCHED, 0);
#endif
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
			goto Redo;
		}
		if (twaiting (zz_curr) && zz_curr->Timer == 0) {
			zz_curr->Status &= 0xfff0;
#ifdef	MONITOR_PIN_SCHED
			_PVS (MONITOR_PIN_SCHED, 0);
#endif
			(zz_curr->code) (tstate (zz_curr), zz_curr->data);
			goto Redo;
		}
	}

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

#if SCHED_PRIO
    }
#endif
}
