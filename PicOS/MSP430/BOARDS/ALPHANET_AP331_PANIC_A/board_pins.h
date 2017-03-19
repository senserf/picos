/* ========================================================================= */
/* Copyright (C) Olsonet Communications, 2017                                */
/* All rights reserved.                                                      */
/* ========================================================================= */

// This is AP331 with an external battery voltage sensor (before the regulator)
// and a charger voltage sensor

// ============================================================================
// P1.0	BUTTON, on low
// P1.4 DATA from loop detector (crosswired with BMA250 INT2, not used)
// P1.5 WAKE from loop detector
// P1.7 INT1 of BMA250
#define	PIN_DEFAULT_P1SEL		0x00
#define	PIN_DEFAULT_P1DIR		0x4E
#define	PIN_DEFAULT_P1REN		0x31
#define	PIN_DEFAULT_P1OUT		0x01

// ============================================================================
// P2.0 LOOP AS3932	SDO
// P2.1 		SDI
// P2.2 		SCL
// P2.3 		CS
// P2.4			EXT voltage sensor
// P2.6			INT battery sensor
#define	PIN_DEFAULT_P2SEL		0x50
#define	PIN_DEFAULT_P2DIR		0xAE
#define	PIN_DEFAULT_P2OUT		0x00
#define	PIN_DEFAULT_P2REN		0x01
// ============================================================================
// P3.1 		Turn on the battery sensor
#define	PIN_DEFAULT_P3SEL		0x00
#define	PIN_DEFAULT_P3DIR		0xFF
#define	PIN_DEFAULT_P3OUT		0x00
// ============================================================================
// P4.6 - LED (G) (active low)
// P4.5 - LED (R) (active low)
#define	PIN_DEFAULT_P4SEL		0x00
#define	PIN_DEFAULT_P4DIR		0x60
#define	PIN_DEFAULT_P4OUT		0x60
// ============================================================================
// P5.2 BMA250 CSB
// P5.5 BMA250 SCx
// P5.6 BMA250 SDx
// P5.7 BMA250 SDO
#define	PIN_DEFAULT_P5SEL		0x00
#define	PIN_DEFAULT_P5DIR		0x7F
#define	PIN_DEFAULT_P5OUT		0x24
// ============================================================================

// Normal button on P1.0
#define	BUTTON_LIST 		{ BUTTON_DEF (1, 0x01, 0) }
#define	BUTTON_PIN_P1_IRQ	0x01
#define	BUTTON_DEBOUNCE_DELAY	32
#define	BUTTON_PRESSED_LOW	1
#define	BUTTON_PANIC		0

#define	PIN_MAX		0
#define	PIN_MAX_ANALOG	0
#define	PIN_DAC_PINS	0x00

// ============================================================================

#define	as3932_csel		_BIS (P2OUT, 0x08)
#define	as3932_cunsel		_BIC (P2OUT, 0x08)

#define	as3932_enable_w		_BIS (P1IE, 0x20)
#define	as3932_disable_w	_BIC (P1IE, 0x20)
#define	as3932_clear_w		_BIC (P1IFG, 0x20)
#define	as3932_int_w		(P1IFG & 0x20)

#define	as3932_clkl		_BIC (P2OUT, 0x04)
#define	as3932_clkh		_BIS (P2OUT, 0x04)

#define	as3932_outl		_BIC (P2OUT, 0x02)
#define	as3932_outh		_BIS (P2OUT, 0x02)

#define	as3932_inp		(P2IN & 0x01)

#define	as3932_enable_d		do { \
				    if (as3932_status & AS3932_STATUS_DATA) \
					_BIS (P1IES, 0x10); \
				    else \
					_BIC (P1IES, 0x10); \
				    as3932_clear_d; \
				    _BIS (P1IE, 0x10); \
				} while (0)

#define	as3932_disable_d	_BIC (P1IE, 0x10)
#define	as3932_clear_d		_BIC (P1IFG, 0x10)

#define	as3932_int_d		(P1IFG & 0x10)

#define	as3932_data		(P1IN & 0x10)

#define	AS3932_CRCVALUE		0x96
// #define	AS3932_CRCVALUE		(-1)

#include "as3932.h"

// ============================================================================

#include "board_rtc.h"
#include "analog_sensor.h"
#include "sensors.h"

// ============================================================================

#include "bma250.h"

#define	bma250_csel	_BIC (P5OUT, 0x04)
#define	bma250_cunsel	_BIS (P5OUT, 0x04)

#define	bma250_enable	_BIS (P1IE, 0x80)
#define	bma250_disable	_BIC (P1IE, 0x80)
#define	bma250_clear	_BIC (P1IFG, 0x80)
#define	bma250_int	(P1IFG & 0x80)

#define	bma250_clkl	_BIC (P5OUT, 0x20)
#define	bma250_clkh	_BIS (P5OUT, 0x20)

#define	bma250_outl	_BIC (P5OUT, 0x40)
#define	bma250_outh	_BIS (P5OUT, 0x40)

#define	bma250_data	(P5IN & 0x80)

// Note: this delay only applies when writing. 15us didn't work, 20us did,
// so 40 looks like a safe bet
#define	bma250_delay	udelay (40)

// ============================================================================

// Portmapper for the sensor pins

#define	PIN_PORTMAP	{ 	portmap_entry (P2MAP0, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_NONE, \
						PM_ANALOG, \
						PM_NONE, \
						PM_ANALOG, \
						PM_NONE) \
			}

// Reference doesn't go out
#define	ADC_REFERENCE_OUT	0

#define	CHA_SEN_PIN		4
#define	CHA_SEN_SHT		4
#define	CHA_SEN_ISI		1
#define	CHA_SEN_NSA		64
// Referenced to Vcc
#define	CHA_SEN_URE		ADC_SREF_VVSS
#define	CHA_SEN_ERE		0

#define	BAT_SEN_PIN		6
#define	BAT_SEN_SHT		4
#define	BAT_SEN_ISI		1
#define	BAT_SEN_NSA		64
// Referenced to 2.5V
#define	BAT_SEN_URE		ADC_SREF_RVSS
#define	BAT_SEN_ERE		ADC_FLG_REF25 | ADC_FLG_REFON

#define	SENSOR_LIST { \
		INTERNAL_VOLTAGE_SENSOR,      \
		INTERNAL_TEMPERATURE_SENSOR,  \
		ANALOG_SENSOR ( BAT_SEN_ISI,  \
				BAT_SEN_NSA,  \
				BAT_SEN_PIN,  \
				BAT_SEN_URE,  \
				BAT_SEN_SHT,  \
				BAT_SEN_ERE), \
		DIGITAL_SENSOR (0, bma250_init, bma250_read),	\
		DIGITAL_SENSOR (0, as3932_init, as3932_read),	\
		ANALOG_SENSOR ( CHA_SEN_ISI,  \
				CHA_SEN_NSA,  \
				CHA_SEN_PIN,  \
				CHA_SEN_URE,  \
				CHA_SEN_SHT,  \
				CHA_SEN_ERE), \
	}

#define	sensor_adc_prelude(p) \
		do { \
			if (ANALOG_SENSOR_PIN (p) == BAT_SEN_PIN) { \
				_BIS (P3OUT, 0x02); \
				mdelay (20); \
			} \
		} while (0);

#define	sensor_adc_postlude(p)	_BIC (P3OUT, 0x02)

// Sensors:
//
//	-3	internal voltage
//	-2	temperature
//	-1	external voltage (replacing standard voltage)
//	 0	accel
//	 1	as3932
//	 2	charger sensor
//

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_EVENTS
#define	SENSOR_INITIALIZERS

// ============================================================================

#define	N_HIDDEN_SENSORS	3

// ============================================================================

#define	EXTRA_INITIALIZERS	__pi_rtc_init ()
