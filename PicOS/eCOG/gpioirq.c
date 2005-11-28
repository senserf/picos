/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include <ecog.h>
#include <ecog1.h>
#include "kernel.h"

#if	CC1000
#include "cc1000.h"
#endif

#if	ETHERNET_DRIVER
#include "ethernet.h"
#endif

#if	RADIO_TYPE == RADIO_XEMICS
#include "radio.h"
#endif

void __irq_entry gpio_int (void) {

/*
 * There is no way to do it right, e.g., to use a jump table filled by
 * registering driver-specific GPIO interrupt routines, because the C
 * compiler won't let you handle a pointer to an __irq_entry function.
 * At least I've tried.
 */

#if	CC1000
#include "irq_cc1000.h"
#endif

#if	ETHERNET_DRIVER
#include "irq_ethernet.h"
#endif

#if	RADIO_TYPE == RADIO_XEMICS
#include "irq_xemics.h"
#endif

}
