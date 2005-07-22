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

#include <ecog.h>
#include <ecog1.h>

#define	SDRAM_ADDR		0x4000	/* SDRAM location in data space */
#define	SDRAM_PHYSICAL		0x0000	/* SDRAM physical block */
#define	ETHER_ADDR		0x3c00
#define	SDRAM_SIZE		15	/* This is a power of 2 (15 == 32K) */
#define	SDRAM_END		0x800000L
#define	SDRAM_BLKSIZE		((word)1 << SDRAM_SIZE)
#define	SDRAM_NBLOCKS		(SDRAM_END / SDRAM_BLKSIZE)
#define	SDRAM_SPARE		(SDRAM_END - SDRAM_BLKSIZE)
#define	SDRAM_CPBUFSIZE		32

typedef unsigned char	bool;
typedef	unsigned int	word;
typedef	unsigned char	byte;
typedef	unsigned long	lword;
typedef	unsigned int	field;
typedef	word		*address;

/* va_arg */
typedef	address		va_list;
#define	va_par(p)	(((address)&(p)) + wsizeof(p))
#define	va_start(a,p)	((a) = va_par (p))
#define	va_arg(a,t)	(*(t *) (((a) = (a) + wsizeof (t)) -  wsizeof (t)))
#define	va_end(a)	((a) = 0)

/* ====================================================================== */
/* Byte pointer conversion. I have changed this to be less nonsensical by */
/* avoiding storing pointers as integers.  This is what type 'address' is */
/* intended for.  When you cast a non-char pointer to char*, the compiler */
/* will shift it for you.                                                 */
/* ====================================================================== */
//#define	byteaddr(p)	((char*)(((long)(p)) << 1))
#define	byteaddr(p)	((char*)(p))

#if	SDRAM_PRESENT
/* malloc is using exclusively SDRAM */
#define	MALLOC_START		((address)SDRAM_ADDR)
#define MALLOC_LENGTH	((word) (((word)1) << SDRAM_SIZE))
#else
#define	MALLOC_START		evar_
#if	STACK_GUARD
#define	MALLOC_LENGTH	((word)estk_ - (word)evar_ - 1)
#else
#define	MALLOC_LENGTH	((word)estk_ - (word)evar_)
#endif	/* STACK_GUARD */
#endif	/* SDRAM_PRESENT */

/* Data/stack boundaries */
extern	address evar_, estk_;

#if	SDRAM_PRESENT || ETHERNET_DRIVER
#define	EMI_USED		1
#else
#define	EMI_USED		0
#endif

/* Exit to scheduler */
void 				zzz_set_release (void);
#define	SET_RELEASE_POINT	zzz_set_release ();
void				zzz_release (void);
#define	release			zzz_release ()

#define	nodata			(*((int*)&data) = 0)

#define	INLINE

#endif
