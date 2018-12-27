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

/* ------------------------------- */
/* Internal offsets for RVariables */
/* ------------------------------- */

#define MAX_MOMENTS     32   // Should content everybody
#define MAX_REGION      24   // The number of last RV mean samples for display

#if     BIG_precision > 0
#define BIG_SIZE        BIG_precision
#else
#define BIG_SIZE        ((sizeof(double) + sizeof(LONG) - 1) / sizeof (LONG))
#endif

#define STYPE                           0               // The type
#define COUNTER                         1               // The counter
#define MIN             ((sizeof(LONG)*2+sizeof(double)-1)/sizeof(double))
#define MAX             (MIN+1)                         // Min/ Max
#define MOMENT          (MAX+1)                         // Moments

/* ----------------------- */
/* For BIG type RVariables */
/* ----------------------- */
#define BCOUNTER        1

#define BMIN ((sizeof(LONG)*(BCOUNTER+BIG_SIZE)+sizeof(double)-1)/sizeof(double))
#define BMAX            (BMIN+1)
#define BMOMENT         (BMAX+1)

/* ---------------------------------------- */
/* Macros to reference RVariable attributes */
/* ---------------------------------------- */
#define stype(s)        ((s)[STYPE])
#define counter(s)      ((s)[COUNTER])
#define minimum(s)      (((double*) (s))[MIN])
#define maximum(s)      (((double*) (s))[MAX])
#define moment(s,i)     (((double*) (s))[MOMENT+(i)])

/* --------------------------------- */
/* The same for a BIG-type RVariable */
/* --------------------------------- */
#define bcounter(s)     (*(BIG*)(&((s)[BCOUNTER])))
#define bminimum(s)     (((double*) (s))[BMIN])
#define bmaximum(s)     (((double*) (s))[BMAX])
#define bmoment(s,i)    (((double*) (s))[BMOMENT+(i)])

/* ------------------------------------------------------ */
/* Values of Z_alpha for calculating confidence intervals */
/* ------------------------------------------------------ */
#define ZALPHA95        1.960
#define ZALPHA99        2.575
