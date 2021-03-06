/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_sensors_sys_h
#define	__pg_sensors_sys_h

#include "sysio.h"

// ============================================================================
// These three structures are interchangeable and take the same amount of space;
// they are alternative layouts of the sensor array
// ============================================================================

typedef struct {

	word	W [3];

} i_sensdesc_t;

typedef	struct {

	byte	tp,		// Type
		adcpars [3];	// ADC parameters
	word	nsamples;	// Number of samples per measurement

} a_sensdesc_t;

typedef struct {

	byte tp, num;
	void (*fun_val) (word, const byte*, address);
	void (*fun_ini) ();

} d_sensdesc_t;

// ============================================================================
//
// The interpretation of ADC parameters (3 bytes) stored in sensor description:
//
//	byte  0   - contents of ADC12MCTL0, i.e., EOS | used reference | pin
//	bytes 1,2 - little endian 16-bit contents of ADC12CTL0, i.e.,
//                  (exported) reference, sample & hold time
//
#define	sensor_adc_config(a)	do { \
			  _BIC (ADC12CTL0, ADC_FLG_ENC); \
			  ADC_CTL2_SET; \
			  ADC12CTL1 = ADC_CLOCK_DIVIDER + \
					ADC_CLOCK_SOURCE + ADC_FLG_SHP; \
			  ADC12MCTL0 = *(a); \
			  ADC12CTL0 = *((word*)(a+1)); \
			} while (0)

#define	DIGITAL_SENSOR(par,ini,pro) \
	{ { ((word) 0x80 | ((word)(par)) << 8), (word)(pro), (word)(ini) } }

#define	ANALOG_SENSOR(isi,ns,pn,uref,sht,eref) \
	{ { (word)(isi) | (((word)((uref) | (pn) | ADC_FLG_EOS)) << 8), \
	     (word)(ADC12ON | (eref)) | (((word)(((sht) << 4) | (sht))) << 8), \
	     (word)(ns) } }

// This one we need to identify sensors from the ADC parameters (by pin number)
// in macros defined in BOARD-specific files, e.g., to implement conditional
// preludes and postludes

#define	ANALOG_SENSOR_PIN(par)	(*(par) & 0xf)

#ifdef	INCH_VCC2
#define	INTERNAL_VOLTAGE_SENSOR	\
      ANALOG_SENSOR (0,16,INCH_VCC2,ADC_SREF_RVSS,4,ADC_FLG_REFON+ADC_FLG_REF25)
#endif

#ifdef	INCH_TEMP
#define	INTERNAL_TEMPERATURE_SENSOR	\
      ANALOG_SENSOR (0,16,INCH_TEMP,ADC_SREF_RVSS,4,ADC_FLG_REFON)
#endif
#endif
