#ifndef	__irq_xemics_h
#define	__irq_xemics_h

/*
 * This is a bit messy - to be compatible with the other two radio boards.
 * It will be simplified some day, when this becomes the only board to stay.
 * Note that this function rightfully belongs to the driver, not here.
 */
if (int_radio) {
	RISE_N_SHINE;
	zzz_last_sense = 0;
	if (zzr_xwait && !(zzz_radiostat & 1)) {
		zzr_xwait -> Status = zzr_xstate << 4;
		zzr_xwait = NULL;
		cli_radio;
	}
}

#endif
