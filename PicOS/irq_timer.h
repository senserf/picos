/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_irq_timer_h
#define __pg_irq_timer_h

/*
 * Here go various helpers to be executed at every timer interrupt
 */

#ifdef PULSE_MONITOR
#include "irq_timer_pins.h"
#endif

#if LEDS_BLINKING
#include "irq_timer_leds.h"
#endif

#ifdef BOARD_IRQ_TIMER
#include "board_irq_timer.h"
#endif

#endif
