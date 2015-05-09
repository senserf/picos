/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2013                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0	BUTTON 4 (on high)
// P1.1 BUTTON 2 (on high)
// P1.2 BUTTON 0
// P1.3 BUTTON 5
// P1.4 SWITCH S4 - 3
// P1.5 UARTRX
// P1.6 UARTTX
// P1.7 SWITCH S4 - 2
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0x40
#define	PIN_DEFAULT_P1OUT	0x9F
#define	PIN_DEFAULT_P1REN	0x0F
// ============================================================================
// P2.0 BUTTON 3
// P2.1 BUTTON 1
// P2.2 NC
// P2.3 NC
// P2.4 NC
// P2.5 NC
// P2.6 NC
// P2.7 NC
#define	PIN_DEFAULT_P2SEL	0x00
#define	PIN_DEFAULT_P2DIR	0xFC
#define	PIN_DEFAULT_P2OUT	0x03
#define	PIN_DEFAULT_P2REN	0x03
// ============================================================================
// P3.0 SWITCH S1 - 1
// P3.1 SWITCH S1 - 2
// P3.2 SWITCH S1 - 4
// P3.3 SWITCH S1 - 8
// P3.4 SWITCH S2 - 1
// P3.5 SWITCH S2 - 2
// P3.6 SWITCH S2 - 4
// P3.7 SWITCH S2 - 8
#define	PIN_DEFAULT_P3SEL	0x00
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P3OUT	0xFF
// ============================================================================
// P4.0 NC
// P4.1 NC
// P4.2 NC
// P4.3 NC
// P4.4 OUT LED (active low)
// P4.5 - LED (R) (active low)
// P4.6 - LED (G) (active low)
// P4.7 - LED (B) (active low)
#define	PIN_DEFAULT_P4SEL	0x00
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0xF0
// ============================================================================
// P5.0 SWITCH S6 - 5
// P5.1 SWITCH S6 - 4
// P5.2 SWITCH S6 - 3
// P5.3 SWITCH S6 - 2
// P5.4 SWITCH S6 - 1
// P5.5 SWITCH S6 - 0
// P5.6 SWITCH S4 - 1
// P5.7 NC
#define	PIN_DEFAULT_P5SEL	0x00
#define	PIN_DEFAULT_P5DIR	0x80
#define	PIN_DEFAULT_P5OUT	0x7F
// ============================================================================

#define	BUTTON_LIST 		{	\
					BUTTON_DEF (1, 0x04, 0), \
					BUTTON_DEF (2, 0x02, 0), \
					BUTTON_DEF (1, 0x02, 0), \
					BUTTON_DEF (2, 0x01, 0), \
					BUTTON_DEF (1, 0x01, 0), \
					BUTTON_DEF (1, 0x08, 0), \
				}

#define	BUTTON_PIN_P1_IRQ	0x0F
#define	BUTTON_PIN_P2_IRQ	0x03

#define	BUTTON_DEBOUNCE_DELAY	4
#define	BUTTON_PRESSED_LOW	1

// ============================================================================

#define	PIN_MAX			0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0x00

// ============================================================================

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR				\
	}


#define	SENSOR_ANALOG

#define	N_HIDDEN_SENSORS	2

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()

#include "switches.h"
