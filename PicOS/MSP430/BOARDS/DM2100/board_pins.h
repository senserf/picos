/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ==================================================================== */
/*                            D M 2 1 0 0                               */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0xF0
#define	PIN_DEFAULT_P2DIR	0x22
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P4DIR	0xF0
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
	PIN_DEF (P1, 0),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P1, 2),	\
	PIN_DEF (P1, 3),	\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC
					// P6.0 is reserved for RSSI

#define	PIN_ADC_RSSI			0	// ADC needed for RSSI (P6.0)
#define	PULSE_MONITOR			PINS_MONITOR_INT (3, 4);
#define	MONITOR_PINS_SEND_INTERRUPTS	1

// ============================================================================
#if 0

#include "analog_sensor.h"
#include "sensors.h"

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR		\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

#endif
// ============================================================================
