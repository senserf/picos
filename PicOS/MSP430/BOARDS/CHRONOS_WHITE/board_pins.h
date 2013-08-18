/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// ============================================================================
// Pin allocation for ez-chronos (white 1.1)
// ============================================================================
//
// P1.5	ACCEL (BMA250) SDO
// P1.6	ACCEL (BMA250) SDX
// P1.7	ACCEL (BMA250) SCK

// P2.0 Button S2
// P2.1 Button M2
// P2.2 Button M1
// P2.3 Button BL
// P2.4 Button S1

// P2.5	ACCEL (BMA250) INT
// P2.6 Pressure (BMP085) DRDY/EOC
// P2.7 Buzzer

// PJ.0	ACCEL (BMA250) Vdd
// PJ.1	ACCEL (BMA250) CSB
// PJ.2	Pressure (BMP085) SDA
// PJ.3	Pressure (BMP085) SCL

// Enable LF crystal (0-1) and COM for LCD (5-7)
#define	PIN_DEFAULT_P5SEL	0xE3
#define	PIN_DEFAULT_P5DIR	0xE0

// For buzzer and buttons (P2.6 is RDY for SCP1000)
#define	PIN_DEFAULT_P2OUT	0x00
#define	PIN_DEFAULT_P2DIR	0x80	// BMA250 INT forced down on power down
// Pull-downs for the buttons
#define	PIN_DEFAULT_P2REN	0x1F

// 5-7 == ACC SDI, SDO, Clock (all initially out, set to zero, ACC switched off)
#define	PIN_DEFAULT_P1DIR	0xFF
#define	PIN_DEFAULT_P1OUT	0x1F
// Pulldown for acc SDI (when switched to input)
// ### this was apparently needed for power savings, to make sure that the pin
// ### doesn't float when the chip is powered down; the pulldown is disabled
// ### when the chip is selected
#define	PIN_DEFAULT_P1REN	0x20

// J0, J1 control the ACC, J2 is In/Out for PRE, J3 is SCK for PRE; J2 is used
// in open drain mode, with the default setting to IN, output set low
#define	PIN_DEFAULT_PJDIR	0xFF
#define	PIN_DEFAULT_PJOUT	0xFC	// BMA250 CSB down on power down

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

//+++ "p2irq.c"
REQUEST_EXTERNAL (p2irq);

#ifdef	BUTTONS

// ============================================================================
// Debounced buttons via a special driver =====================================
// ============================================================================

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

#define	BUTTON_PIN_P2_IRQ		0x1F

#else

// ============================================================================
// Buttons implemented as a pin sensor ========================================
// ============================================================================

#define	INPUT_PIN_LIST	{ \
				INPUT_PIN (P2, 2, 0), \
				INPUT_PIN (P2, 1, 0), \
				INPUT_PIN (P2, 4, 0), \
				INPUT_PIN (P2, 0, 0), \
				INPUT_PIN (P2, 3, 0)  \
			}

#define	BUTTON_M1       0x0001
#define	BUTTON_M2       0x0002
#define	BUTTON_S1       0x0004
#define	BUTTON_S2       0x0008
#define	BUTTON_BL       0x0010

#define	INPUT_PIN_P2_IRQ	0x1F

#endif
// ============================================================================
// ============================================================================
// ============================================================================

#include "board_sensors.h"

#include "board_buzzer.h"

#include "board_rtc.h"

#define	EXTRA_INITIALIZERS	do { \
					__pi_rtc_init (); \
					buzzer_init (); \
				} while (0)

// Current measurements:
//
//				This is supposedly RMS, looks bogus ----|
//									V
//
// Radio OFF, sensors OFF, display ON, PD mode, 1 sec wakeup: 8.6uA / 16.6uA
// ------------------------------- OFF ---------------------: 3.4uA / 18.5uA
// Radio OFF, display on, CPU idle:			      433uA / 433uA
// ---------------------- CPU loop:			      3.2mA / 3.2mA
// ------ ON ---------------------:			      16.5mA / 64mA ?
//

