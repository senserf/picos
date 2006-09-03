#ifndef __pg_arch_h
#define	__pg_arch_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Architecture-specific definitions                                          */
/*                                                                            */

#include <ecog.h>
#include <ecog1.h>

#define	ETHER_ADDR		0x3c00

#define	SDRAM_CPBUFSIZE		32
#define	SDRAM_ADDR		0x4000	/* SDRAM location in data space */
#define	SDRAM_SIZE		15	/* This is a power of 2 (15 == 32K) */
#define	SDRAM_BLKSIZE		((word)1 << SDRAM_SIZE)

#include "emi.h"

#define	SDRAM_NBLOCKS		(SDRAM_END / SDRAM_BLKSIZE)
#define	SDRAM_SPARE		(SDRAM_END - SDRAM_BLKSIZE)

typedef unsigned char	bool;
typedef	unsigned int	word;
typedef	int		sint;
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

typedef struct {

	word	pdmode:1,	// Power down flag (unused on eCOG)
		evntpn:1,	// Scheduler event pending
		fstblk:1,	// Fast blink flag
		ledblk:1,	// Blink flag
		ledsts:4;	// Blink status of four leds

		// Lots of room to spare

	byte	ledblc;		// Blink counter (one byte needed)

} systat_t;

extern	systat_t zz_systat;
		
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

#define	STATIC_LENGTH	((word)evar_ - (word)RAM_START)

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
