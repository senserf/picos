/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// UART on 2,3, two buttons, 13, 14, polarity down
//
// RF control on 1 (low: 2.4GHz, high: sub 1GHz)
// RF switch on 30 (high -> on)
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
#define	IOCPORTS { \
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
			0), \
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
			0), \
		iocportconfig (IOID_13, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0), \
		iocportconfig (IOID_14, IOC_PORT_GPIO, \
			IOC_IOMODE_NORMAL 	| \
			IOC_NO_WAKE_UP		| \
			IOC_FALLING_EDGE	| \
			IOC_INT_DISABLE		| \
			IOC_IOPULL_UP		| \
			IOC_INPUT_ENABLE	| \
			IOC_HYST_DISABLE	| \
			IOC_SLEW_DISABLE	| \
			IOC_CURRENT_2MA		| \
			IOC_STRENGTH_AUTO	| \
			0), \
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
			0), \
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
			0), \
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
			0), \
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
			0), \
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
			0), \
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
			0), \
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
			0), \
	}

// ============================================================================

#define	RADIO_PINS_PREINIT	do { \
					GPIO_setOutputEnableDio (IOID_1 , 1); \
					GPIO_setOutputEnableDio (IOID_30, 1); \
					RADIO_PINS_OFF; \
				} while (0)

#define	RADIO_PINS_ON		do { \
					GPIO_setDio (IOID_30); \
					GPIO_setDio (IOID_1); \
				} while (0)

#define	RADIO_PINS_OFF		do { \
					GPIO_clearDio (IOID_30); \
					GPIO_clearDio (IOID_1); \
				} while (0)

// ============================================================================

#define	PIN_LIST { \
		PIN_DEF (IOID_4),  \
		PIN_DEF (IOID_5),  \
		PIN_DEF (IOID_8),  \
		PIN_DEF (IOID_9),  \
		PIN_DEF (IOID_10), \
		PIN_DEF (IOID_11), \
		PIN_DEF (IOID_12), \
		PIN_DEF (IOID_15), \
		PIN_DEF (IOID_16), \
		PIN_DEF (IOID_17), \
		PIN_DEF (IOID_18), \
		PIN_DEF (IOID_19), \
		PIN_DEF (IOID_20), \
		PIN_DEF (IOID_21), \
		PIN_DEF (IOID_22), \
		PIN_DEF (IOID_24), \
		PIN_DEF (IOID_29), \
	}

#define	PIN_MAX	17

#define	BUTTON_LIST	{ \
				BUTTON_DEF (IOID_13, 0), \
				BUTTON_DEF (IOID_14, 0), \
			}

#define	BUTTON_DEBOUNCE_DELAY	4
#define	BUTTON_PRESSED_LOW	1
// This is needed for CC1350, not needed for MSP430; should we get rid of them
// by some kind of better (dynamic) initialization? Code size is (probably) not
// an issue on CC1350.
#define	N_BUTTONS		2
#define	BUTTON_GPIOS		((1 << IOID_13) | (1 << IOID_14))

// ============================================================================

#define	BUTTON_A	0x0001
#define	BUTTON_B	0x0002
#define	BUTTON_C	0x0004
#define	BUTTON_D	0x0008

// Edge == 1 means reverse polarity, i.e., pressed low
#define	INPUT_PIN_LIST	{ \
				INPUT_PIN (IOID_25, 1), \
				INPUT_PIN (IOID_26, 1), \
				INPUT_PIN (IOID_27, 1), \
				INPUT_PIN (IOID_28, 1), \
			}

#define	N_PINLIST		4
#define	INPUT_PINLIST_GPIOS	((1 << IOID_25) | \
				 (1 << IOID_26) | \
				 (1 << IOID_27) | \
				 (1 << IOID_28))

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
		DIGITAL_SENSOR (0, NULL, pin_sensor_read),	\
}

//
// Notes:
//
//	Both RF control pins are down in the RF off state. Is this OK for
//	low power?
//
//	DIO4 and DIO5 have external pullups to Vdd (3.3K each)
//
//	DIO20 is nCS for external flash, pulled up 2.2K
//
