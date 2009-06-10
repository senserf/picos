#ifndef	__picos_h__
#define	__picos_h__

#define	__UNIX__	1

/* ========================================================================== */

#define	MILLISECOND	0.0009765625	// 1/1024 s

typedef	unsigned short	word;
typedef	word		*address;
typedef	short int	sint;
typedef	unsigned int	lword;
typedef	unsigned char	byte;

#define	MAX_UINT	((word)0xFFFF)
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
				(int) (create a (b)) : 0)
#define	RELEASE		sleep
#define	nodefun(t,n,s)	t Node::n

#define	heapmem		const static byte zz_heapmem [] =

#define	join(p,s)	do { ((Process*)(p))->wait (DEATH, s); } while (0)

/* ========================================================================== */

#define	PHYSID			0
#define	MINIMUM_PACKET_LENGTH	8
#define	RADIO_DEF_BUF_LEN	48
#define	UART_DEF_BUF_LEN	82
#define	CC1100_MAXPLEN		60
#define	INFO_PHYS_DM2200	0x0700

#include "sysio.h"

#endif
