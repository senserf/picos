/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.			*/
/* All rights reserved.							*/
/* ==================================================================== */

// +++ "net.c"
// For a stub, this is interpreted as a contingency, i.e., the stub is only
// included in the library, if the indicated file belongs there as well

#include "tarp.h"

// Stubs for the "virtual" functions of TARP needed to shut up the Cyan
// compiler

Boolean msg_isClear (msg_t a) { return YES; }
