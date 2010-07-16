/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "adc_sampler.h"

static word	b_limit, b_in, b_out, b_overflow;

#ifndef	ADCS_SAMPLE_BLOCK
#define	ADCS_SAMPLE_BLOCK	1
#endif

#if ADCS_SAMPLE_BLOCK == 0
#undef	ADCS_SAMPLE_BLOCK
#define	ADCS_SAMPLE_BLOCK	1
#endif

#if ADCS_SAMPLE_BLOCK > 1
static word	b_blkptr, b_newin;
#endif

static word	*buff = NULL;
static int	consummer = 0;

#define	BSTAT_OVF	0x0001

word adcs_start (word bufsize) {

	adcs_init_regs;

	// Allocate the buffer
	if (buff != NULL)
		syserror (ETOOMANY, "adcs_start");

	// This is in words
	b_limit = bufsize * ADCS_SAMPLE_LENGTH * ADCS_SAMPLE_BLOCK;
	if ((buff = umalloc (b_limit * sizeof (word))) == NULL)
		return ERROR;

	b_in = b_out = b_overflow = 0;
#if ADCS_SAMPLE_BLOCK > 1
	b_blkptr = 0;
#endif
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
	word bp;

#if ADCS_SAMPLE_BLOCK > 1

	if (b_blkptr == 0) {
		b_newin = b_in;
		// Full condition
		if ((bp = b_in + ADCS_SAMPLE_LENGTH * ADCS_SAMPLE_BLOCK)
		    == b_limit)
			bp = 0;

		if (bp == b_out) {
			// Overflow
			if (b_overflow != MAX_WORD)
				b_overflow++;
			adcs_clear_sample ();
			return NO;
		}
	}

	adcs_sample (buff + b_newin);
	b_newin += ADCS_SAMPLE_LENGTH;

	if (b_blkptr == ADCS_SAMPLE_BLOCK - 1) {
		// The last block
		b_blkptr = 0;
		if ((b_in == b_out) && consummer) {
			p_trigger (consummer, consummer);
			b_in = (b_newin == b_limit) ? 0 : b_newin;
			return YES;
		}
		b_in = (b_newin == b_limit) ? 0 : b_newin;
		return NO;
	}

	b_blkptr++;
	return NO;

#else
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

	if ((b_in == b_out) && consummer) {
		p_trigger (consummer, consummer);
		b_in = bp;
		return YES;
	}

	b_in = bp;
	return NO;
#endif

}

Boolean adcs_get_sample (word st, word *b) {

	word bp;

	if (st == WNONE) {
		if (b_in == b_out)
			return NO;
	} else {
		consummer = getcpid ();
		cli;
		if (b_in == b_out) {
			wait (consummer, st);
			sti;
			release;
		}
		sti;
		consummer = 0;
	}

	memcpy (b, buff + b_out, ADCS_SAMPLE_BLOCK * ADCS_SAMPLE_LENGTH *
								sizeof (word));
	if ((bp = b_out + ADCS_SAMPLE_LENGTH * ADCS_SAMPLE_BLOCK) == b_limit)
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
