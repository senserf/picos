/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tarp.h"
#include "diag.h"

bool tarp_findInSpd (lword what) {
	word i;

	if (what == 0)
		return false;

	if (spdCache->repos[spdCache->last].id == what) { // same as last

		net_diag (D_DEBUG, "Spd double hit (%lu)", what);
		return true;
	
	}

	for (i = 0; i < spdCacheSize; i++) {
		if (spdCache->free == i) // free entry (zero)
			return false;

		if (spdCache->repos[i].id == what) {
			spdCache->last = i;
			return true;
		}
	}

	net_diag (D_SERIOUS, "Spd full");
	return false;
}
