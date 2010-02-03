/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// Pin allocation for ez-chronos
// ============================================================================
//
// P1.5	ACCEL (CMA3000) MISO
// P1.6	ACCEL (CMA3000) MOSI-SDA
// P1.7	ACCEL (CMA3000) SCK-SCL

// P2.0 Button S2
// P2.1 Button M2
// P2.2 Button M1
// P2.3 Button BL
// P2.4 Button S1

// P2.5	ACCEL (CMA3000) INT

// P2.6 Pressure (SCP1000) DRDY

// P2.7 Buzzer

// PJ.0	ACCEL (CMA3000) Vdd
// PJ.1	ACCEL (CMA3000) CSB
// PJ.2	Pressure (CMA3000) MOSI-SDA
// PJ.2	Pressure (CMA3000) SCK

// Enable LF crystal (0-1) and COM for LCD (5-7)
#define	PIN_DEFAULT_P5SEL	0xE3
#define	PIN_DEFAULT_P5DIR	0xE0

// For buzzer and buttons
#define	PIN_DEFAULT_P2OUT	0x00
#define	PIN_DEFAULT_P2DIR	0x80
// Pull-downs for the buttons
#define	PIN_DEFAULT_P2REN	0x1F

#define	PIN_DEFAULT_P1DIR	0x1F
#define	PIN_DEFAULT_P1OUT	0x1F

// J0, J1 are power switches for ACC i PRE, J2 is In/Out for PRE
#define	PIN_DEFAULT_PJDIR	0xFB
#define	PIN_DEFAULT_PJOUT	0xF8

// Portmapper
#define	PIN_PORTMAP	{ 	portmap_entry (P2MAP0, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_TA1CCR0A), \
			  	portmap_entry (P1MAP0, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_UCA0SOMI, \
						PM_UCA0SIMO, \
						PM_UCA0CLK)  \
			}

// ============================================================================

//+++ "p1irq.c"

REQUEST_EXTERNAL (p1irq);

#define	BUTTON_LIST 	{ \
				BUTTON_DEF (2, 0x04, 0), \
				BUTTON_DEF (2, 0x02, 0), \
				BUTTON_DEF (2, 0x10, 0), \
				BUTTON_DEF (2, 0x01, 0), \
				BUTTON_DEF (2, 0x08, 0), \
			}

#define	BUTTON_M1	0
#define	BUTTON_M2	1
#define	BUTTON_S1	2
#define	BUTTON_S2	3
#define	BUTTON_BL	4

#define	P2_PINS_INTERRUPT_MASK	0x1F
#define	BUTTON_DEBOUNCE_DELAY	64


// ============================================================================

#include "board_sensors.h"
