/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#ifndef	__picos_h__
#define	__picos_h__

#define	__UNIX__	1

/* ========================================================================== */

#define	MSCINSECOND	1024.0			// Milliseconds in a second
#define	MILLISECOND	(1.0/MSCINSECOND)	// Seconds in a millisecond

#ifndef	__TYPE_WORD__
#define	__TYPE_WORD__	uint16_t
#endif

#ifndef	__TYPE_LWORD__
#define	__TYPE_LWORD__	uint32_t
#endif

#ifndef	__TYPE_SINT__
// Good for MSP430, but not for ARM
#define	__TYPE_SINT__	int16_t
#endif

#ifndef	__TYPE_WINT__
#define	__TYPE_WINT__	int16_t
#endif

#ifndef	__TYPE_LINT__
#define	__TYPE_LINT__	int32_t
#endif

#ifndef	__TYPE_AWORD__
// Good for MSP430, but not for ARM
#define	__TYPE_AWORD__	uint16_t
#endif

#ifndef	IFLASH_SIZE
#define	IFLASH_SIZE	128	// Words
#endif

typedef	__TYPE_WORD__	word;
typedef	__TYPE_SINT__	sint;
typedef	__TYPE_WINT__	wint;
typedef __TYPE_LINT__	lint;
typedef	__TYPE_LWORD__	lword;
typedef	__TYPE_AWORD__	aword;

typedef	unsigned char	byte;
typedef	word		*address;

#define	sysassert(a,b)	do { if (!(a)) syserror (EASSERT, b); } while (0)
#define	CNOP		do { } while (0)

#define	byteaddr(p)	((char*)(p))
#define	entry(s)	transient s :
#define	process(a,b)	a::perform {
#define	endprocess(a)	}
#define	nodata		CNOP
// #define	FORK(a,b)	create a
#define	fork(a,b)	(((PicOSNode*)TheStation)->tally_in_pcs () ? \
				(create a (b)) -> _pp_apid_ () : 0)
#define	RELEASE		sleep
#define	nodefun(t,n,s)	t Node::n

#define	heapmem		const static byte __pi_heapmem [] =

#define	strlen(s)	__pi_strlen (s)
#define	strcpy(a,b)	__pi_strcpy (a, b)
#define	strncpy(a,b,c)	__pi_strncpy (a, b, c)
#define	strcat(a,b) 	__pi_strcat (a, b)
#define	strncat(a,b,c)	__pi_strncat (a, b, c)

sint		__pi_strlen (const char*);
void		__pi_strcpy (char*, const char*);
void		__pi_strncpy (char*, const char*, int);
void		__pi_strcat (char*, const char*);
void		__pi_strncat (char*, const char*, int);

/* ========================================================================== */

#include "sysio.h"

#endif
