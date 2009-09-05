//
// This file is normally part of board_pins.h. I am putting here the sensor
// related definitions, which will be used primarily for testing
//

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

#define	shtxx_ini_regs	CNOP
#define	shtxx_dtup	_BIC (P1DIR, 0x40)
#define	shtxx_dtdown	_BIS (P1DIR, 0x40)
#define	shtxx_dtin	_BIC (P1DIR, 0x40)
#define	shtxx_dtout	CNOP
#define	shtxx_data	(P1IN & 0x40)

#define	shtxx_ckup	_BIS (P1OUT, 0x80)
#define	shtxx_ckdown	_BIC (P1OUT, 0x80)

// ============================================================================

#define	QSO_PXR_SHT	4	// Sample hold time indicator
#define	QSO_PXR_ISI	1	// Inter sample interval indicator
#define	QSO_PXR_NSA	512	// Number of samples
#define	QSO_PXR_URE	SREF_VEREF_AVSS	// Voltage reference: Veref
#define	QSO_PXR_ERE	0	// No exported reference

#define	MOI_ECO_SHT	4	// Sample hold time indicator
#define	MOI_ECO_ISI	0	// Inter sample interval indicator
#define	MOI_ECO_NSA	16
#define	MOI_ECO_URE	SREF_VEREF_AVSS
#define	MOI_ECO_ERE	0

#define	VSN_INT_PIN	2
#define	VSN_INT_SHT	4
#define	VSN_INT_ISI	0
#define	VSN_INT_NSA	8
#define	VSN_INT_URE	SREF_VREF_AVSS
#define	VSN_INT_ERE	(REFON + REF2_5V)

#define	VSN_EXT_PIN	1
#define	VSN_EXT_SHT	8
#define	VSN_EXT_ISI	0
#define	VSN_EXT_NSA	8
#define	VSN_EXT_URE	SREF_VREF_AVSS
#define	VSN_EXT_ERE	REFON

#define	QSO_PAR0_PIN	0	// PAR/PYR
#define	QSO_PAR1_PIN	3
#define	QSO_PYR2_PIN	4
#define	QSO_PYR3_PIN	5
#define	MOI_ECO_PIN	7	// ECHO moisture sensor


#define	SENSOR_LIST { \
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_humid), \
		ANALOG_SENSOR (   QSO_PXR_ISI,  \
				  QSO_PXR_NSA,  \
				  QSO_PAR0_PIN, \
				  QSO_PXR_URE,  \
				  QSO_PXR_SHT,  \
				  QSO_PXR_ERE), \
		ANALOG_SENSOR (   MOI_ECO_ISI,  \
				  MOI_ECO_NSA,  \
				  MOI_ECO_PIN,  \
				  MOI_ECO_URE,  \
				  MOI_ECO_SHT,  \
				  MOI_ECO_ERE), \
		ANALOG_SENSOR (   QSO_PXR_ISI,  \
				  QSO_PXR_NSA,  \
				  QSO_PAR1_PIN, \
				  QSO_PXR_URE,  \
				  QSO_PXR_SHT,  \
				  QSO_PXR_ERE), \
		ANALOG_SENSOR (   QSO_PXR_ISI,  \
				  QSO_PXR_NSA,  \
				  QSO_PYR2_PIN, \
				  QSO_PXR_URE,  \
				  QSO_PXR_SHT,  \
				  QSO_PXR_ERE), \
		ANALOG_SENSOR (   QSO_PXR_ISI,  \
				  QSO_PXR_NSA,  \
				  QSO_PYR3_PIN, \
				  QSO_PXR_URE,  \
				  QSO_PXR_SHT,  \
				  QSO_PXR_ERE), \
		ANALOG_SENSOR (   VSN_INT_ISI,  \
				  VSN_INT_NSA,  \
				  VSN_INT_PIN,  \
				  VSN_INT_URE,  \
				  VSN_INT_SHT,  \
				  VSN_INT_ERE), \
		ANALOG_SENSOR (   VSN_EXT_ISI,  \
				  VSN_EXT_NSA,  \
				  VSN_EXT_PIN,  \
				  VSN_EXT_URE,  \
				  VSN_EXT_SHT,  \
				  VSN_EXT_ERE)  \
	}

// Sensor list:
//
//	0 - SHT TMP
//	1 - SHT HUM
//	2 - PXR 0
//	3 - ECHO
//	4 - PXR 1
//	5 - PXR 2
//	6 - PXR 3
//	7 - Voltage int
//	8 - Voltage ext

#define	sensor_adc_prelude(p) \
			do { \
			    switch (ANALOG_SENSOR_PIN (p)) { \
				case QSO_PAR0_PIN: \
				case QSO_PAR1_PIN: \
				case QSO_PYR2_PIN: \
				case QSO_PYR3_PIN: \
					_BIS (P5OUT, 0x10); \
					goto Del__; \
				case MOI_ECO_PIN: \
					_BIS (P4OUT, 0x80); \
				Del__:  mdelay (40); \
					break; \
				case VSN_INT_PIN: \
					_BIS (P2DIR, 0x20); \
					mdelay (10); \
			    } \
			} while (0)

// Note: the ramp on Vref (internal reference 2.5V) is ca. 4.5ms, for 1.5V,
// the ramp is 2.1ms, so 10ms pretty much covers it

#define	sensor_adc_postlude(p) \
			do { \
				_BIC (P5OUT, 0x10); \
				_BIC (P4OUT, 0x80); \
				_BIC (P2DIR, 0x20); \
			} while (0)

// Note: the 40ms delay (see above) was determined experimentally by looking
// at the reference voltage ramp across the 10uF/100nF capacitors connected
// to Veref. The ramp takes about 25ms to reach stable voltage, to the 40ms
// include some safety margin.
