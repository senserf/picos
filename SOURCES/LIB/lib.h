/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

/* --- */

/* ------------------------------------------------------------- */
/* SMURPH version of the C++ library (the minimum needed subset) */
/* ------------------------------------------------------------- */

#define	OBUFSIZE  256		// Size of an ostream buffer

#define	_GNU_SOURCE	1

#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<math.h>
#include	<stdarg.h>
#include	<setjmp.h>
#include	<netinet/in.h>
#include	"sxml.h"

#ifndef	HUGE
#define	HUGE	HUGE_VAL
#endif

#include	<iostream>
#include	<fstream>

using std::cin;
using std::cout;
using std::cerr;
using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;

char *form (const char*, ...);
char *vform (const char*, va_list);

/* =============== */
/* A few constants */
/* =============== */
#define		SOL_VACUUM		299792458.0	/* m/s */
#define		SOL_FIBER		214100000.0
#define		SOL_COAX_SLOW		197340000.0
#define		SOL_COAX_FAST		235210000.0
#define		SOL_COAX		((SOL_COAX_SLOW + SOL_COAX_FAST) / 2.0)

#ifndef	PATH_MAX
#define	PATH_MAX	2048		/* A safe bet */
#endif
