/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Sensor connections:
//
//	SHA = P6.0	(analog input)
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2-6 nc
#define	PIN_DEFAULT_P3DIR	0xC9

// P5: EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xEB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x0E	// P4.7 == Veref selector (PAR/MOI)
#define	PIN_DEFAULT_P6DIR	0x00
#define	PIN_DEFAULT_P6SEL	0x07	// Select the ADC pins (P6.0-2)

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "analog_sensor.h"

#define	SEN_SHA_PIN	0	// SHA sensor = P6.0
#define	SEN_SHA_SHT	4	// Sample hold time indicator
#define	SEN_SHA_ISI	1	// Inter sample interval in ms
#define	SEN_SHA_NSA	64	// Number of samples to average
#define	SEN_SHA_URE	ADC_SREF_VVSS	// Used reference
#define	SEN_SHA_ERE	0	// Exported reference (none)

// Not needed
// #define	SENSOR_INITIALIZERS
// #define	SENSOR_DIGITAL
#define	SENSOR_ANALOG		// To make sure analog sensors are processed

#define	SENSOR_LIST { \
		ANALOG_SENSOR ( SEN_SHA_ISI,  \
				SEN_SHA_NSA,  \
				SEN_SHA_PIN,  \
				SEN_SHA_URE,  \
				SEN_SHA_SHT,  \
				SEN_SHA_ERE), \
		ANALOG_SENSOR ( SEN_SHA_ISI,  \
				SEN_SHA_NSA,  \
				SEN_SHA_PIN+1,  \
				SEN_SHA_URE,  \
				SEN_SHA_SHT,  \
				SEN_SHA_ERE), \
		ANALOG_SENSOR ( SEN_SHA_ISI,  \
				SEN_SHA_NSA,  \
				SEN_SHA_PIN+2,  \
				SEN_SHA_URE,  \
				SEN_SHA_SHT,  \
				SEN_SHA_ERE)  \
	}

// ============================================================================

#define	SENSOR_SHA	0
