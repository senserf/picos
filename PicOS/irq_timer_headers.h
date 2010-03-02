#ifndef __pg_irq_timer_headers_h
#define __pg_irq_timer_headers_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Room for headers for intrinsic extra code for timer interrupt
 */

#if RADIO_INTERRUPTS
#include "irq_timer_headers_radio.h"
#endif

#if CC1100
#include "irq_timer_headers_cc1100.h"
#endif

#ifdef PULSE_MONITOR
#include "irq_timer_headers_pins.h"
#endif

#ifdef irmtn_signal
extern word irmtn_counter;
#endif

#endif
