/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"

static const pind_t pinmap [] = PIN_LIST;

#if PULSE_MONITOR
word		zz_pmonevent [0];
zz_pmon_t	zz_pmon;
#endif

bool zz_pin_available (word p) {

	if ((p >= PIN_MAX) || (pinmap[p].poff == 0xff))
		return NO;

#if PULSE_MONITOR

	if (p == 1 && (pmon.stat & PMON_CNT_ON) != 0)
		return NO;

	if (p == 2 && (pmon.stat & PMON_NOT_ON) != 0)
		return NO;
#endif
	return YES;
}

bool zz_pin_adc_available (word p) {

	if (!zz_pin_available (p) || p >= PIN_MAX_ANALOG)
		return NO;

	return YES;
}

word zz_pin_ivalue (word p) {

	if (p >= PIN_MAX)
		return 0;
	return (*(byte*)(P1IN_ + pinmap[p].poff) >> pinmap[p].pnum) & 1;
}

word zz_pin_ovalue (word p) {

	if (p >= PIN_MAX)
		return 0;
	return (*(byte*)(P1IN_ + pinmap[p].poff + POUT_off) >> pinmap[p].pnum)
		& 1;
}

#if PIN_DAC_PINS != 0

bool zz_pin_dac_available (word p) {

	if (p != (PIN_DAC_PINS & 0xf) && p != ((PIN_DAC_PINS >> 8) & 0xf))
		return NO;

	return YES;
}

bool zz_pin_dac (word p) {

	if (p != (PIN_DAC_PINS & 0xf) && p != ((PIN_DAC_PINS >> 8) & 0xf))
		// Up to two DAC pins are handled at the moment
		return 0;

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		return (DAC12_0CTL & DAC12AMP_7) != 0;
	}

	return (DAC12_1CTL & DAC12AMP_7) != 0;
}

void zz_clear_dac (word p) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		DAC12_0CTL = 0;	// Disable DAC0
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		DAC12_1CTL = 0;
	}
}

void zz_set_dac (word p) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		dac_config_write (0, 0, 0);
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		dac_config_write (1, 0, 0);
	}
}

void zz_write_dac (word p, word val, word ref) {

	if (p == (PIN_DAC_PINS & 0xf)) {
		// DAC0
		dac_config_write (0, val, ref);
	} else if (p == ((PIN_DAC_PINS >> 8) & 0xf)) {
		// DAC1
		dac_config_write (1, val, ref);
	}
}

#endif	/* PIN_DAC_PINS */

bool zz_pin_adc (word p) {

	if (p >= PIN_MAX_ANALOG)
		return 0;

	return (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off) >> pinmap[p].pnum)
		& 1;
}

bool zz_pin_output (word p) {

	return (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off) >> pinmap[p].pnum)
		& 1;
}

void zz_pin_set (word p) {

	_BIS (*(byte*)(P1IN_ + pinmap[p].poff + POUT_off), 1 << pinmap[p].pnum);
}

void zz_pin_clear (word p) {

	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + POUT_off), 1 << pinmap[p].pnum);
}

void zz_pin_set_input (word p) {

	zz_clear_dac (p);
	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off), 1 << pinmap[p].pnum);
	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void zz_pin_set_output (word p) {

	zz_clear_dac (p);
	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off), 1 << pinmap[p].pnum);
	_BIS (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void zz_pin_set_adc (word p) {

	zz_clear_dac (p);

	if (p < PIN_MAX_ANALOG) {
	    _BIC (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off),
		1 << pinmap[p].pnum);
	    _BIS (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off),
		1 << pinmap[p].pnum);
	}
}
