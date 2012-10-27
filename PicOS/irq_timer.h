#ifndef __pg_irq_timer_h
#define __pg_irq_timer_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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
