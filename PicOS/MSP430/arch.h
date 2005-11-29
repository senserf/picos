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

	byte	pdmode:1,	// Power down flag
		evntpn:1,	// Scheduler event pending
		unused:1,
		ledblk:1,	// Blink flag
		ledsts:4;	// Blink status of four leds

	byte	ledblc;		// Blink counter

} systat_t;
		
extern	systat_t zz_systat;

extern void	__bss_end;
#define	MALLOC_START		((address)&__bss_end)

#if	STACK_GUARD
#define MALLOC_LENGTH	(	(((word) STACK_END - (word)&__bss_end)/2) - 1)
#else
#define MALLOC_LENGTH		(((word) STACK_END - (word)&__bss_end)/2)
#endif

#define	STATIC_LENGTH		(((word)&__bss_end - (word)RAM_START + 1)/2)

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
