/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2016                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// ============================================================================
// P1.0 NC
// P1.1 NC
// P1.2 NC
// P1.3 NC
// P1.4 NC
// P1.5 UARTRX
// P1.6 UARTTX
// P1.7 Active High RS485 switch; not sure how it should be set, we shall make
// a macro available for that
#define	PIN_DEFAULT_P1SEL	0x60
#define	PIN_DEFAULT_P1DIR	0xDF
#define	PIN_DEFAULT_P1OUT	0x00

#define	rs485_set_active(d)	do { \
				    if (d) \
						_BIS (P1OUT,0x80) \
					else \
						_BIC (P1OUT,0x80); \
				} while (0)

// ============================================================================
// P2.0 NC
// P2.1 NC
// P2.2 NC
// P2.3 NC
// P2.4 NC
// P2.5 NC
// P2.6 NC
// P2.7 NC
#define	PIN_DEFAULT_P2SEL	0x00
#define	PIN_DEFAULT_P2DIR	0xFF
#define	PIN_DEFAULT_P2OUT	0x00

// ============================================================================
// Switches to GND, for now assume NC low
// P3.0 NC
// P3.1 NC
// P3.2 NC
// P3.3 NC
// P3.4 NC
// P3.5 NC
// P3.6 NC
// P3.7 NC
#define	PIN_DEFAULT_P3SEL	0x00
#define	PIN_DEFAULT_P3DIR	0xFF
#define	PIN_DEFAULT_P3OUT	0x00

// ============================================================================
// P4.0 NC GND
// P4.1 NC
// P4.2 NC
// P4.3 NC
// P4.4 NC
// P4.5 - LED (B) (active low)
// P4.6 NC
// P4.7 - LED (R) (active low)
#define	PIN_DEFAULT_P4SEL	0x00
#define	PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0xA0
// ============================================================================
// P5.0 NO CRYSTAL
// P5.1 NO CRYSTAL
// P5.2 NC
// P5.3 NC
// P5.4 NC Switch to GND
// P5.5 NC Switch to GND
// P5.6 NC Switch to GND
// P5.7 NC Switch to GND
#define	PIN_DEFAULT_P5SEL	0x00
#define	PIN_DEFAULT_P5DIR	0xFF
#define	PIN_DEFAULT_P5OUT	0x00
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
