/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_actuators_sys_h
#define	__pg_actuators_sys_h

#include "sysio.h"

typedef struct {

	word	W [3];

} i_actudesc_t;

typedef	struct {

	byte	tp, iref;	// Type (0 or 1)
	word	dacpars,	// Parameters
		interval;
} a_actudesc_t;

typedef struct {

	byte tp, num;
	void (*fun_val) (word, const byte*, address);
	void (*fun_ini) ();

} d_actudesc_t;

#define	actuator_dac_config(a) do { \
		if ((a->dacpars & DAC12SREF1) == 0) \
			adc_set_reference (a->iref); \
		if (a->tp) { \
			_BIC (DAC12_1CTL, DAC12ENC); \
			DAC12_1CTL = a->dacpars; \
		} else { \
			_BIC (DAC12_0CTL, DAC12ENC); \
			DAC12_0CTL = a->dacpars; \
		} \
	} while (0)

#define	actuator_dac_setvalue(a,v) do { \
		if (a->tp) { \
			DAC12_1DAT = (v); \
		} else { \
			DAC12_0DAT = (v); \
		} \
	} while (0)

#define	actuator_dac_off(a) do { \
		if (a->tp == 0) { \
			DAC12_0CTL = DAC12AMP_1; \
		} else { \
			DAC12_1CTL = DAC12AMP_1; \
		} \
		if ((a->dacpars & DAC12SREF1) == 0) \
			adc_disable; \
	} while (0)

#define DIGITAL_ACTUATOR(par,ini,pro) \
	{ { ((word) 0x80 | ((word)(par)) << 8), (word)(pro), (word)(ini) } }

#define ANALOG_ACTUATOR(tim,pin,bump,ref,href) \
	{ { (word)(pin) | (((href) != 0) << 8), \
		 DAC12AMP_5 | ((word)ref ? DAC12SREF_2 : DAC12SREF_0) | \
			((bump) ? DAC12IR : 0), (word)(tim) } }

#define	ANALOG_ACTUATOR_PIN(par) (*(par) & 0xf)

#endif
