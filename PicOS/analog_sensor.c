/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "analog_sensor.h"
#include "pins.h"

//
// Driver for a generic analog sensor
//

//
// The interpretation of param:
//
//	bits  0-4 : the ADC pin number
//	bits  5-8 : sample hold time select
//	bits  9-10: inter sample interval: 0, 1, 4, 16 msec
//	bits 11-13: samples to average: 1, 4, 16, 64, 128, 256, 512, 1024
//	bits 14-15: reference: 1.5V, 2.5V, Vcc, Veref
//
// If EREF_ON is defined, so must be EREF_OFF; then Veref is switched on
// before the measurement, and swicthed off afterwards, to reduce power
// consumption
//

#define	ASNS_PNO	(param & 0x0f)
#define	ASNS_SHT	((param >> ASNS_SHT_SH) & 0x0f)
#define	ASNS_ISI	(asns_isi [(param >> ASNS_ISI_SH) & 0x3])
#define	ASNS_NSA	(asns_nsa [(param >> ASNS_NSA_SH) & 0x7])
#define	ASNS_REF	((param >> ASNS_REF_SH) & 0x03)

static const word asns_isi [] = { 0, 1, 4, 16 };
static const word asns_nsa [] = { 1, 4, 16, 64, 128, 256, 512, 1024 };

#ifndef	EREF_ON
#define	EREF_ON		CNOP
#define	EREF_OFF	CNOP
#endif

static	lword praa_avg;
static	int praa_count = 0;

void analog_sensor_read (word state, word param, address val) {

	if (praa_count <= 0) {
		// Starting up
		if (adc_inuse) {
			// Do not interfere
			if (state == NONE)
				// No way out
				syserror (ENOTNOW, "qso_par_read");
			delay (2, state);
			release;
		}

		EREF_ON;
		adc_config_read (ASNS_PNO, ASNS_REF, ASNS_SHT);
		praa_count = ASNS_NSA * 2;
		praa_avg = 0;
Next:
		adc_start;
		if (state != NONE) {
			delay (1, state);
			release;
		}
		udelay (10);
	}

	if ((praa_count & 1)) {
		// Return from delay -> start the ADC
		praa_count --;
		goto Next;
	}

	adc_stop;
	// Wait for the end of sampling
	while (adc_busy) {
		if (state != NONE) {
			delay (1, state);
			release;
		}
	}
	adc_off;
	praa_avg += adc_value;

	if (praa_count > 2) {
		// Must keep going
		if (state != NONE) {
			praa_count--;
			delay (ASNS_ISI, state);
			release;
		}
		mdelay (ASNS_ISI);
		praa_count -= 2;
		goto Next;
	}

	EREF_OFF;
	adc_disable;
	praa_count = 0;
	*val = (word)(praa_avg / ASNS_NSA);
}
