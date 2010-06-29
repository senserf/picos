/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "cc1100.h"

// This is only compiled when CC430 is selected

#ifndef	__CC430__
#error "S: irq_cc430_rf.c can only be compiled with __CC430__!"
#endif

interrupt (CC1101_VECTOR) irq_cc430_rf () {

#include "irq_cc1100.h"

}
