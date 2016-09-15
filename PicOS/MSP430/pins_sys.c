/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pins.h"
#include "pins_sys_in.h"
#include "pins_sys_out.h"

#ifdef PULSE_MONITOR
word		__pi_pmonevent [0];
__pi_pmon_t	__pi_pmon;
#endif

#ifdef PIN_LIST

static const pind_t pinmap [] = PIN_LIST;

#define	__pin_max (sizeof (pinmap) / sizeof (pind_t))

Boolean __pi_pin_available (word p) {

	if ((p >= __pin_max) || (pinmap[p].poff == 0xff))
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

static volatile byte *__port_sel (word p) {

	switch (p) {

#ifdef __PORT0_PRESENT__
		case 0: return &P0SEL;
#endif
#ifdef __PORT1_PRESENT__
		case 1: return &P1SEL;
#endif
#ifdef __PORT2_PRESENT__
		case 2: return &P2SEL;
#endif
#ifdef __PORT3_PRESENT__
		case 3: return &P3SEL;
#endif
#ifdef __PORT4_PRESENT__
		case 4: return &P4SEL;
#endif
#ifdef __PORT5_PRESENT__
		case 5: return &P5SEL;
#endif
#ifdef __PORT6_PRESENT__
		case 6: return &P6SEL;
#endif
#ifdef __PORT7_PRESENT__
		case 7: return &P7SEL;
#endif
#ifdef __PORT8_PRESENT__
		case 8: return &P8SEL;
#endif
#ifdef __PORT9_PRESENT__
		case 9: return &P9SEL;
#endif
#ifdef __PORT10_PRESENT__
		case 10: return &P10SEL;
#endif
#ifdef __PORT11_PRESENT__
		case 11: return &P11SEL;
#endif
	}

	return NULL;
}

static volatile byte *__port_dir (word p) {

	switch (p) {

#ifdef __PORT0_PRESENT__
		case 0: return &P0DIR;
#endif
#ifdef __PORT1_PRESENT__
		case 1: return &P1DIR;
#endif
#ifdef __PORT2_PRESENT__
		case 2: return &P2DIR;
#endif
#ifdef __PORT3_PRESENT__
		case 3: return &P3DIR;
#endif
#ifdef __PORT4_PRESENT__
		case 4: return &P4DIR;
#endif
#ifdef __PORT5_PRESENT__
		case 5: return &P5DIR;
#endif
#ifdef __PORT6_PRESENT__
		case 6: return &P6DIR;
#endif
#ifdef __PORT7_PRESENT__
		case 7: return &P7DIR;
#endif
#ifdef __PORT8_PRESENT__
		case 8: return &P8DIR;
#endif
#ifdef __PORT9_PRESENT__
		case 9: return &P9DIR;
#endif
#ifdef __PORT10_PRESENT__
		case 10: return &P10DIR;
#endif
#ifdef __PORT11_PRESENT__
		case 11: return &P11DIR;
#endif
	}

	return NULL;
}

word __pi_pin_ivalue (word p) {

	if (p >= __pin_max)
		return 0;

	return (__port_in (pinmap[p].poff) >> pinmap[p].pnum) & 1;
}

word __pi_pin_ovalue (word p) {

	volatile byte *pa;

	if (p >= __pin_max)
		return 0;

	if ((pa = __port_out (pinmap[p].poff)) != NULL)
		return ((*pa) >> pinmap[p].pnum) & 1;
	else
		return 0;
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

	volatile byte *pa;

	if (p >= PIN_MAX_ANALOG)
		return 0;

	if ((pa = __port_sel (pinmap[p].poff)) != NULL)
		return ((*pa) >> pinmap[p].pnum) & 1;
	else
		return 0;
}

Boolean __pi_pin_output (word p) {

	volatile byte *pa;

	if ((pa = __port_dir (pinmap[p].poff)) != NULL)
		return ((*pa) >> pinmap[p].pnum) & 1;
	else
		return 0;
}

void __pi_pin_set (word p) {

	volatile byte *pa;

	if ((pa = __port_out (pinmap[p].poff)) != NULL)
		_BIS (*pa, 1 << pinmap[p].pnum);
}

void __pi_pin_clear (word p) {

	volatile byte *pa;

	if ((pa = __port_out (pinmap[p].poff)) != NULL)
		_BIC (*pa, 1 << pinmap[p].pnum);
}

void __pi_pin_set_input (word p) {

	volatile byte *pa;

	__pi_clear_dac (p);
#if PIN_MAX_ANALOG > 0
	if (p < PIN_MAX_ANALOG) {
		if ((pa = __port_sel (pinmap[p].poff)) != NULL)
			_BIC (*pa, 1 << pinmap[p].pnum);
	}
#endif
	if ((pa = __port_dir (pinmap[p].poff)) != NULL)
		_BIC (*pa, 1 << pinmap[p].pnum);
}

void __pi_pin_set_output (word p) {

	volatile byte *pa;

	__pi_clear_dac (p);
#if PIN_MAX_ANALOG > 0
	if (p < PIN_MAX_ANALOG) {
		if ((pa = __port_sel (pinmap[p].poff)) != NULL)
			_BIC (*pa, 1 << pinmap[p].pnum);
	}
#endif
	if ((pa = __port_dir (pinmap[p].poff)) != NULL)
		_BIS (*pa, 1 << pinmap[p].pnum);
}

void __pi_pin_set_adc (word p) {

	volatile byte *pa;

	__pi_clear_dac (p);
#if PIN_MAX_ANALOG > 0
	if (p < PIN_MAX_ANALOG) {
		if ((pa = __port_dir (pinmap[p].poff)) != NULL)
			_BIC (*pa, 1 << pinmap[p].pnum);
		if ((pa = __port_sel (pinmap[p].poff)) != NULL)
			_BIS (*pa, 1 << pinmap[p].pnum);
	}
#endif
}

#endif /* __pin_max */
