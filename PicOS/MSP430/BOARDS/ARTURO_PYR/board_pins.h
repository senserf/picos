/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// Sensor connections:
//
//	PAR = P6.0	(analog input)
//	      P6.3
//	PYR = P6.4
//	      P6.5
//	MOI = P6.7	(analog input)
//	      P4.7	(excitation)
//	SHT = P1.6 Data, P1.7 Clock
//
//	For reading PAR/PYR, set Veref to 1.2V by pulling P4.7 down and P5.4
//	up to Vcc
//	For reading MOI, set Veref to excitation voltage (minus constant
//	forward Zener drop) by setting P5.4 to ground and P4.7 to Vcc
//	(which will also excite the sensor).
//	The normal state of P4.7/P5.4 should be output low for both
//

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80	// P1.7 == CLK for the SHT sensor
#define	PIN_DEFAULT_P2DIR	0x83	// 0,1 and 7 hang loose, 2-6 nc
#define	PIN_DEFAULT_P3DIR	0xC9

// P5.4 == reference voltage for PAR; EEPROM: 0-CS, 1-SI, 2-SO, 3-SCK
#define	PIN_DEFAULT_P5DIR	0xFB
#define	PIN_DEFAULT_P5OUT	0x09

#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off by default
#define	PIN_DEFAULT_P4DIR	0x8E	// P4.7 == Veref selector (PAR/MOI)
#define	PIN_DEFAULT_P6DIR	0x00
#define PIN_DEFAULT_P6SEL	0xB9	// Five sensors

//#define	RESET_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#define	PIN_MAX 		0
#define	PIN_MAX_ANALOG		0
#define	PIN_DAC_PINS		0

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	QSO_PAR0_PIN	0	// PAR sensor = P6.0
#define	QSO_PAR0_SHT	4	// Sample hold time indicator
#define	QSO_PAR0_ISI	1	// Inter sample interval indicator
#define	QSO_PAR0_NSA	512	// Number of samples
#define	QSO_PAR0_URE	SREF_VEREF_AVSS	// Used reference
#define	QSO_PAR0_ERE	0	// Exported reference

#define	QSO_PAR1_PIN	3
#define	QSO_PAR1_SHT	QSO_PAR0_SHT
#define	QSO_PAR1_ISI	QSO_PAR0_ISI
#define	QSO_PAR1_NSA	QSO_PAR0_NSA
#define	QSO_PAR1_URE	QSO_PAR0_URE
#define	QSO_PAR1_ERE	0

#define	QSO_PYR0_PIN	4
#define	QSO_PYR0_SHT	QSO_PAR0_SHT
#define	QSO_PYR0_ISI	QSO_PAR0_ISI
#define	QSO_PYR0_NSA	QSO_PAR0_NSA
#define	QSO_PYR0_URE	QSO_PAR0_URE
#define	QSO_PYR0_ERE	0

#define	QSO_PYR1_PIN	5
#define	QSO_PYR1_SHT	QSO_PAR0_SHT
#define	QSO_PYR1_ISI	QSO_PAR0_ISI
#define	QSO_PYR1_NSA	QSO_PAR0_NSA
#define	QSO_PYR1_URE	QSO_PAR0_URE
#define	QSO_PYR1_ERE	0

#define	QSO_ZENER_PINS	((1 << QSO_PAR0_PIN) | (1 << QSO_PAR1_PIN) | \
			 (1 << QSO_PYR0_PIN) | (1 << QSO_PYR1_PIN) )

#define	MOI_ECO_PIN	7	// ECHO moisture sensor = P6.7
#define	MOI_ECO_SHT	4	// Sample hold time indicator
#define	MOI_ECO_ISI	0	// Inter sample interval indicator
#define	MOI_ECO_NSA	16
#define	MOI_ECO_URE	SREF_VEREF_AVSS
#define	MOI_ECO_ERE	SREF_VEREF_AVSS

// Notes re ECHO sensor reference:
//
// Tests regarding stability in the face of varying Vcc were rather
// disappointing.
//
// Forward Zener fed from excitation voltage (4.7K to ground):
//	3.12V	->	449
//	2.91V	->	427   5.02% / 0.21V = 23.9%/V
//	2.59V	->	390   9.06% / 0.32V = 28.3%/V
//
// Both pins high:
//	3.12V	->	335  
//	2.90V	->	311   7.43% / 0.22V = 33.8%/V
//	2.59V	->	274  12.65% / 0.31V = 40.8%/V
//
// Vcc ref:
//	3.12V	->	331
//	2.90V	->	307   7.52% / 0.22V = 34.2%/V
//	2.60V	->	273  11.72% / 0.30V = 39.1%/V
//
// ===========================================================================
// The moral: use the Zener to ground (option 1).
//
// Let us calibrate at 3.05V Vcc. This is very rough, as I am using the
// oscilloscope to determine the excitation voltage on the pin and the
// reference voltage on the Zener.
//
// EC-5:
//
// The excitation voltage taken from the pin turns out 2.8V, while the
// reference (after the Zener) is 2.2V.
//
// The formula from ECHO manual (for ECHO-5):
//
// F = 11.9 * 0.0001 * V - 0.401
//
// assumes that V is in mV in response to 2500mV excitation. We have ADC
// indications r between 0 an 4095. Thus, we should replace V with
//
// 2500 * (r / 4095) * 2.8/2.2.
//
// which yields:
//
// F = 0.0007265 * V_exc/V_ref * r - 0.401, and for V_exc/V_ref = 2.8/2.2:
//
// F = (0.0009246 * r - 0.401) * 100%
//
// EC-20:
//
// V_exc = 2.95, V_ref = 2.35 (fixed drop on the Zener)
//
// The formula:
//
// F = 0.000695 * V - 0.29, i.e.,
// F = 0.000424 * V_exc/V_ref * r - 0.26, i.e.,
// F = 0.000532 * r - 0.26
// ============================================================================


#define	SENSOR_LIST { \
		ANALOG_SENSOR ( QSO_PAR0_ISI,  \
				QSO_PAR0_NSA,  \
				QSO_PAR0_PIN,  \
				QSO_PAR0_URE,  \
				QSO_PAR0_SHT,  \
				QSO_PAR0_ERE), \
		ANALOG_SENSOR ( QSO_PAR1_ISI,  \
				QSO_PAR1_NSA,  \
				QSO_PAR1_PIN,  \
				QSO_PAR1_URE,  \
				QSO_PAR1_SHT,  \
				QSO_PAR1_ERE), \
		ANALOG_SENSOR ( QSO_PYR0_ISI,  \
				QSO_PYR0_NSA,  \
				QSO_PYR0_PIN,  \
				QSO_PYR0_URE,  \
				QSO_PYR0_SHT,  \
				QSO_PYR0_ERE), \
		ANALOG_SENSOR ( QSO_PYR1_ISI,  \
				QSO_PYR1_NSA,  \
				QSO_PYR1_PIN,  \
				QSO_PYR1_URE,  \
				QSO_PYR1_SHT,  \
				QSO_PYR1_ERE), \
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_humid), \
		ANALOG_SENSOR ( MOI_ECO_ISI,  \
				MOI_ECO_NSA,  \
				MOI_ECO_PIN,  \
				MOI_ECO_URE,  \
				MOI_ECO_SHT,  \
				MOI_ECO_ERE)  \
	}

#define	sensor_adc_prelude(p) \
			do { \
				if (((1 << ANALOG_SENSOR_PIN (p)) & \
				    QSO_ZENER_PINS)) { \
					_BIS (P5OUT, 0x10); \
				} else { \
					_BIS (P4OUT, 0x80); \
				} \
				mdelay (20); \
			} while (0)

#define	sensor_adc_postlude(p) \
			do { _BIC (P5OUT, 0x10); _BIC (P4OUT, 0x80); } while (0)

#define	SENSOR_PAR0	0
#define	SENSOR_PAR1	1
#define	SENSOR_PYR0	2
#define	SENSOR_PYR1	3
#define	SENSOR_TEMP	4
#define	SENSOR_HUMID	5
#define	SENSOR_MOI	6

// Pin definitions for the SHT sensor

// No need to initialize - just make sure the defaults above are right
#define	shtxx_ini_regs	CNOP
#define	shtxx_dtup	_BIC (P1DIR, 0x40)
#define	shtxx_dtdown	_BIS (P1DIR, 0x40)
#define	shtxx_dtin	_BIC (P1DIR, 0x40)
#define	shtxx_dtout	do { } while (0)
#define	shtxx_data	(P1IN & 0x40)

#define	shtxx_ckup	_BIS (P1OUT, 0x80)
#define	shtxx_ckdown	_BIC (P1OUT, 0x80)
