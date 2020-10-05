/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_encrypt_h
#define	__pg_encrypt_h

#ifndef	__SMURPH__
#include "sysio.h"
#endif

//+++ "encrypt.c"

void encrypt (word*, int, const lword*);
void decrypt (word*, int, const lword*);

#endif
