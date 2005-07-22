#ifndef __pg_form_h
#define __pg_form_h
/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */
#include "sysio.h"

//+++ "form.c" "scan.c"

char *vform (char*, const char*, va_list);
int vscan (const char*, const char*, va_list);
char *form (char*, const char*, ...);
int scan (const char*, const char*, ...);

#define	isdigit(a)	((a) >= '0' && (a) <= '9')
#define	isspace(a)	((a)==' ' || (a)=='\t' || (a)=='\n' || (a)=='\r')
#define	isxdigit(a)	(isdigit(a)||((a)>='a'&&(a)<='f')||((a)>='A'&&(a)<='F'))
#define	hexcode(a)	(isdigit(a) ? ((a) - '0') : ( ((a)>='a'&&(a)<='f') ? ((a) - 'a' + 10) : (((a)>='A'&&(a)<='F') ? ((a) - 'A' + 10) : 0) ) )

#endif
