#ifndef	__picos_h__
#define	__picos_h__

#define	__UNIX__	1

/* ========================================================================== */

#define	MSCINSECOND	1024.0			// Milliseconds in a second
#define	MILLISECOND	(1.0/MSCINSECOND)	// Seconds in a millisecond

typedef	unsigned short	word;
typedef	word		*address;
typedef	short int	sint;
typedef Long		lint;
typedef	U_Long		lword;
typedef	unsigned char	byte;

#define	MAX_INT		((int)0x7FFF)		// This is the PicOS int !!
#define	MAX_UINT	((word)0xFFFF)
#define	MAX_WORD	MAX_UINT
#define	MAX_ULONG	((lword)0xFFFFFFFF)

#define	IFLASH_SIZE	128	// Words

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

#define	heapmem		const static byte zz_heapmem [] =

/* ========================================================================== */

#include "sysio.h"

#endif
