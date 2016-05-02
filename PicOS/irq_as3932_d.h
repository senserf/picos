//+++ "as3932irq.c"
//
// IRQ service for the AS3932 LF wake receiver (the DATA signal)
//
if (as3932_int_d && (as3932_status & AS3932_STATUS_RUNNING)) {

	register word del;

	// How much time from last transition
	del = as3932_clock;

	// Start the timer for the next one
	as3932_start_timer;

	if (del <= AS3932_SHORT_BIT) {
		// This is a short transition
		if (as3932_status & AS3932_STATUS_BOUNDARY) {
			// Short transition from a boundary, i.e., a bit
			if (as3932_addbit ()) {
as3932_l_done:
#if AS3932_CRCVALUE >= 0
				for (del = 0; del < AS3932_NBYTES; del++)
					as3932_bytes [AS3932_NBYTES] ^=
						as3932_bytes [del];
				if (as3932_bytes [del] != AS3932_CRCVALUE) {
					as3932_clearall (1);
					goto as3932_l_end;
				}
#endif
				as3932_clearall (0);

				_BIS (as3932_status, AS3932_STATUS_EVENT);
				if (as3932_status & AS3932_STATUS_WAIT) {
					_BIC (as3932_status,
						AS3932_STATUS_WAIT);
					i_trigger ((word)(&as3932_status));
					RISE_N_SHINE;
				}
				goto as3932_l_end;
			}

			// We are inside a bit
			_BIC (as3932_status, AS3932_STATUS_BOUNDARY);
			goto as3932_l_continue;
		}

		// Short transition from a bit, now get to a boundary
as3932_l_boundary:
		_BIS (as3932_status, AS3932_STATUS_BOUNDARY);
		// Keep going
		goto as3932_l_continue;
	}

	// Long transition, we are necessarily inside a bit
	if (as3932_addbit ())
		goto as3932_l_done;

	// Boundary status doesn't change

as3932_l_continue:

	// Flip the DATA bit
	_BIX (as3932_status, AS3932_STATUS_DATA);
	as3932_enable_d;

as3932_l_end:

	CNOP;
}
