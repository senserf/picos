/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-03   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

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

