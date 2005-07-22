/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tarp.h"

word tarp_getHco (lword what) {

        if (tarp_findInSpd(what))
	       return spdCache->repos[spdCache->last].hco | (tarp_level <<12);
	return tarp_level << 12;
}
