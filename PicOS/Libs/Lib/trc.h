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

#define	process(p,d)	int p (word __pi_st, address __pi_da) { \
				d *data = (d*) __pi_da; \
				if (__pi_st == 0xffff) \
					diag ("PCS " #p " STARTED"); \
				else \
					__procname = #p; \
				switch (__pi_st) {

#define	thread(p)	int p (word __pi_st, address __pi_dummy) { \
				if (__pi_st == 0xffff) \
					diag ("PCS " #p " STARTED"); \
				else \
					__procname = #p; \
				switch (__pi_st) {

#define	entry(s)	case s:  diag ("PCS %s at " #s, __procname);

#endif

