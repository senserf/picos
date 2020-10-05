/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "pins.h"

#ifdef PULSE_MONITOR
#error "S: PULSE_MONITOR not implemented for this architecture"
#endif

#ifdef PIN_LIST

static byte pinmap [] = PIN_LIST;

#define	__pin_max sizeof (pinmap)

Boolean __pi_pin_available (word p) {

	return p < __pin_max;
}

Boolean __pi_pin_adc_available (word p) {
//
// Not implemented yet
//
	return NO;
}

word __pi_pin_ivalue (word p) {

	if (p >= __pin_max)
		return 0;

	return GPIO_readDio (pinmap [p]);
}

word __pi_pin_ovalue (word p) {

	if (p >= __pin_max)
		return 0;

	return (HWREG (GPIO_BASE + GPIO_O_DOUT31_0) >> pinmap [p]) & 1;
}

#if PIN_DAC_PINS != 0

#error "S: DAC not implemented yet for this architecture"

Boolean __pi_pin_dac_available (word p) {

	return NO;
}

Boolean __pi_pin_dac (word p) {

	return NO;
}

void __pi_clear_dac (word p) {

}

void __pi_set_dac (word p) {


void __pi_write_dac (word p, word val, word ref) {

}

#endif	/* PIN_DAC_PINS */

Boolean __pi_pin_adc (word p) {

	return 0;
}

Boolean __pi_pin_output (word p) {

	if (p >= __pin_max)
		return 0;

	return GPIO_getOutputEnableDio (pinmap [p]);
}

void __pi_pin_set (word p) {

	if (p < __pin_max)
		GPIO_setDio (pinmap [p]);
}

void __pi_pin_clear (word p) {

	if (p < __pin_max)
		GPIO_clearDio (pinmap [p]);
}

void __pi_pin_set_input (word p) {

	if (p < __pin_max)
		GPIO_setOutputEnableDio (pinmap [p], GPIO_OUTPUT_DISABLE);
}

void __pi_pin_set_output (word p) {

	if (p < __pin_max)
		GPIO_setOutputEnableDio (pinmap [p], GPIO_OUTPUT_ENABLE);
}

void __pi_pin_set_adc (word p) {

}

#endif /* __pin_max */
