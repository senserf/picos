// ============================================================================
// CMA3000 - acceleration sensor ==============================================
// ============================================================================

#include "cma_3000.h"

#define	__pi_cma_3000_bring_down	do { \
						_BIC (P4OUT, 0x01); \
						_BIC (P4DIR, 0x01); \
						_BIC (P5OUT, 0xE4); \
						_BIS (P5DIR, 0xE4); \
						__pi_cma_3000_disable; \
					} while (0)
//+++ "p1irq.c"

#define	__pi_cma_3000_bring_up		do { \
						_BIC (P5DIR, 0x80); \
						_BIS (P4OUT, 0x01); \
						_BIS (P4DIR, 0x01); \
					} while (0)
					
#define	__pi_cma_3000_csel		do { \
						_BIC (P5REN, 0x80); \
						_BIC (P5OUT, 0x04); \
					} while (0)

#define	__pi_cma_3000_cunsel		do { \
						_BIS (P5OUT, 0x04); \
						_BIS (P5REN, 0x80); \
					} while (0)

// Rising edge, no need to change IES
#define	__pi_cma_3000_enable	_BIS (P1IE, 0x80)
#define	__pi_cma_3000_disable	_BIC (P1IE, 0x80)
#define	__pi_cma_3000_clear	_BIC (P1IFG, 0x80)
#define	__pi_cma_3000_int	(P1IFG & 0x80)

// Raw pin I/O
#define	__pi_cma_3000_clkh	_BIS (P5OUT, 0x20)
#define	__pi_cma_3000_clkl	_BIC (P5OUT, 0x20)
#define	__pi_cma_3000_outh	_BIS (P5OUT, 0x40)
#define	__pi_cma_3000_outl	_BIC (P5OUT, 0x40)
#define	__pi_cma_3000_data	(P5IN & 0x80)

// ============================================================================
// ============================================================================

#define	SENSOR_DIGITAL
#define	SENSOR_ANALOG
#define	SENSOR_INITIALIZERS

#include "analog_sensor.h"
#include "sensors.h"
//+++ "sensors.c"

#define	SEN_POWER_SRC	11	// Vcc/2 (pin for int-int voltage sensor)
#define	SEN_POWER_SHT	4	// 8 cycles = 490us
#define	SEN_POWER_ISI	0	// Inter-sample interval
#define	SEN_POWER_NSA	16	// Samples to average
#define	SEN_POWER_URE	ADC_SREF_RVSS	// Internal
#define	SEN_POWER_ERE	(ADC_FLG_REFON + ADC_FLG_REF25)

#define	SEN_POWER_PIN	6	// Int-ext voltage sensor pin, same ref

#define	SEN_EPOWER_PIN	7	// External voltage pin
#define	SEN_EPOWER_ERE	ADC_FLG_REFON

#define	SENSOR_LIST { \
		ANALOG_SENSOR (SEN_POWER_ISI,  \
			       SEN_POWER_NSA,  \
			       SEN_POWER_SRC,  \
		       	       SEN_POWER_URE,  \
			       SEN_POWER_SHT,  \
			       SEN_POWER_ERE), \
		ANALOG_SENSOR (SEN_POWER_ISI,  \
			       SEN_POWER_NSA,  \
			       SEN_POWER_PIN,  \
		       	       SEN_POWER_URE,  \
			       SEN_POWER_SHT,  \
			       SEN_POWER_ERE), \
		ANALOG_SENSOR (SEN_POWER_ISI,  \
			       SEN_POWER_NSA,  \
			       SEN_EPOWER_PIN, \
		       	       SEN_POWER_URE,  \
			       SEN_POWER_SHT,  \
			       SEN_EPOWER_ERE),\
		DIGITAL_SENSOR (0, NULL, cma_3000_read) \
}

// Voltage internal-internal
// Voltage internal-external
// Voltage external
// Motion

#define	sensor_adc_prelude(p) \
			do { \
			    switch (ANALOG_SENSOR_PIN (p)) { \
				case SEN_POWER_PIN: \
				case SEN_EPOWER_PIN: \
					_BIS (P4DIR, 0x08); \
					mdelay (100); \
			    } \
			} while (0)

#define	sensor_adc_postlude(p) \
			do { \
				_BIC (P4DIR, 0x08); \
			} while (0)