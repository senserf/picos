#ifndef	__irq_ethernet_h
#define	__irq_ethernet_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

	/*
	 * select (2) not needed. If the interrupt is allowed to occur while
	 * something else is selected, it means that we are out to lunch.
	 */
if (tcv_interrupt) {
	soft_din;
	RISE_N_SHINE;
	/* Clear allocation interrupt */
	outw (IM_ALLOC_INT, INT_REG);
	/* Trigger the device events */
	if (zzd_data->flags & FLG_ENRCV)
		i_trigger (ETYPE_IO, devevent (ETHERNET, READ));
	if (zzd_data->flags & FLG_ENXMT)
		i_trigger (ETYPE_IO, devevent (ETHERNET, WRITE));
	zzd_data->flags = 0;
}

#endif
