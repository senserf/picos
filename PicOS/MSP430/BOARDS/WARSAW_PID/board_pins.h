/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2020                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization

#if CC1100

// ============================================================================
// Radio is on P1
// ============================================================================
// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6,7 General, unused by default
#define	PIN_DEFAULT_P1DIR	0xE3

#else

#define	PIN_DEFAULT_P1DIR	0xFF

#endif

// ============================================================================

// 0, 1, 7 hang loose, 4 = soft reset button, must be IN
// 2, 3, 5, 6 general, unused by default
#define	PIN_DEFAULT_P2DIR	0xEF

#define	SOFT_RESET_BUTTON_PRESSED	((P2IN & 0x10) == 0)

// ============================================================================

// 0 doubles with RXD1 (as CTS, and is in the way, so must be input)
// 1 doubles with TXD0 (and is in the way, so must be input)
// 2 doubles with RXD0 (and is in the way, so must be input)
// 3 doubles with TXD1 ....
// 4 is TXD0 (output)
// 5 is RXD0 (input)
// 6 is TXD1
// 7 is RXD1
#define	PIN_DEFAULT_P3DIR	0x50

// 1, 2, 3 = LEDs, 0, 4-7 = general unused by default
#define PIN_DEFAULT_P4DIR	0xFF
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off (high) by default

// ============================================================================
// P5 used by EEPROM/SDCARD
// ============================================================================
// 0 = EEPROM CS OUT
// 1 = STORAGE SI OUT
// 2 = STORAGE SO IN
// 3 = STORAGE SCK OUT
// 4 = SD card CS OUT
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default both CS, CLK are up

#if ADC_SAMPLER

#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF	(P5, 4),	\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		0	// Number of available analog pins
#define	PIN_DAC_PINS		0

#else	/* NO SAMPLER */

#define	PIN_DEFAULT_P6DIR	0xFF

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
	PIN_DEF	(P6, 7),	\
	PIN_DEF	(P1, 6),	\
	PIN_DEF	(P1, 7),	\
	PIN_DEF	(P2, 2),	\
	PIN_DEF	(P2, 3),	\
	PIN_DEF	(P2, 5),	\
	PIN_DEF	(P2, 6),	\
	PIN_DEF	(P4, 0),	\
	PIN_DEF	(P4, 4),	\
	PIN_DEF	(P4, 5),	\
	PIN_DEF	(P4, 6),	\
	PIN_DEF	(P4, 7),	\
	PIN_DEF (P5, 4)		\
}

#define	PIN_MAX			20	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7

#endif /* SAMPLER or no SAMPLER */

// ============================================================================
#if 1

#include "analog_sensor.h"
#include "sensors.h"
#include "analog_actuator.h"
#include "actuators.h"

#define	PLANT_SENSOR_PIN	0	// P6.0
#define	PLANT_SENSOR_SHT	4	// Hold time
#define	PLANT_SENSOR_ISI	1	// Inter-sample interval
#define	PLANT_SENSOR_NSA	4	// Number of samples to average
#define	PLANT_SENSOR_URE	SREF_AVCC_AVSS	// Voltage reference

#define	PLANT_ACTUATOR_PIN	0	// P6.6
#define	PLANT_ACTUATOR_TIM	4	// Time
#define	PLANT_ACTUATOR_VOL	0	// Full scale, 3x ref voltage
#define	PLANT_ACTUATOR_REF	0	// Internal
#define	PLANT_ACTUATOR_HRF	1	// 2.5 V ref

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,		\
		INTERNAL_VOLTAGE_SENSOR,		\
		ANALOG_SENSOR (	PLANT_SENSOR_ISI,	\
				PLANT_SENSOR_NSA,	\
				PLANT_SENSOR_PIN,	\
				PLANT_SENSOR_URE,	\
				PLANT_SENSOR_SHT,	\
				0)			\
	}
#define	SENSOR_ANALOG
#define	N_HIDDEN_SENSORS	2

#define	ACTUATOR_LIST { \
		ANALOG_ACTUATOR (	PLANT_ACTUATOR_TIM,	\
					PLANT_ACTUATOR_PIN,	\
					PLANT_ACTUATOR_VOL,	\
					PLANT_ACTUATOR_REF,	\
					PLANT_ACTUATOR_HRF	\
		)						\
	}
#endif
#define	ACTUATOR_ANALOG
// ============================================================================

