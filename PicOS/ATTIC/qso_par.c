/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "kernel.h"
#include "qso_par.h"
#include "pins.h"

//
// Driver for QSO PAR sensor
//

#ifndef	QSO_PAR_PIN
#error	"QSO_PAR_PIN must be defined (QSO PAR sensor configured)"
#endif

#ifndef	QSO_PAR_SHOLD
#define	QSO_PAR_SHOLD	4	// Default sample hold time select (0-15)
#endif

#ifndef	QSO_PAR_SINT
#define	QSO_PAR_SINT	1	// Default inter sample interval (msec)
#endif

#ifndef	QSO_PAR_NSMP
#define	QSO_PAR_NSMP	512	// Default number of samples to average
#endif

#ifndef	QSO_PAR_REF	
#define	QSO_PAR_REF	0	// 1.5V by default
#endif

#ifndef	qso_set_ref
#define	qso_set_ref	CNOP
#endif

#ifndef	qso_clr_ref
#define	qso_clr_ref	CNOP
#endif

static	lword praa_avg, praa_count = 0;

void qso_par_read (word state, address val) {

	if (praa_count == 0) {
		// Starting up
		if (adc_inuse) {
			// Do not interfere
			if (state == NONE)
				// No way out
				syserror (ENOTNOW, "qso_par_read");
			delay (2, state);
			release;
		}

		qso_set_ref;
		adc_config_read (QSO_PAR_PIN, QSO_PAR_REF, QSO_PAR_SHOLD);

		praa_count = QSO_PAR_NSMP * 2;
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
			delay (QSO_PAR_SINT, state);
			release;
		}
		mdelay (QSO_PAR_SINT);
		praa_count -= 2;
		goto Next;
	}

	qso_clr_ref;
	praa_count = 0;
	adc_disable;

	*val = (word)(praa_avg / QSO_PAR_NSMP);
}
