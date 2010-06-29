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
typedef unsigned char	Boolean;
typedef	unsigned int	word;
typedef	int		sint;
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
		fstblk:1,	// Fast blink flag
		ledblk:1,	// Blink flag
		ledsts:4;	// Blink status of four leds

	byte	ledblc;		// Blink counter

} systat_t;
		
extern volatile systat_t zz_systat;

extern void	__bss_end;
#define	MALLOC_START		((address)&__bss_end)

#if	STACK_GUARD
#define MALLOC_LENGTH	(	(((word) STACK_END - (word)&__bss_end)/2) - 1)
#else
#define MALLOC_LENGTH		(((word) STACK_END - (word)&__bss_end)/2)
#endif

#define	STATIC_LENGTH		(((word)&__bss_end - (word)RAM_START + 1)/2)

#define	SET_RELEASE_POINT	__asm__ __volatile__ (\
		".global zz_restart_entry\n"\
		"zz_restart_entry: mov %0, r1"\
			:: "i"(STACK_START): "r1")

void		zz_restart_entry () __attribute__ ((noreturn));
#define	release	zz_restart_entry ()

#define	hard_reset	__asm__ __volatile__("br #_reset_vector__"::)

#define	nodata		do { } while (0)

#define	INLINE		inline

#define	REQUEST_EXTERNAL(p) __asm__ (".global " #p)

#endif
