/* SIDE version number */
#define  VERSION      "2.80L"

#ifdef	__CYGWIN32__
#define	ZZ_CYW	1

#define		MAXSHORT		((short int)0x7fff)
#define		MINSHORT		((short int)0x8000)
#define		MAXINT			((int)0x7fffffff)
#define		MININT			((int)0x80000000)
#define		MAXLONG			((long)MAXINT)
#define		MINLONG			((long)MININT)
#define		LONGBITS		32

#else
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

#if	__WORDSIZE <= 32

#define		LONG			long long
#define 	ZZ_LONG_is_not_long 	1
#define 	IPointer            	int
#define 	LPointer            	LONG
#define         Long                    long
#define		MAX_LONG	((LONG)(((LONG)1<<(LONGBITS+LONGBITS-1))-1))
#define		MIN_LONG	((LONG)( (LONG)1<<(LONGBITS+LONGBITS-1))   )
#define         MAX_Long                MAX_long
#define		MIN_Long		MIN_long
#define		ZZ_XLong		"long"     /* For SMPP */

#else

#define		LONG			long
#define		MAX_LONG		MAX_long
#define		MIN_LONG		MIN_long
/* Honorary long, must be 32-bits long, but doesn't have to be longer */
#define		ZZ_Long_eq_int		1
#define         Long                    int
#define         MAX_Long                MAX_int
#define		MIN_Long		MIN_int
#define		ZZ_XLong		"int"      /* For SMPP */

/* Pointer type to be used in arithmetic */
#if PTRBITS == INTBITS
#define	IPointer int
#else
#if PTRBITS == LONGBITS
#define	IPointer LONG
#else
ERROR: unsupported pointer size
#endif
#endif

/* A type capable of storing both pointers and LONG numbers */
#if LONGBITS > PTRBITS
#define	LPointer LONG
#else
#define	LPointer IPointer
#endif

#endif

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
#define  ZZ_SOURCES      "/home/pawel/SOFTWARE/SIDE/SOURCES"
#define  ZZ_LIBPATH      "/home/pawel/SOFTWARE/SIDE/LIB"
#define  ZZ_INCPATH      "/home/pawel/SOFTWARE/SIDE/Examples/IncLib"
#define  ZZ_MONHOST      "localhost"
#define  ZZ_MONSOCK      4442
