/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ==================================================================== */
/*                           G E N E S I S                              */
/* ==================================================================== */

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80
#define	PIN_DEFAULT_P3DIR	0xCF
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P6DIR	0x00

#define	SOFT_RESET_BUTTON_PRESSED	((P6IN & 0x01) == 0)

#if LEDS_DRIVER
// If selected by the application (for experiment), some pins have
// to go
#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_RESERVED,		\
	PIN_RESERVED,		\
	PIN_RESERVED,		\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
}

#else	/* LEDS_DRIVER */

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
}

#endif	/* LEDS_DRIVER */

#define	PIN_MAX			8	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC

// No ADC for RSSI, which is collected internally by the RF chip

#define	PULSE_MONITOR		PINS_MONITOR (P6, 1, P6, 2)

/ ============================================================================
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

