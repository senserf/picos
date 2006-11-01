/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */
#include	"pins.h"
#include	"pinopts.h"

#if	DM2200
#include	"dm2200.h"
#endif

interrupt (PORT2_VECTOR) p2_int () {

#if	DM2200
#include	"irq_dm2200_rcv.h"
#endif

#ifdef	MONITOR_PINS_SEND_INTERRUPTS
#if	MONITOR_PINS_SEND_INTERRUPTS
#include	"irq_pins.h"
#endif
#endif

// Room for more functions

}
