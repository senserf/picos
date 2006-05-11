/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
/*
 * This code handles all interrupts triggered by P2 pins
 */
#include	"pins.h"
#include	"pinopts.h"

#if	DM2200
#include	"dm2200.h"
#endif

interrupt (PORT2_VECTOR) p2_int () {

#if	DM2200
#include	"irq_dm2200_rcv.h"
#endif

#if	TARGET_BOARD == BOARD_DM2100
#include	"irq_pins.h"
#endif

#if	TARGET_BOARD == BOARD_VERSA2
#include	"irq_pins.h"
#endif

// Room for more functions

}
