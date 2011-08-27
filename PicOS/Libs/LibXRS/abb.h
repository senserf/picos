#ifndef	__pg_abb_h__
#define	__pg_abb_h__

//+++ "abb.c" "ab_common.c"
#include "ab_modes.h"

//+++
byte *abb_outf (word, word);
byte *abb_out (word, byte*, word);
byte *abb_in (word, word*);

#endif
