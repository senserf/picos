/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "dm2200.h"

typedef struct {
	byte poff, pnum;
} pind_t;

#if PULSE_MONITOR
extern	volatile zz_pmon_t zz_pmon;
#endif

#define	PSEL_off	(P3SEL_ - P3IN_)
#define	PDIR_off	(P1DIR_ - P1IN_)
#define	POUT_off	(P1OUT_ - P1IN_)
	

static const pind_t pinmap [] = {

	{ 0xff, 0 },		// GPIO_0 is reserved for RSSI

	{ P6IN_ - P1IN_, 1 },	// P6.1
	{ P6IN_ - P1IN_, 2 },	// P6.2

	{ P6IN_ - P1IN_, 3 },	// P6.3
	{ P6IN_ - P1IN_, 4 },	// P6.4
	{ P6IN_ - P1IN_, 5 },	// P6.5
	{ P6IN_ - P1IN_, 6 },	// P6.6
	{ P6IN_ - P1IN_, 7 },	// P6.7

	{ P1IN_ - P1IN_, 0 },	// CFG0 == P1.0
	{ P1IN_ - P1IN_, 1 },	// CFG1 == P1.1
#if VERSA2_TARGET_BOARD
	{ P2IN_ - P1IN_, 2 },	// CFG2 == P1.2 (will be P2.2)
#else
	{ P1IN_ - P1IN_, 2 },	// CFG2 == P1.2 (will be P2.2)
#endif
	{ 0xff, 0}		// CFG3 used for reset
};

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
