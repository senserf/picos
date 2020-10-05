/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_trc_h
#define __pg_trc_h

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
