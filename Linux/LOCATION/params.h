/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __params_h
#define	__params_h

#include "locengine.h"
#include "sxml.h"
#include <stdarg.h>
#include <string.h>

#define	DEBUGGING 0

// ============================================================================
// Parameters
// ============================================================================

extern u32	PM_dbver;			// DB version

extern float	*PM_rts_v;
extern u16	*PM_rts_a;
extern int	PM_rts_n;

extern u32	PM_dis_min;
extern float	PM_dis_fac;
extern int	PM_dis_tag;
extern float	PM_dis_taf;

extern u32	PM_sel_min, PM_sel_max;
extern float	PM_sel_fac;

extern int	PM_ave_for;
extern float	PM_ave_fac;

// ============================================================================

void abt (const char*, ...);

int getint (char**, long*);
int getdouble (char**, double*);
int findint (char**, long*);
int finddouble (char**, double*);

#define	getu32(a,b) getint (a, (long*)(b))
#define	findu32(a,b) findint (a, (long*)(b))

static inline int getfloat (char **lp, float *v) {
	double dv;
	int r;
	r = getdouble (lp, &dv);
	*v = (float) dv;
	return r;
}

static inline int findfloat (char **lp, float *v) {
	double dv;
	int r;
	r = finddouble (lp, &dv);
	*v = (float) dv;
	return r;

};

void set_params (const char*);
void get_db_version (char*);

#endif
