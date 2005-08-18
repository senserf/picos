/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

#if		CHIPCON
#include	"chipcon.h"
#endif

interrupt (PORT1_VECTOR) p1_int () {

#if	CHIPCON
#include "irq_chipcon.h"
#endif

// Here room for more functions

}
