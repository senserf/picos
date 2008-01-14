/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"
/*
 * This code handles all interrupts triggered by P1 pins
 */

#if 0

// Note: this is obsolete and should be handled on a per-board basis

#if		CC1000
#include	"cc1000.h"
#endif

#if		RF24G
#include	"rf24g.h"
#endif

#if		RF24L01
#include	"rf24l01.h"
#endif

interrupt (PORT1_VECTOR) p1irq () {

#if		CC1000
#include 	"irq_cc1000.h"
#endif

#if		RF24G
#include	"irq_rf24g.h"
#endif

#if		RF24L01
#include	"irq_rf24l01.h"
#endif

#endif	/* OBSOLETE */

// ============================================================================


interrupt (PORT1_VECTOR) p1irq () {

#define	P1_INTERRUPT_SERVICE
#include "board_pins_interrupts.h"
#undef	P1_INTERRUPT_SERVICE

	RTNI;
}
