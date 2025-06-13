/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

/* SIDE version number */
#define  VERSION      "3.9-AG"

#ifdef	__CYGWIN__
// 231206: I am adding this (back) because neither select nor pselect work for
// me on Cygwin (they don't delay, which amounts to spinning). With this,
// sockets are actively polled (at SOCKCHKINT intervals).
// This used to be the case some time ago; then, with the advent of 64-bit
// Cygwin, I reverted to select without paying attention to the spinning
// CPU (one core anyway) which has started to bother me recently ;-).
// 
#define	ZZ_CYW				1
#endif

#ifdef	__CYGWIN32__
#define	ZZ_ARCH_EXPL_SM

#define		MAXSHORT		((short int)0x7fff)
#define		MINSHORT		((short int)0x8000)
#define		MAXINT			((int)0x7fffffff)
#define		MININT			((int)0x80000000)
#define		MAXLONG			((long)MAXINT)
#define		MINLONG			((long)MININT)
#define		LONGBITS		32
#define		INTBITS			32
#define		PTRBITS			32

#endif	/* i386 Cygwin */

#ifdef	__i386__
#define	ZZ_ARCH_EXPL_SM

#define		MAXSHORT		((short int)0x7fff)
#define		MINSHORT		((short int)0x8000)
#define		MAXINT			((int)0x7fffffff)
#define		MININT			((int)0x80000000)
#define		MAXLONG			((long)MAXINT)
#define		MINLONG			((long)MININT)
#define		LONGBITS		32
#define		INTBITS			32
#define		PTRBITS			32

#endif	/* i386 Linux */

#ifdef	__x86_64__
#define	ZZ_ARCH_EXPL_SM

#define		MAXSHORT		((short int)0x7fff)
#define		MINSHORT		((short int)0x8000)
#define		MAXINT			((int)0x7fffffff)
#define		MININT			((int)0x80000000)
#define		MAXLONG			((long)0x7fffffffffffffff)
#define		MINLONG			((long)0x8000000000000000)
#define		LONGBITS		64
#define		INTBITS			32
#define		PTRBITS			64

#endif	/* 64 bit */

#ifndef	ZZ_ARCH_EXPL_SM
#include	<stdio.h>
#include	<values.h>
#endif

/* --------------------------------------------------- */
/* Constants  of  standard  types  (machine-dependent) */
/* --------------------------------------------------- */
#define         MAX_short               MAXSHORT
#define         MIN_short               MINSHORT
#define         MAX_int                 MAXINT
#define         MIN_int                 MININT
#define         MAX_long                MAXLONG
#define         MIN_long                MINLONG

#if	LONGBITS <= 32

#define 	ZZ_LONG_is_not_long 	1
typedef		long long		LONG;
typedef		unsigned long long	U_LONG;
typedef		int			IPointer;
typedef		LONG			LPointer;
typedef		long			Long;
typedef		unsigned long		U_Long;
#define		MAX_LONG	((LONG)(((U_LONG)1<<(LONGBITS+LONGBITS-1))-1))
#define		MIN_LONG	((LONG)( (U_LONG)1<<(LONGBITS+LONGBITS-1))   )
#define         MAX_Long                MAX_long
#define		MIN_Long		MIN_long
#define		ZZ_XLong		"long"     /* For SMPP */

#else

typedef		long			LONG;
typedef		unsigned long		U_LONG;
#define		MAX_LONG		MAX_long
#define		MIN_LONG		MIN_long
/* Honorary long, must be 32-bits long, but doesn't have to be longer */
#define		ZZ_Long_eq_int		1
typedef		int			Long;
typedef		unsigned int		U_Long;
#define         MAX_Long                MAX_int
#define		MIN_Long		MIN_int
#define		ZZ_XLong		"int"      /* For SMPP */

/* Pointer type to be used in arithmetic */
#if PTRBITS == INTBITS
typedef		int			IPointer;
#else
#if PTRBITS == LONGBITS
typedef		LONG			IPointer;
#else
ERROR: unsupported pointer size
#endif
#endif	/* PTRBITS == INTBITS */

/* A type capable of storing both pointers and LONG numbers */
#if LONGBITS > PTRBITS
typedef		LONG			LPointer;
#else
typedef		IPointer		LPointer;
#endif

#endif	/* LONGBITS <= 32 */

// ============================================================================
#if PTRBITS == LONGBITS
// A single cast to Long/int causes a compilation error on 64-bit systems
#define	ptrToLong(a)			((Long)(LONG)(a))
#define	ptrToInt(a)			((int)(LONG)(a))
#else
#define	ptrToLong(a)			((Long)(a))
#define	ptrToInt(a)			((int)(a))
#endif
// ============================================================================

#ifndef	BCPCASTS	
#ifdef	ZZ_CYW
#define	BCPCASTS	const char*
#define BCPCASTD	char*
#else
#define	BCPCASTS	const void*
#define BCPCASTD	void*
#endif
#endif

typedef void (*SIGARG) (int);

/* The following defs are inserted by maker. Everything starting with the */
/* next line will be removed and written from scratch.                    */
#define  ZZ_SOURCES      "/cygdrive/c/Users/nripg/OneDrive/STORAGE/SOFTWARE/PICOS/SIDE/SOURCES"
#define  ZZ_LIBPATH      "/cygdrive/c/Users/nripg/OneDrive/STORAGE/SOFTWARE/PICOS/SIDE/LIB"
#define  ZZ_INCPATH      "-I /cygdrive/c/Users/nripg/OneDrive/STORAGE/SOFTWARE/PICOS/SIDE/Examples/IncLib -I /cygdrive/c/Users/nripg/OneDrive/STORAGE/SOFTWARE/PICOS/VUEE/PICOS"
#define  ZZ_XINCPAT      {"/cygdrive/c/Users/nripg/OneDrive/STORAGE/SOFTWARE/PICOS/PICOS/Apps/DataLib", NULL}
#define  ZZ_MONHOST      "localhost"
#define  ZZ_MONSOCK      4442
#define  ZZ_RTAG	"PG250505A"
