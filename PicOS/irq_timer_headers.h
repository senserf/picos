#ifndef __pg_irq_timer_headers_h
#define __pg_irq_timer_headers_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Roome for headers for intrinsic extra code for timer interrupt
 */

#if RADIO_INTERRUPTS
#include "irq_timer_headers_radio.h"
#endif

#if CC1100
#include "irq_timer_headers_cc1100.h"
#endif

#endif
