#ifndef	__pg_tcvplug_h
#define	__pg_tcvplug_h		1

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* The file to be included by TCV plugins                                       */
/*                                                                              */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002--2005                 */
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

#if	TCV_PRESENT
extern	tcvplug_t	*zzz_plugins [];

#if	TCV_TIMERS
void	tcvp_settimer (address, word);
void	tcvp_cleartimer (address);
#endif

int	tcvp_length (address);
int	tcvp_control (int, int, address);
void	tcvp_assign (address, int);
void	tcvp_attach (address, int);
void	tcvp_dispose (address, int);
address	tcvp_clone (address, int);
address	tcvp_new (int, int, int);

#if	TCV_HOOKS
void	tcvp_hook (address, address*);
void	tcvp_unhook (address);
#endif

/* Disposition codes */
#define	TCV_DSP_PASS	0
#define	TCV_DSP_DROP	1
#define	TCV_DSP_RCV	2
#define	TCV_DSP_RCVU	3
#define	TCV_DSP_XMT	4
#define	TCV_DSP_XMTU	5

#endif

#endif
