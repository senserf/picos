/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__second_reset_h
#define	__second_reset_h
//
// This is the standard code for reset/erase on a button (checked at second
// intervals)
//

#ifdef	EMERGENCY_RESET_CONDITION

	if (EMERGENCY_RESET_CONDITION) {
		watchdog_stop ();
		cli;

#ifdef	EMERGENCY_RESET_ACTION
		EMERGENCY_RESET_ACTION;
#endif
		reset ();
	}
#endif

#endif
