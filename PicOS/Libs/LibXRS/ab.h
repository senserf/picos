#ifndef	__pg_ab_h__
#define	__pg_ab_h__

//+++ "ab.c"
#include "form.h"

void ab_init (int);
void ab_outf (word, const char*, ...);
void ab_out (word, char*);
int ab_inf (word, const char*, ...);
char *ab_in (word);
void ab_mode (byte);

#define	AB_MODE_HOLD	0
#define	AB_MODE_PASSIVE	1
#define	AB_MODE_ACTIVE	2

#endif
