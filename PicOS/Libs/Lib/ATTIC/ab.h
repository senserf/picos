#ifndef	__pg_ab_h__
#define	__pg_ab_h__

//+++ "ab.c"
#include "form.h"

void ab_init (int);
void ab_on ();
void ab_off ();
void ab_outf (word, const char*, ...);
int ab_inf (word, const char*, ...);
char *ab_in (word);

#endif
