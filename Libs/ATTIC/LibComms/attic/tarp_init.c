/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "tarp.h"

// some TARP globals
ddCacheType  * ddCache		= NULL;
spdCacheType * spdCache		= NULL;


void tarp_init() {

	ddCache = (ddCacheType *) 
		umalloc (ddCacheSize * sizeof(ddCacheType));
	if (ddCache == NULL)
		syserror (EMALLOC, "ddCache");

	spdCache = (spdCacheType *) umalloc (sizeof(spdCacheType));
	if (spdCache == NULL)
		syserror (EMALLOC, "spdCache");

	memset (ddCache, 0, ddCacheSize * sizeof(ddCacheType));
	memset (spdCache, 0, sizeof(spdCacheType));

}
