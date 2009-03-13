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
#define	PIN_DEFAULT_P5DIR	0x50	// Vref generator (pos end of Zener)

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
#define	QSO_PXR_NSA	6	// Number of samples, corresponds to 512
#define	QSO_PXR_REF	3	// Voltage reference: Veref

#define	MOI_ECO_SHT	4	// Sample hold time indicator
#define	MOI_ECO_ISI	0	// Inter sample interval indicator
#define	MOI_ECO_NSA	2	// Number of samples, corresponds to 16
#define	MOI_ECO_REF	3	// Voltage reference: Veref

#define	QSO_PXR0_PIN	0	// PAR/PYR
#define	QSO_PXR1_PIN	3
#define	QSO_PXR2_PIN	4
#define	QSO_PXR3_PIN	5
#define	MOI_ECO_PIN	7	// ECHO moisture sensor

#define	EREF_ON		do { \
				if (ASNS_PNO == MOI_ECO_PIN) { \
					_BIS (P4OUT, 0x80); \
				} else { \
					_BIS (P5OUT, 0x10); \
				} \
				mdelay (400); \
			} while (0)

#define	EREF_OFF	do { _BIC (P5OUT, 0x10); _BIC (P4OUT, 0x80); } while (0)

#define	SENSOR_LIST	{ \
		SENSOR_DEF (shtxx_init, shtxx_temp, 0), \
		SENSOR_DEF (NULL, shtxx_humid, 0), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 QSO_PXR0_PIN | \
			(QSO_PXR_SHT << ASNS_SHT_SH) | \
			(QSO_PXR_ISI << ASNS_ISI_SH) | \
			(QSO_PXR_NSA << ASNS_NSA_SH) | \
			(QSO_PXR_REF << ASNS_REF_SH)), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			MOI_ECO_PIN | \
			(MOI_ECO_SHT << ASNS_SHT_SH) | \
			(MOI_ECO_ISI << ASNS_ISI_SH) | \
			(MOI_ECO_NSA << ASNS_NSA_SH) | \
			(MOI_ECO_REF << ASNS_REF_SH)), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 QSO_PXR1_PIN | \
			(QSO_PXR_SHT << ASNS_SHT_SH) | \
			(QSO_PXR_ISI << ASNS_ISI_SH) | \
			(QSO_PXR_NSA << ASNS_NSA_SH) | \
			(QSO_PXR_REF << ASNS_REF_SH)), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 QSO_PXR2_PIN | \
			(QSO_PXR_SHT << ASNS_SHT_SH) | \
			(QSO_PXR_ISI << ASNS_ISI_SH) | \
			(0 << ASNS_NSA_SH) | \
			(QSO_PXR_REF << ASNS_REF_SH)), \
		SENSOR_DEF (NULL, analog_sensor_read, \
			 QSO_PXR3_PIN | \
			(QSO_PXR_SHT << ASNS_SHT_SH) | \
			(QSO_PXR_ISI << ASNS_ISI_SH) | \
			(QSO_PXR_NSA << ASNS_NSA_SH) | \
			(QSO_PXR_REF << ASNS_REF_SH))  \
	}


