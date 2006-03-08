#ifndef __pg_dm2200_p2_h
#define __pg_dm2200_p2_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * P2 interrupts for DM2200
 */
	if (rcv_interrupt) {

#include "irq_dm2200_rcv.h"

	}

	if (pin_interrupt) {

#include "irq_dm2200_pins.h"

	}
#endif
