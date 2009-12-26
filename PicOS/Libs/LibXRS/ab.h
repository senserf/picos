#ifndef	__pg_ab_h__
#define	__pg_ab_h__

//+++ "ab.c" "ab_common.c"
#include "form.h"
#include "ab_modes.h"

char *ab_outf (word, const char*, ...);
char *ab_out (word, char*);
int ab_inf (word, const char*, ...);
char *ab_in (word);

#endif
