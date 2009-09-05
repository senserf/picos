#ifndef	__pg_analog_sensor_sys_h
#define	__pg_analog_sensor_sys_h

#include "pins.h"

//
// The interpretation of ADC parameters (3 bytes) stored in sensor description:
//
//	byte  0   - contents of ADC12MCTL0, i.e., EOS | used reference | pin
//	bytes 1,2 - little endian 16-bit contents of ADC12CTL0, i.e.,
//                  (exported) reference, sample & hold time
//
#define	sensor_adc_config(a)	do { \
			  _BIC (ADC12CTL0, ENC); \
			  ADC12CTL1 = ADC_CLOCK_DIVIDER + \
						ADC_CLOCK_SOURCE + SHP; \
			  ADC12MCTL0 = *(a); \
			  ADC12CTL0 = *((word*)(a+1)); \
			} while (0)

#define	ANALOG_SENSOR(isi,ns,pn,uref,sht,eref) \
	__ANALOG_SENSOR (isi, ns, \
		(uref) | (pn) | EOS, \
		ADC12ON | (eref), \
		((sht) << 4) | (sht) )

// This one we need to identify sensors from the ADC parameters (by pin number)
#define	ANALOG_SENSOR_PIN(par)	(*(par) & 0xf)

#endif
