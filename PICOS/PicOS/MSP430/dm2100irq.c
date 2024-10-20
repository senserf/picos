/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "dm2100.h"

interrupt (TIMERA0_VECTOR) dm2100_st_int () {

// Signal strobe (transmission and reception)
#include "irq_dm2100.h"

	RTNI;

}

// Receiver timeout
interrupt (TIMERA1_VECTOR) dm2100_tm_int () {

	if (TAIV == 2) {
		// CCR1 comparator; ignore everything else; on CCR1 comparator,
		// trigger CCR0 interrupt to complete reception
		_BIS (TACCTL0, CCIFG);
		// Disable further timeout interrupts
		disable_rcv_timeout;
	}
	RTNI;
}
