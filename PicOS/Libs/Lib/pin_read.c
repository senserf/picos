/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "pinopts.h"
#include "pins.h"

//+++ pins_sys.c

word pin_read (word pin) {
/*
 * Bit 0 == pin value
 * Bit 1 == pin is out
 * Bit 2 == pin is analog in
 * Bit 3 == pin is unavailable
 */
	word res;
	byte pm;

	res = pin_value (pin);

	if (!pin_available (pin))
		return res | 8;

	if (pin_analog (pin))
		// Analog
		return res | 4;

	if (pin_output (pin))
		return res | 2;

	return res;
}

void pin_write (word pin, word val) {
/*
 * Bit 0 == value to be written (if other bits are zero)
 * Bit 1 == set pin to IN
 * Bit 2 == set pin to analog in
 */
	word pm;

	if (!pin_available (pin))
		return;

	if ((val & 4)) {
		// Set to analog input
		pin_setanalog (pin);
		return;
	}

	if ((val & 2)) {
		// Set to digital IN
		pin_setinput (pin);
		return;
	}

	pin_setvalue (pin, val);
	pin_setoutput (pin);
}

int pin_read_adc (word state, word pin, word ref, word smpt) {
/*
 * pin == 0-7 (corresponds to to P6.0-P6.7)
 * ref == 0 (1.5V) or 1 (2.5V) or 2 (Vcc)
 * smpt == sample time in msec (0 == 1)
 */
	int res;

	if (!pin_available (pin))
		return -1;

	if (adc_inuse) {
		if (adc_rcvmode)
			// We never interfere with the receiver using ADC
			return -1;
		// This means that we are in the middle of conversion and
		// we have been restarted to terminate it
		goto End;
	}

	// Initiate conversion: take over and reset the ADC
	adc_config_read (pin, ref);
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

	// Restore RSSI setting
	adc_config_rssi;

	return res;
}

#if	PULSE_MONITOR

void pmon_start_cnt (long count, bool edge) {

	pin_disable_cnt;

	pin_book_cnt;

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

	pin_setedge_cnt;

	pmon.deb_mas = PMON_RETRY_DELAY;

	_BIS (pmon.stat, PMON_CNT_ON);

	if ((pmon.stat & PMON_CMP_ON) &&
		pmon.cmp [0] == pmon.cnt [0] &&
	    	pmon.cmp [1] == pmon.cnt [1] &&
	    	pmon.cmp [2] == pmon.cnt [2] )
			_BIS (pmon.stat, PMON_CMP_PENDING);

	if (pin_vedge_cnt)
		pin_trigger_cnt;
	else
		pin_clrint_cnt;

	pin_enable_cnt;
}

void pmon_dec_cnt (void) {

	long cnt;

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

void pmon_sub_cnt (long decr) {

	long cnt;

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

void pmon_add_cmp (long incr) {

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
	pin_disable_cnt;
	pin_release_cnt;
	_BIC (pmon.stat, PMON_CNT_ON);
}

void pmon_set_cmp (long count) {

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

bool pmon_pending_cmp () {

	bool res;
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

void pmon_start_not (bool edge) {

	pin_disable_not;

	pin_book_not;

	pmon.deb_not = 0;

	if (edge)
		// UP
		_BIS (pmon.stat, PMON_NOT_EDGE_UP);
	else
		_BIC (pmon.stat, PMON_NOT_EDGE_UP);

	_BIC (pmon.stat, PMON_NOT_PENDING);

	pmon.state_not = PCS_WPULSE;

	pin_setedge_not;

	if (pin_vedge_not)
		pin_trigger_not;
	else
		pin_clrint_not;

	_BIS (pmon.stat, PMON_NOT_ON);

	pin_enable_not;
}

bool pmon_pending_not () {

	bool res;
	cli;
	res = ((pmon.stat & PMON_NOT_PENDING) != 0);
	_BIC (pmon.stat, PMON_NOT_PENDING);
	pmon.deb_not = 0;
	sti;
	return res;
}

void pmon_stop_not () {

	pmon.deb_not = 0;
	pin_disable_not;
	pin_release_not;
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
