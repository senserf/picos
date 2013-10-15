/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-13   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

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
