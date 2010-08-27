#ifndef __pg_trc_h
#define __pg_trc_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

static	char	*__procname;

#undef	process
#undef	thread
#undef	entry

#define	process(p,d)	void p (word __pi_st) { \
				d *data = (d*) __pi_curr->data; \
				__procname = #p; \
				switch (__pi_st) {

#define	thread(p)	void p (word __pi_st) { \
				__procname = #p; \
				switch (__pi_st) {

#define	entry(s)	case s:  diag ("PCS %s at " #s, __procname);

#endif
