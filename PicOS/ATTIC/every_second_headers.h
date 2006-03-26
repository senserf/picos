#ifndef __pg_every_second_headers_h
#define __pg_every_second_headers_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
 * Room for headers for intrinsic extre code to be executed every second
 */

#if TARGET_BOARD == BOARD_GENESIS
#include "every_second_headers_genesis.h"
#endif

#if TARGET_BOARD == BOARD_VERSA2
#include "every_second_headers_versa2.h"
#endif

#endif
