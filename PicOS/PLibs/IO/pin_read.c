/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "kernel.h"
#include "pinopts.h"
#include "pins.h"

//+++ pins_sys.c

word pin_read (word pin) {
/*
 * Bit 0 == pin value
 * Bit 1 == pin is out
 * Bit 2 == pin is analog (bit 1 tells whether DAC (1) or ADC (0))
 * Bit 3 == pin is unavailable
 */
	if (!__pi_pin_available (pin))
		return 8;

	// Mind the order of testing
	if (__pi_pin_dac (pin))
		return 6;			// Analog + output

	if (__pi_pin_adc (pin))
		return 4;			// Analog + input

	if (__pi_pin_output (pin))
		return __pi_pin_ovalue (pin) | 2;	// Output

	return __pi_pin_ivalue (pin);		// Input
}

int pin_write (word pin, word val) {
/*
 * Bit 0 == value to be written (if other bits are zero)
 * Bit 1 == set pin to IN
 * Bit 2 == set pin to analog (bit 1 tells whether DAC (0) or ADC (1))
 */
	if (!__pi_pin_available (pin))
		return ERROR;

	if ((val & 4)) {
#if PIN_DAC_PINS
		if ((val & 2) == 0) {
			if (!__pi_pin_dac_available (pin))
				return ERROR;
			__pi_set_dac (pin);
		} else
#endif
		{
			if (!__pi_pin_adc_available (pin))
				return ERROR;
			__pi_pin_set_adc (pin);
		}
		return 0;
	}

	if ((val & 2)) {
		// Set to digital IN
		__pi_pin_set_input (pin);
		return 0;
	}

	if (val)
		__pi_pin_set (pin);
	else
		__pi_pin_clear (pin);
	__pi_pin_set_output (pin);
	return 0;
}

int pin_read_adc (word state, word pin, word ref, word smpt) {
/*
 * pin == 0-7 (corresponds to to P6.0-P6.7)
 * ref == 0 (1.5V) or 1 (2.5V) or 2 (Vcc) or 3 (VERef+)
 * smpt == sample time in msec (0 == 1)
 */

#if PIN_MAX_ANALOG

	int res;

	if (!__pi_pin_adc_available (pin))
		return -1;

	__pi_clear_dac (pin);

	if (adc_inuse) {
#if 0
		if (adc_rcvmode)
			// We never interfere with the receiver using ADC
			return -1;
#endif
		// This means that we are in the middle of conversion and
		// we have been restarted to terminate it
		goto End;
	}

	// Initiate conversion: take over and reset the ADC
	adc_config_read (pin, ref, 0);
	adc_start;

	if (smpt == 0)
		smpt = 1;

	if (state != NONE) {
		delay (smpt, state);
		release;
	}

	mdelay (smpt);

End:
	adc_stop;
	adc_wait;
	res = (int) adc_value;
	adc_disable;

	// Restore RSSI setting
	adc_config_rssi;

	return res;
#else
	return -1;

#endif /* PIN_MAX_ANALOG */
}


int pin_write_dac (word pin, word val, word ref) {
/*
 * ref = 1 -> 3x Vref
 */
#if PIN_DAC_PINS
	if (!__pi_pin_dac_available (pin))
		return -1;
	__pi_write_dac (pin, val, ref);
	return 0;
#else
	return -1;
#endif	/* PIN_DAC_PINS */
}

#ifdef	PULSE_MONITOR

// ============================================================================
// Internal functions =========================================================
// ============================================================================

// Note: this is incompatible with eCOG, as these functions are invoked in
// interrupts. This is OK. The whole idea of PULSE_MONITOR (in its present
// complicated version) is broken and will be replaced if this thing is ever
// needed again.

void pins_int_wait_cnt (byte st, byte deb, byte on) {

	pmon.deb_cnt = deb;
	if (deb) {
		pmon.deb_mas = PMON_DEBOUNCE_UNIT;
		// If dynamic (3-timer) version
		TCI_RUN_AUXILIARY_TIMER;
	}
	if (on) 
		pin_setedge_cnt ();
	else
		pin_revedge_cnt ();
	pmon.state_cnt = st;
	pin_clrint_cnt ();
	if (pin_vedge_cnt ())
		pin_trigger_cnt ();
}

void pins_int_wait_not (byte st, byte deb, byte on) {

	pmon.deb_not = (deb);
	if (deb) {
		pmon.deb_mas = PMON_DEBOUNCE_UNIT;
		TCI_RUN_AUXILIARY_TIMER;
	}
	if (on)
		pin_setedge_not ();
	else
		pin_revedge_not ();
	pmon.state_not = (st);
	pin_clrint_not ();
	if (pin_vedge_not ())
		pin_trigger_not ();
}

// ============================================================================

void pmon_start_cnt (lint count, Boolean edge) {

	pin_disable_cnt ();

	// pin_book_cnt;

	pmon.deb_cnt = 0;

	if (edge)
		// UP
		_BIS (pmon.stat, PMON_CNT_EDGE_UP);
	else
		_BIC (pmon.stat, PMON_CNT_EDGE_UP);

	_BIC (pmon.stat, PMON_CMP_PENDING);

	pmon.state_cnt = PCS_WPULSE;

	if (count >= 0) {
		pmon.cnt [0] = (count      ) & 0xff;
		pmon.cnt [1] = (count >>  8) & 0xff;
		pmon.cnt [2] = (count >> 16) & 0xff;
	}

	pin_setedge_cnt ();

	pmon.deb_mas = PMON_RETRY_DELAY;

#if MONITOR_PINS_SEND_INTERRUPTS
	TCI_RUN_AUXILIARY_TIMER;
#endif

	_BIS (pmon.stat, PMON_CNT_ON);

	if ((pmon.stat & PMON_CMP_ON) &&
		pmon.cmp [0] == pmon.cnt [0] &&
	    	pmon.cmp [1] == pmon.cnt [1] &&
	    	pmon.cmp [2] == pmon.cnt [2] )
			_BIS (pmon.stat, PMON_CMP_PENDING);

	if (pin_vedge_cnt)
		pin_trigger_cnt ();
	else
		pin_clrint_cnt ();

	pin_enable_cnt ();
}

void pmon_dec_cnt (void) {

	lint cnt;

	cli;

	cnt = ((lword)(pmon.cnt [0]) | ((lword)(pmon.cnt [1]) << 8) |
		((lword)(pmon.cnt [2]) << 16))
		-
	      ((lword)(pmon.cmp [0]) | ((lword)(pmon.cmp [1]) << 8) |
		((lword)(pmon.cmp [2]) << 16));

	pmon.cnt [0] = (cnt      ) & 0xff;
	pmon.cnt [1] = (cnt >>  8) & 0xff;
	pmon.cnt [2] = (cnt >> 16) & 0xff;

	sti;
}

void pmon_sub_cnt (lint decr) {

	lint cnt;

	cli;

	cnt = ((lword)(pmon.cnt [0]) | ((lword)(pmon.cnt [1]) << 8) |
	      ((lword)(pmon.cnt [2]) << 16))
	      -
		decr;

	pmon.cnt [0] = (cnt      ) & 0xff;
	pmon.cnt [1] = (cnt >>  8) & 0xff;
	pmon.cnt [2] = (cnt >> 16) & 0xff;

	sti;
}

void pmon_add_cmp (lint incr) {

	cli;

	incr += ((lword)(pmon.cmp [0]) | ((lword)(pmon.cmp [1]) << 8) |
	        ((lword)(pmon.cmp [2]) << 16));

	pmon.cmp [0] = (incr      ) & 0xff;
	pmon.cmp [1] = (incr >>  8) & 0xff;
	pmon.cmp [2] = (incr >> 16) & 0xff;

	sti;
}

void pmon_stop_cnt () {

	pmon.deb_cnt = 0;
	pin_disable_cnt ();
	_BIC (pmon.stat, PMON_CNT_ON);
}

void pmon_set_cmp (lint count) {

	if (count < 0) {
		_BIC (pmon.stat, PMON_CMP_ON | PMON_CMP_PENDING);
		return;
	}

	cli;
	pmon.cmp [0] = (count      ) & 0xff;
	pmon.cmp [1] = (count >>  8) & 0xff;
	pmon.cmp [2] = (count >> 16) & 0xff;
	_BIS (pmon.stat, PMON_CMP_ON);
	_BIC (pmon.stat, PMON_CMP_PENDING);

	if (pmon.cmp [0] == pmon.cnt [0] &&
	    pmon.cmp [1] == pmon.cnt [1] &&
	    pmon.cmp [2] == pmon.cnt [2] )
		_BIS (pmon.stat, PMON_CMP_PENDING);
	sti;

}

lword pmon_get_cnt () {

	lword res;

	cli;

	res = (lword)(pmon.cnt [0]) | ((lword)(pmon.cnt [1]) << 8) |
		((lword)(pmon.cnt [2]) << 16);
	sti;

	return res;
}

Boolean pmon_pending_cmp () {

	Boolean res;
	cli;
	res = ((pmon.stat & PMON_CMP_PENDING) != 0);
	_BIC (pmon.stat, PMON_CMP_PENDING);
	pmon.deb_cnt = 0;
	sti;
	return res;
}

lword pmon_get_cmp () {

	lword res;

	res = (lword)(pmon.cmp [0]) | ((lword)(pmon.cmp [1]) << 8) |
		((lword)(pmon.cmp [2]) << 16);
	sti;

	return res;
}

void pmon_start_not (Boolean edge) {

	pin_disable_not ();

	// pin_book_not;

	pmon.deb_not = 0;

	if (edge)
		// UP
		_BIS (pmon.stat, PMON_NOT_EDGE_UP);
	else
		_BIC (pmon.stat, PMON_NOT_EDGE_UP);

	_BIC (pmon.stat, PMON_NOT_PENDING);

	pmon.state_not = PCS_WPULSE;

	pin_setedge_not ();

	if (pin_vedge_not)
		pin_trigger_not ();
	else
		pin_clrint_not ();

	_BIS (pmon.stat, PMON_NOT_ON);

	pin_enable_not ();
}

Boolean pmon_pending_not () {

	Boolean res;
	cli;
	res = ((pmon.stat & PMON_NOT_PENDING) != 0);
	_BIC (pmon.stat, PMON_NOT_PENDING);
	pmon.deb_not = 0;
	sti;
	return res;
}

void pmon_stop_not () {

	pmon.deb_not = 0;
	pin_disable_not ();
	_BIC (pmon.stat, PMON_NOT_ON | PMON_NOT_PENDING);
}

word pmon_get_state () {

	word state = 0;

	if ((pmon.stat & PMON_CNT_EDGE_UP))
		state |= PMON_STATE_CNT_RISING;

	if ((pmon.stat & PMON_NOT_EDGE_UP))
		state |= PMON_STATE_NOT_RISING;

	if ((pmon.stat & PMON_CMP_ON))
		state |= PMON_STATE_CMP_ON;

	if ((pmon.stat & PMON_NOT_ON))
		state |= PMON_STATE_NOT_ON;

	if ((pmon.stat & PMON_NOT_PENDING))
		state |= PMON_STATE_NOT_PENDING;

	if ((pmon.stat & PMON_CMP_PENDING))
		state |= PMON_STATE_CMP_PENDING;

	if ((pmon.stat & PMON_CNT_ON))
		state |= PMON_STATE_CNT_ON;

	return state;
}

#endif	/* PULSE_MONITOR */

