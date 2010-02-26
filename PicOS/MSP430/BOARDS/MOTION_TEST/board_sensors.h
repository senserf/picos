//
// This file is normally part of board_pins.h. I am putting here the sensor
// related definitions, which will be used primarily for testing
//

#include "analog_sensor.h"
#include "sensors.h"

#undef	PIN_DEFAULT_P5DIR
// Vref generator (Zener): P5.4; Probe on: P5.7, GPS on P5.6
#define	PIN_DEFAULT_P5DIR	0xD0
// Voltage probe switch: high == off
#define	PIN_DEFAULT_P5OUT	0x80

#define	PIN_DEFAULT_P6SEL	0xFF	// ADC

// ============================================================================

#define	ANA_SEN_SHT		4	// For all analog sensors
#define	ANA_SEN_ISI		0
#define	ANA_SEN_NSA		4
#define	ANA_SEN_URE		SREF_VEREF_AVSS
#define	ANA_SEN_ERE		0

#define	SENSOR_VEXT_PIN		1	// Sensor probe pins
#define	SENSOR_VINT_PIN		2
#define	SENSOR_VOLT_URE		SREF_AVCC_AVSS

#define	SENSOR_ANA0_PIN		0
#define	SENSOR_ANA1_PIN		3
#define	SENSOR_ANA2_PIN		4
#define	SENSOR_ANA3_PIN		5
#define	SENSOR_ANA4_PIN		6
#define	SENSOR_ANA5_PIN		7

#define	SENSOR_LIST { \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_ANA0_PIN,  \
				ANA_SEN_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_ANA1_PIN,  \
				ANA_SEN_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_ANA2_PIN,  \
				ANA_SEN_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_ANA3_PIN,  \
				ANA_SEN_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_ANA4_PIN,  \
				ANA_SEN_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_VEXT_PIN,  \
				SENSOR_VOLT_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
		ANALOG_SENSOR ( ANA_SEN_ISI,  \
				ANA_SEN_NSA,  \
				SENSOR_VINT_PIN,  \
				SENSOR_VOLT_URE,  \
				ANA_SEN_SHT,  \
				ANA_SEN_ERE), \
	}

// ============================================================================

#define	sensor_adc_prelude(p) 	CNOP
#define	sensor_adc_postlude(p) 	CNOP

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed

// ============================================================================

#define	BOARD_TIMER_SERVICE
#define	motion_value	(P2IN & 0x40)

//+++ "motion.c"
