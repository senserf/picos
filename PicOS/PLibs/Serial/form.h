/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_form_h
#define __pg_form_h
#include "sysio.h"

//+++ "form.c" "scan.c"

word __pi_vfparse (char*, word, const char*, va_list);
char *vform (char*, const char*, va_list);
int vscan (const char*, const char*, va_list);
char *form (char*, const char*, ...);
word fsize (const char*, ...);
int scan (const char*, const char*, ...);

#define vfsize(a,b)	__pi_vfparse (NULL, 0, a, b)

#define	isdigit(a)	((a) >= '0' && (a) <= '9')
#define	isspace(a)	((a)==' ' || (a)=='\t' || (a)=='\n' || (a)=='\r')
#define	isxdigit(a)	(isdigit(a)||((a)>='a'&&(a)<='f')||((a)>='A'&&(a)<='F'))
#define	hexcode(a)	(isdigit(a) ? ((a) - '0') : ( ((a)>='a'&&(a)<='f') ? ((a) - 'a' + 10) : (((a)>='A'&&(a)<='F') ? ((a) - 'A' + 10) : 0) ) )
#define	isalpha(a)	(((a) >= 'A' && (a) <= 'Z') || ((a) >= 'a' && (a) <= 'z') || (a) == '_')
#define	isalnum(a)	(isdigit (a) || isalpha (a))
#define tolower(a)	(((a) >= 'A' && (a) <= 'Z') ? ((a) - 'A') + 'a' : (a))

#endif
