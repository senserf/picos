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

	lword	W [3];

} i_sensdesc_t;

typedef	struct {

	byte	tp,		// Type (this must be a byte)
		dummy;
	word	nsamples;
	word	adcpars [4];	// One spare

} a_sensdesc_t;

typedef struct {

	byte	tp, num, dummy [2];
	void	(*fun_val) (word, const byte*, address);
	void	(*fun_ini) ();

} d_sensdesc_t;

// Temporary?
#define	adc_inuse	NO

#define	sensor_adc_config(a)	do { \
		AUXWUCClockEnable (AUX_WUC_ADI_CLOCK | AUX_WUC_ANAIF_CLOCK); \
		AUXADCSelectInput (a [0]); \
		AUXADCEnableSync (a [1], a [2], AUXADC_TRIGGER_MANUAL); \
		AUXADCFlushFifo (); \
	} while (0)

#define	adc_start	AUXADCGenManualTrigger ()
#define	adc_busy	(AUXADCGetFifoStatus () & AUXADC_FIFO_EMPTY_M)
#define	adc_off		CNOP
#define	adc_value	((word)AUXADCReadFifo ())

#define	adc_disable	do { \
		AUXADCDisable (); \
		AUXWUCClockDisable (AUX_WUC_ADI_CLOCK | AUX_WUC_ANAIF_CLOCK); \
			} while (0)
				

// tp encodes the inter sampling interval, which must be below 128 ms,
// adcpars encodes: pin (mux) number, reference, and sampling time
#define	ANALOG_SENSOR(isi,ns,pn,ref,sht) \
	{ {	((lword) (isi) | (((lword) ( ns)) << 16)), \
		((lword) ( pn) | (((lword) (ref)) << 16)), \
		((lword) (sht) ) } }

#define	DIGITAL_SENSOR(par,ini,pro) \
	{ {	((lword) 0x80 | ((lword) (par)) << 8), \
		(lword) (pro), (lword) (ini) } }

// This one we need to identify sensors from the ADC parameters (by pin number)
// in macros defined in BOARD-specific files, e.g., to implement conditional
// preludes and postludes

#define	ANALOG_SENSOR_PIN(par)	(*(par))

#ifdef	SENSOR_DIGITAL

void __pi_batmon (word, const byte*, address);

#define	INTERNAL_VOLTAGE_SENSOR	\
	DIGITAL_SENSOR (0, NULL, __pi_batmon)

#define	INTERNAL_TEMPERATURE_SENSOR	\
	DIGITAL_SENSOR (1, NULL, __pi_batmon)

#endif
#endif
