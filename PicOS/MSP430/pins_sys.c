/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"

static const pind_t pinmap [] = PIN_LIST;

word		zz_pmonevent [0];
zz_pmon_t	zz_pmon;

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

word zz_pin_value (word p) {

	if (p >= PIN_MAX)
		return 0;
	return (*(byte*)(P1IN_ + pinmap[p].poff) >> pinmap[p].pnum) & 1;
}

bool zz_pin_analog (word p) {

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

void zz_pin_setinput (word p) {

	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off), 1 << pinmap[p].pnum);
	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void zz_pin_setoutput (word p) {

	_BIC (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off), 1 << pinmap[p].pnum);
	_BIS (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off), 1 << pinmap[p].pnum);
}

void zz_pin_setanalog (word p) {

	if (p < PIN_MAX_ANALOG) {
	    _BIC (*(byte*)(P1IN_ + pinmap[p].poff + PDIR_off),
		1 << pinmap[p].pnum);
	    _BIS (*(byte*)(P1IN_ + pinmap[p].poff + PSEL_off),
		1 << pinmap[p].pnum);
	}
}
