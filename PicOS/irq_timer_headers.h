#ifndef __pg_irq_timer_headers_h
#define __pg_irq_timer_headers_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Room for headers for intrinsic extra code for timer interrupt
 */

#ifdef PULSE_MONITOR
#include "irq_timer_headers_pins.h"
#endif

// The board defines IR motion sensor
#ifdef irmtn_active
#include "irq_timer_headers_irmtn.h"
#endif

#endif
