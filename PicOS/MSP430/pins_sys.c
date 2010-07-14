/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"

#ifdef PULSE_MONITOR
word		__pi_pmonevent [0];
__pi_pmon_t	__pi_pmon;
#endif

#if PIN_MAX

static const pind_t pinmap [] = PIN_LIST;

Boolean __pi_pin_available (word p) {

	if ((p >= PIN_MAX) || (pinmap[p].poff == 0xff))
		return NO;

#ifdef PULSE_MONITOR

	if (p == 1 && (pmon.stat & PMON_CNT_ON) != 0)
		return NO;

	if (p == 2 && (pmon.stat & PMON_NOT_ON) != 0)
		return NO;
#endif
	return YES;
}

Boolean __pi_pin_adc_available (word p) {

	if (!__pi_pin_available (p) || p >= PIN_MAX_ANALOG)
		return NO;

	return YES;
}

word __pi_pin_ivalue (word p) {

	if (p >= PIN_MAX)
		return 0;
	return (*(byte*)
		(__PORT_FBASE__ + pinmap[p].poff) >> pinmap[p].pnum) & 1;
}

word __pi_pin_ovalue (word p) {

	if (p >= PIN_MAX)
		return 0;
	return (*(byte*)
		(__PORT_FBASE__ + pinmap[p].poff + POUT_off) >> pinmap[p].pnum)
			& 1;
}

#if PIN_DAC_PINS != 0

Boolean __pi_pin_dac_available (word p) {

	if (p != (PIN_DAC_PINS & 0xf) && p != ((PIN_DAC_PINS >> 8) & 0xf))
		return NO;

	return YES;
}

Boolean __pi_pin_dac (word p) {

	if (p != (PIN_DAC_PINS & 0xf) && p != ((PIN_DAC_PINS >> 8) & 0xf))
		// Up to two DAC pins are handled at the moment
		return 0;

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		return (DAC12_0CTL & DAC12AMP_7) != 0;
	}

	return (DAC12_1CTL & DAC12AMP_7) != 0;
}

void __pi_clear_dac (word p) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		DAC12_0CTL = 0;	// Disable DAC0
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		DAC12_1CTL = 0;
	}
}

void __pi_set_dac (word p) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		dac_config_write (0, 0, 0);
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		dac_config_write (1, 0, 0);
	}
}

void __pi_write_dac (word p, word val, word ref) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		dac_config_write (0, val, ref);
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		dac_config_write (1, val, ref);
	}
}

#endif	/* PIN_DAC_PINS */

Boolean __pi_pin_adc (word p) {

	if (p >= PIN_MAX_ANALOG)
		return 0;

	return (*(byte*)
		(__PORT_FBASE__ + pinmap[p].poff + PSEL_off) >> pinmap[p].pnum)
			& 1;
}

Boolean __pi_pin_output (word p) {

	return (*(byte*)
		(__PORT_FBASE__ + pinmap[p].poff + PDIR_off) >> pinmap[p].pnum)
			& 1;
}

void __pi_pin_set (word p) {

	_BIS (*(byte*)
	    (__PORT_FBASE__ + pinmap[p].poff + POUT_off), 1 << pinmap[p].pnum);
}

void __pi_pin_clear (word p) {

	_BIC (*(byte*)
	    (__PORT_FBASE__ + pinmap[p].poff + POUT_off), 1 << pinmap[p].pnum);
}

void __pi_pin_set_input (word p) {

	__pi_clear_dac (p);
	if (p < PIN_MAX_ANALOG)
		_BIC (*(byte*)(__PORT_FBASE__ + pinmap[p].poff + PSEL_off),
			1 << pinmap[p].pnum);
	_BIC (*(byte*)
	    (__PORT_FBASE__ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void __pi_pin_set_output (word p) {

	__pi_clear_dac (p);
	if (p < PIN_MAX_ANALOG)
		_BIC (*(byte*)(__PORT_FBASE__ + pinmap[p].poff + PSEL_off),
			1 << pinmap[p].pnum);
	_BIS (*(byte*)
	    (__PORT_FBASE__ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void __pi_pin_set_adc (word p) {

	__pi_clear_dac (p);

	if (p < PIN_MAX_ANALOG) {
	    _BIC (*(byte*)(__PORT_FBASE__ + pinmap[p].poff + PDIR_off),
		1 << pinmap[p].pnum);
	    _BIS (*(byte*)(__PORT_FBASE__ + pinmap[p].poff + PSEL_off),
		1 << pinmap[p].pnum);
	}
}

#endif /* PIN_MAX */
