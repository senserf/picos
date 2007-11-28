/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */

#if 0

// Obsolete

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

#endif	/* OBSOLETE */

// ============================================================================

interrupt (PORT2_VECTOR) p2_int () {

#define	P2_INTERRUPT_SERVICE
#include "board_pins_interrupts.h"
#undef	P2_INTERRUPT_SERVICE

	RTNI;
}
