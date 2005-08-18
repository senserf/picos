#ifndef __pg_trc_h
#define __pg_trc_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

static	char	*__procname;

#undef	process
#undef	entry

#define	process(p,d)	int p (word zz_st, address zz_da) { \
				d *data = (d*) zz_da; \
				if (zz_st == 0xffff) \
					diag ("PCS " #p " STARTED"); \
				else \
					__procname = #p; \
				switch (zz_st) {

#define	entry(s)	case s:  diag ("PCS %s at " #s, __procname);

#endif

