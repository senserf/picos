/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//+++ "as3932irq.c"
//
// IRQ service for the AS3932 LF wake receiver (the WAKE signal)
//
if (as3932_int_w && (as3932_status & AS3932_STATUS_ON)) {

	// Disable until the event is cleared
	as3932_disable_w;
	as3932_clear_w;

	// Configure the timer for receiving data
	as3932_start_timer;
	as3932_tim_sti;

	// While I hate to do something like this in an interrupt, this is just
	// a few machine instructions, and there is no simpler way to resolve
	// the apparent race. We have to make sure DAT has stabilized.
	udelay (10);

	if (!as3932_data)
		// Make it look like the previous state was opposite (WAKE
		// seems to occur at the bit transition)
		_BIS (as3932_status, AS3932_STATUS_DATA);

	// Add in the first bit
	as3932_addbit ();

	// Flip DATA
	_BIX (as3932_status, AS3932_STATUS_DATA);

	// Mark as running
	_BIS (as3932_status, AS3932_STATUS_RUNNING);

	as3932_enable_d;
}
