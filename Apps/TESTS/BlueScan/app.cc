/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "bluescan.h"

static char mcode [13];

void static mac_encode (bscan_item_t *p) {

	int i, j;

	for (i = j = 0; i < 6; i++) {
		mcode [j++] = HEX_TO_ASCII (p->mac [i] >> 4);
		mcode [j++] = HEX_TO_ASCII (p->mac [i]     );
	}
}

fsm root {

	bscan_item_t *p;
	byte cnt;

	state INIT:

		ser_out (INIT, "Starting\r\n");
		bscan_start (NULL);

	state WAIT_NEW:

		when (BSCAN_EVENT, PROCESS);
		release;

	state PROCESS:

		for_all_bscan_entries (p)
			if (bscan_pending (p))
				sameas REPORT;

		// Nothing to report
		sameas WAIT_NEW;

	state REPORT:

		// This also resets the pending status of this entry
		cnt = bscan_counter (p);
		mac_encode (p);

	state LINE_OUT:

		ser_outf (LINE_OUT, "Time: %ld <%s> %s [%s]\r\n",
			seconds (),
			cnt ? "NEW" : "GONE",
			mcode,
			p->name);

		// New items may have become pending in the meantime
		proceed PROCESS;
}
