#ifndef __pg_arch_h
#define	__pg_arch_h		1

/* ============================================================================ */
/*                       PicOS                                                  */
/*                                                                              */
/* Architecture-specific definitions                                            */
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
#include <io.h>
#include <signal.h>

typedef unsigned char	bool;
typedef	unsigned int	word;
typedef	unsigned char	byte;
typedef	unsigned long	lword;
typedef	unsigned int	field;
typedef	word		*address;

/* va_arg */
typedef	volatile address	va_list;
#define	va_par(p)		(((address)&(p)) + wsizeof(p))
#define	va_start(a,p)		((a) = va_par (p))
#define	va_arg(a,t)		(*(t *) (((a) += wsizeof (t)) - wsizeof (t)))
#define	va_end(a)		((a) = 0)

#define	byteaddr(p)		((char*)(p))

typedef struct {
	word pdmode:1,	// Power down flag
	     unused:15;
} systat_t;

extern	systat_t zz_systat;

extern void	__bss_end;
extern word	zz_malloc_length;
#define	MALLOC_START		((address)&__bss_end)
#define MALLOC_LENGTH		zz_malloc_length

extern	word	zz_restart_sp;
#define	SET_RELEASE_POINT	asm (\
		"mov r1, zz_restart_sp\n"\
		".global zz_restart_entry\n"\
		"zz_restart_entry: mov zz_restart_sp, r1")
void		zz_restart_entry ();
#define	release	zz_restart_entry ()

#define	_BIS(a,b)	(a) |= (b)
#define	_BIC(a,b)	(a) &= ~(b)

#define	nodata		do { } while (0)

#define	INLINE		inline

#endif
