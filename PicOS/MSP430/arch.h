#ifndef __pg_arch_h
#define	__pg_arch_h		1

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

/* ==================================================================== */
/* Architecture-specific definitions                                    */
/* ==================================================================== */

typedef unsigned char	bool;
typedef unsigned char	Boolean;
typedef	unsigned int	word;		// 16-bit
typedef	int		sint;		// Machine's standard integer (VUEE)
typedef	long int	lint;		// Machine's standard long integer
typedef	unsigned char	byte;		// 8-bit
typedef	unsigned long	lword;		// 32-bit
typedef	unsigned int	aword;		// Integer accommodating address
typedef	word		*address;

// Size of pointer in bytes
#define	SIZE_OF_AWORD	2

#define	byteaddr(p)	((char*)(p))

typedef struct {

	byte	pdmode:1,	// Power down flag
		evntpn:1,	// Scheduler event pending
		fstblk:1,	// Fast blink flag
		ledblk:1,	// Blink flag
		ledsts:4;	// Blink status of four leds

	byte	ledblc;		// Blink counter

} systat_t;
		
extern volatile systat_t __pi_systat;

#define	MALLOC_START		((address)&__BSS_END)

#if	STACK_GUARD
#define MALLOC_LENGTH	(	(((aword) STACK_END - (aword)&__BSS_END)/2) - 1)
#else
#define MALLOC_LENGTH		(((aword) STACK_END - (aword)&__BSS_END)/2)
#endif

#define	STATIC_LENGTH		(((aword)&__BSS_END - (aword)RAM_START + 1)/2)

#define	mkmk_eval
#if __GNUC__ > 4

#define	SET_RELEASE_POINT	__asm__ __volatile__ (\
		".global __pi_release\n"\
		"__pi_release: mov %0, R1"\
			:: "i"(STACK_START): "R1")
#else

#define	SET_RELEASE_POINT	__asm__ __volatile__ (\
		".global __pi_release\n"\
		"__pi_release: mov %0, r1"\
			:: "i"(STACK_START): "r1")
#endif
#undef mkmk_eval

void		__pi_release () __attribute__ ((noreturn));

#define	hard_reset	__asm__ __volatile__("br &0xfffe"::)
//#define	hard_reset	__asm__ __volatile__("br #_reset_vector__"::)

#define	nodata		do { } while (0)

#define	INLINE		inline

#define	REQUEST_EXTERNAL(p) __asm__ (".global " #p)

#endif
