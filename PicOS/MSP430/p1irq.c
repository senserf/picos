/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

#if		CC1000
#include	"cc1000.h"
#endif

#if		RF24G
#include	"rf24g.h"
#endif

#if		RF24L01
#include	"rf24l01.h"
#endif

#if		CC1100
#include	"cc1100.h"
#endif

interrupt (PORT1_VECTOR) p1_int () {

#if		CC1000
#include 	"irq_cc1000.h"
#endif

#if		RF24G
#include	"irq_rf24g.h"
#endif

#if		RF24L01
#include	"irq_rf24l01.h"
#endif

#if		CC1100
#include	"irq_cc1100.h"
#endif

// Here room for more functions

}
