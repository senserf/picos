#ifndef __pg_irq_timer_h
#define __pg_irq_timer_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Here go various helpers to be executed at every timer interrupt
 */

#if RADIO_INTERRUPTS
#include "irq_timer_radio.h"
#endif

#if CC1100
#include "irq_timer_cc1100.h"
#endif

#endif
