/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "cc1100.h"

// This is only compiled when CC430 is selected

#ifndef	__CC430__
#error "S: irq_cc430_rf.c can only be compiled with __CC430__!"
#endif

interrupt (CC1101_VECTOR) irq_cc430_rf () {

	RF1AIE = 0;
	// Make sure FLL is on while we are doing this (just for a test)
	_BIC_SR (SCG0 + SCG1);

#if RADIO_WOR_MODE

	// Emulate what CC110x does automatically (without notifying the
	// driver FSM)
	if ((RF1AIFG & IRQ_EVT0) && cc1100_worstate == 1) {
		// Event 0, start RX
		cc1100_worstate = 2;
		// Make sure to clear the flag before doing SRX (guess why)
		RF1AIFG = 0;
		erx_enable_int;
		//
		// This really (I mean REALLY) sucks. I have to use a spin-loop
		// (this is what the SRX strobe amounts to) in an interrupt
		// service routine (of order 1msec) to make the stupid chip
		// happy. The manual says that the CPU cannot go to the low
		// power mode for over 800us when transiting from SLEEP (also
		// WOR) to any active state. Apparently, the manual is right as
		// otherwise the chip tends to hang in RX.
		//
		cc1100_strobe (CCxxx0_SRX);
		goto Rtn;
	}

	if ((RF1AIFG & IRQ_RXTM) && cc1100_worstate == 2) {
		// RX timeout in WOR
		RF1AIFG = 0;
		wor_enable_int;
		cc1100_strobe (CCxxx0_SWOR);
		cc1100_worstate = 1;
		goto Rtn;
	}

	if ((RF1AIFG & (IRQ_PQTR | IRQ_RCPT)) && cc1100_worstate != 1) {
#else
	if (RF1AIFG & IRQ_RCPT) {
#endif
    		p_trigger (__cc1100_v_drvprcs, __cc1100_v_qevent);
    		RISE_N_SHINE;
	}

	RF1AIFG = 0;
Rtn:
	RTNI;
}
