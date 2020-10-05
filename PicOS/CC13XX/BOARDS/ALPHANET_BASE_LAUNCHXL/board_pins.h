/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

// UART on 2,3, two buttons, 13, 14, polarity down, shutdown wakeup on low
//
// LEDs on 6, 7
//
// RF control on 1 (low: 2.4GHz, high: sub 1GHz)
// RF switch on 30 (high -> on)
//
// External flash:
//
//	 8	SO
//	 9	SI
//	10	SCLK
//	20	CS (pulled up on external resistor)
//
// Note: not sure whether the hysteresis option wouldn't help to debounce
// buttons (the manual doesn't bother to mention how it exactly works); same
// about slew (no clue what it is)
//
// Button interrupts are enabled explicitly in the driver, so they should be
// disabled in the definitions below
//
// ADC test on 23 as AUXIO7
//
// Pin sensor on 25, 26, 27, 28 polarity low (pulled to ground)
//
//

// Pin defs: number, function, standard options, set as output, output value;
// e.g., IOID_6 is set as output and to 0 (low); note that the value is also
// preset for an input (or open) pin (with the "set as output" flag equal 0)
// and it will become valid when the pin is set to output later; for example,
// IOID_20 is used as CS for the flash chip and initially set as open drain
// (an external resistor pulling it up), and it pulls the the CS down when set
// as output (see board_storage.h)
#define	IOCPORTS { \
		iocportconfig (IOID_6, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_7, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_3, IOC_PORT_MCU_UART0_TX, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_2, IOC_PORT_MCU_UART0_RX, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_13, IOC_PORT_GPIO, \
			/* Button 0, also shutdown wakeup */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			IOC_WAKE_ON_LOW		| \
			0, 0, 0), \
		iocportconfig (IOID_14, IOC_PORT_GPIO, \
			/* Button 1 */ \
			IOC_IOMODE_NORMAL 	| \
			IOC_WAKE_ON_LOW		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_23, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			IOC_PORT_AUX_IO		| \
			0, 0, 0), \
		iocportconfig (IOID_25, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_ENABLE		| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_26, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_ENABLE		| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_27, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_ENABLE		| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_28, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_ENABLE		| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_1, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_MAX	| \
			0, 1, 0), \
		iocportconfig (IOID_30, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_8MA		| \
			IOC_STRENGTH_MAX	| \
			0, 1, 0), \
		iocportconfig (IOID_8, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_DOWN		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
		iocportconfig (IOID_9, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_10, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 1, 0), \
		iocportconfig (IOID_20, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_NO_EDGE		| \
			IOC_INT_DISABLE		| \
			IOC_NO_IOPULL		| \
			IOC_INPUT_DISABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0, 0, 0), \
	}

// ============================================================================

#define	RADIO_PINS_ON		do { \
					GPIO_setDio (IOID_30); \
					GPIO_setDio (IOID_1); \
				} while (0)

#define	RADIO_PINS_OFF		do { \
					GPIO_clearDio (IOID_30); \
					GPIO_clearDio (IOID_1); \
				} while (0)

// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_EVENTS
// #define SENSOR_INITIALIZERS
#define	N_HIDDEN_SENSORS	2

#include "analog_sensor.h"
#include "sensors.h"
#include "pin_sensor.h"

#define	SENS_ISI	2	// Inter-sample interval
#define	SENS_NSA	8	// Number of samples
// AUXIO7 maps into pin 23
#define	SENS_PIN	ADC_COMPB_IN_AUXIO7
// This is 4.3V, the other option is AUXADC_REF_VDDS_REL (VDDS)
#define	SENS_REF	AUXADC_REF_FIXED
// Sample holding (sampling) time, the options are:
//	AUXADC_SAMPLE_TIME_2P7_US
//	AUXADC_SAMPLE_TIME_5P3_US
//	AUXADC_SAMPLE_TIME_10P6_US
//	AUXADC_SAMPLE_TIME_21P3_US
//	AUXADC_SAMPLE_TIME_42P6_US
//	AUXADC_SAMPLE_TIME_85P3_US
//	AUXADC_SAMPLE_TIME_170_US
//	AUXADC_SAMPLE_TIME_341_US
//	AUXADC_SAMPLE_TIME_682_US
//	AUXADC_SAMPLE_TIME_1P37_MS
//	AUXADC_SAMPLE_TIME_2P73_MS
//	AUXADC_SAMPLE_TIME_5P46_MS
//	AUXADC_SAMPLE_TIME_10P9_MS
#define	SENS_SHT	AUXADC_SAMPLE_TIME_1P37_MS

#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,			\
		INTERNAL_VOLTAGE_SENSOR,			\
		ANALOG_SENSOR (	SENS_ISI,			\
				SENS_NSA,			\
				SENS_PIN,			\
				SENS_REF,			\
				SENS_SHT),			\
}
