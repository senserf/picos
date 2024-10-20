/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_abb_h__
#define	__pg_abb_h__

//+++ "abb.cc" "ab_common.cc"
#include "ab_modes.h"

byte *abb_outf (word, word);
byte *abb_out (word, byte*, word);
byte *abb_in (word, word*);

#endif
