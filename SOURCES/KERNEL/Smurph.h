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

/* --------------------------------------- */
/* Standard include file for user protocol */
/* --------------------------------------- */

#include        "global.h"

/* --------------------------------------- */
/* Global variables (external definitions) */
/* --------------------------------------- */

extern  ZZ_KERNEL              *Kernel;         // The system process

/* ----------- */
/* Environment */
/* ----------- */

#if	ZZ_NFP
#else
extern  double                 Itu, Etu;
#endif

extern  int                    TheObserverState;       // The observer state

/* --------- */
/* Debugging */
/* --------- */

extern  int		EndOfData;              // EOF flag

#if ZZ_DBG
extern  int             DebugTracingFull;       // Full debug trace flag
#endif

/* ---------------------- */
/* Display (visible part) */
/* ---------------------- */

extern  int          DisplayInterval,           // Display interval
		     DisplayOpening,            // Set for a new findow
		     DisplayClosing;            // The window is being closed

/* ---------------- */
/* Standard streams */
/* ---------------- */

extern  istream       *zz_ifpp;                 // Input stream
extern  ostream       *zz_ofpp;                 // Output file

