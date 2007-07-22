/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "adc_sampler.h"

static word	b_limit, b_in, b_out, b_overflow;
static word	*buff = NULL;
static int	consummer = 0;

#define	BSTAT_OVF	0x0001

word adcs_start (word bufsize) {

	adcs_init_regs;

	// Allocate the buffer
	if (buff != NULL)
		syserror (ETOOMANY, "adcs_start");

	// This is in words
	b_limit = bufsize * ADCS_SAMPLE_LENGTH;
	if ((buff = umalloc (b_limit * sizeof (word))) == NULL)
		return ERROR;

	b_in = b_out = b_overflow = 0;
	adcs_start_sys;

	return 0;
}

void adcs_stop () {

	if (buff != NULL) {
		ufree (buff);
		buff = NULL;
		adcs_stop_sys;
	}
}

Boolean adcs_new_sample () {
/*
 * Note: this may run asynchronously with the praxis
 */
	Boolean wake;
	word bp, i;

	wake = (b_in == b_out);

	if ((bp = b_in + ADCS_SAMPLE_LENGTH) == b_limit)
		bp = 0;

	if (bp == b_out) {
		// The buffer is full: overflow
		if (b_overflow != MAX_WORD)
			b_overflow++;
		// This may be void. On some versions of MSP430, you have to
		// read the conversion registers to clear the interrupt
		adcs_clear_sample ();
		return NO;
	}

	adcs_sample (buff + b_in);

	b_in = bp;

	if (wake && consummer) {
		p_trigger (consummer, ETYPE_USER, consummer);
		return YES;
	}
	return NO;
}

Boolean adcs_get_sample (word st, word *b) {

	word bp;

	if (st == WNONE) {
		if (b_in == b_out)
			return NO;
	} else {
		consummer = getpid ();
		cli;
		if (b_in == b_out) {
			wait (consummer, st);
			sti;
			release;
		}
		sti;
		consummer = 0;
	}

	memcpy (b, buff + b_out, ADCS_SAMPLE_LENGTH * sizeof (word));
	if ((bp = b_out + ADCS_SAMPLE_LENGTH) == b_limit)
		b_out = 0;
	else
		b_out = bp;
	return YES;
}

word adcs_overflow () {

	word ret = b_overflow;
	b_overflow = 0;
	return ret;
}
