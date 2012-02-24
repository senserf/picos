//
// This file is normally part of board_pins.h. I am putting here the sensor
// related definitions, which will be used primarily for testing
//

#include "sht_xx.h"
#include "analog_sensor.h"
#include "sensors.h"

#undef	PIN_DEFAULT_P1DIR
#undef	PIN_DEFAULT_P4DIR
#undef	PIN_DEFAULT_P5DIR

#define	PIN_DEFAULT_P1DIR	0xa3	// P1.7 = SHT clock (closed drain)
#define	PIN_DEFAULT_P4DIR	0x8e	// Vref generator (neg end of Zener)

// Vref generator (Zener): P5.4; Probe on: P5.7, GPS on P5.6
#define	PIN_DEFAULT_P5DIR	0xD0
// P5.7 = voltage sensor switch (active low)
#define	PIN_DEFAULT_P5OUT	0x80

#define	PIN_DEFAULT_P6SEL	0xFF	// The sensors

#define	shtxx_ini_regs	CNOP
#define	shtxx_dtup	_BIC (P1DIR, 0x40)
#define	shtxx_dtdown	_BIS (P1DIR, 0x40)
#define	shtxx_dtin	_BIC (P1DIR, 0x40)
#define	shtxx_dtout	do { } while (0)
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

#define	QSO_PXR0_PIN	0	// PAR/PYR
#define	QSO_PXR1_PIN	3
#define	QSO_PXR2_PIN	4
#define	QSO_PXR3_PIN	5
#define	MOI_ECO_PIN	7	// ECHO moisture sensor

#define	SENSOR_PROBE0_PIN	1	// Sensor probe pins
#define	SENSOR_PROBE1_PIN	2
#define	SENSOR_PROBE2_PIN	6

#define	SENSOR_PROBE_SHT	4	// ADC parameters for sensor probe SHT
#define	SENSOR_PROBE_ISI	0
#define	SENSOR_PROBE_NSA	4
#define	SENSOR_PROBE_URE	SREF_AVCC_AVSS
#define	SENSOR_PROBE_ERE	0


#define	SENSOR_LIST { \
		INTERNAL_TEMPERATURE_SENSOR,	\
		INTERNAL_VOLTAGE_SENSOR,	\
		DIGITAL_SENSOR (0, shtxx_init, shtxx_temp), \
		DIGITAL_SENSOR (0, NULL, shtxx_humid), \
		ANALOG_SENSOR ( QSO_PXR_ISI,  \
				QSO_PXR_NSA,  \
				QSO_PXR0_PIN,  \
				QSO_PXR_URE,  \
				QSO_PXR_SHT,  \
				QSO_PXR_ERE), \
		ANALOG_SENSOR ( MOI_ECO_ISI,  \
				MOI_ECO_NSA,  \
				MOI_ECO_PIN,  \
				MOI_ECO_URE,  \
				MOI_ECO_SHT,  \
				MOI_ECO_ERE), \
		ANALOG_SENSOR ( QSO_PXR_ISI,  \
				QSO_PXR_NSA,  \
				QSO_PXR1_PIN,  \
				QSO_PXR_URE,  \
				QSO_PXR_SHT,  \
				QSO_PXR_ERE), \
		ANALOG_SENSOR ( QSO_PXR_ISI,  \
				QSO_PXR_NSA,  \
				QSO_PXR2_PIN,  \
				QSO_PXR_URE,  \
				QSO_PXR_SHT,  \
				QSO_PXR_ERE), \
		ANALOG_SENSOR ( QSO_PXR_ISI,  \
				QSO_PXR_NSA,  \
				QSO_PXR3_PIN,  \
				QSO_PXR_URE,  \
				QSO_PXR_SHT,  \
				QSO_PXR_ERE), \
		ANALOG_SENSOR ( SENSOR_PROBE_ISI,  \
				SENSOR_PROBE_NSA,  \
				SENSOR_PROBE0_PIN,  \
				SENSOR_PROBE_URE,  \
				SENSOR_PROBE_SHT,  \
				SENSOR_PROBE_ERE), \
		ANALOG_SENSOR ( SENSOR_PROBE_ISI,  \
				SENSOR_PROBE_NSA,  \
				SENSOR_PROBE1_PIN,  \
				SENSOR_PROBE_URE,  \
				SENSOR_PROBE_SHT,  \
				SENSOR_PROBE_ERE), \
		ANALOG_SENSOR ( SENSOR_PROBE_ISI,  \
			  	SENSOR_PROBE_NSA,  \
				SENSOR_PROBE2_PIN,  \
				SENSOR_PROBE_URE,  \
				SENSOR_PROBE_SHT,  \
				SENSOR_PROBE_ERE) \
	}

#define	N_HIDDEN_SENSORS	2

// Sensor list:
//
//	0 - SHT TMP
//	1 - SHT HUM
//	2 - PXR 0
//	3 - ECHO
//	4 - PXR 1
//	5 - PXR 2
//	6 - PXR 3
//	7 - PROBE 0
//	8 - PROBE 1
//	9 - PROBE 2

#define	sensor_adc_prelude(p) \
			do { \
			  switch (ANALOG_SENSOR_PIN (p)) { \
			    case QSO_PXR0_PIN: \
			    case QSO_PXR1_PIN: \
			    case QSO_PXR2_PIN: \
			    case QSO_PXR3_PIN: \
				_BIS (P5OUT, 0x10); \
				break; \
			    case MOI_ECO_PIN: \
				_BIS (P4OUT, 0x80); \
				break; \
			    case SENSOR_PROBE0_PIN: \
			    case SENSOR_PROBE1_PIN: \
				_BIC (P5OUT, 0x80); \
			  } \
			  mdelay (40); \
			} while (0)

// Note: the 40ms delay (see above) was determined experimentally by looking
// at the reference voltage ramp across the 10uF/100nF capacitors connected
// to Veref. The ramp takes about 25ms to reach stable voltage, to the 40ms
// include some safety margin.

#define	sensor_adc_postlude(p) \
    do { _BIC (P5OUT, 0x10); _BIS (P5OUT, 0x80); _BIC (P4OUT, 0x80); } while (0)

#define	SENSOR_INITIALIZERS	// To make sure global init function gets called
#define	SENSOR_ANALOG		// To make sure analog sensors are processed
#define	SENSOR_DIGITAL		// To make sure digital sensors are processed
