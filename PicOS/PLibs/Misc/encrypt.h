#ifndef	__pg_encrypt_h
#define	__pg_encrypt_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__SMURPH__
#include "sysio.h"
#endif

//+++ "encrypt.c"

void encrypt (word*, int, const lword*);
void decrypt (word*, int, const lword*);

#endif
