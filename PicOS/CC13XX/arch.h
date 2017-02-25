#ifndef __pg_arch_h
#define	__pg_arch_h		1

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2017                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Standard headers from the compiler and CC13XXWARE

#include <stdint.h>
#include <stdarg.h>
#include <inc/hw_cpu_scs.h>
#include <inc/hw_uart.h>
#include <inc/hw_aon_rtc.h>
#include <driverlib/prcm.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include <driverlib/ioc.h>
#include <driverlib/sys_ctrl.h>
#include <driverlib/aon_rtc.h>
#include <driverlib/aon_batmon.h>
#include <driverlib/aux_adc.h>

#define	mkmk_eval
#include "cmsis_gcc.h"
#undef	mkmk_eval

/* ==================================================================== */
/* Architecture-specific definitions                                    */
/* ==================================================================== */

typedef uint8_t		Boolean;
typedef	uint16_t	word;		// 16-bit
typedef	int32_t		sint;		// Machine's standard integer
typedef	int32_t		lint;		// Machine's standard long integer
typedef	uint8_t		byte;		// 8-bit
typedef	uint32_t	lword;		// 32-bit
typedef	uint32_t	aword;		// Integer accommodating address
typedef	word		*address;

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

extern	char	*__bss_start__, *__bss_end__;

#define	MALLOC_START		((address)&__bss_end__)

// This is in awords
#if	STACK_GUARD
#define MALLOC_LENGTH	(	(((aword) STACK_END - (aword)&__bss_end__)/4) \
					- 1)
#else
#define MALLOC_LENGTH		(((aword) STACK_END - (aword)&__bss_end__)/4)
#endif

#define	STATIC_LENGTH		(((aword)&__bss_end__ - (aword)RAM_START + 1)/4)

// ============================================================================

#if 0
#define	SET_RELEASE_POINT	__asm__ __volatile__ (\
		".global __pi_release\n"\
		"__pi_release: mov r2, %0\n"\
		"movt r2, %1\n"\
		"mov sp, r2\n"\
			:: "i"((lword)(STACK_START) & 0x0000ffff),\
			   "i"((lword)(STACK_START) >> 16) : "r2")

#define	SET_RELEASE_POINT	do { \
		__saved_sp = __get_MSP (); \
		__asm__ __volatile__ (	".global __pi_release\n" \
					"__pi_release:\n" ::: \
					"r0", "r1", "r2", "r3", "r4", \
					"r5", "r6", "r7", "r8", "r9", \
					"lr", "ip", "sl", "fp"); \
		__set_MSP (__saved_sp); \
	} while (0)

#define	SET_RELEASE_POINT	do { \
		__asm__ __volatile__ (	".global __pi_release\n" \
					"__pi_release:\n" ::: \
					"r0", "r1", "r2", "r3", "r4", \
					"r5", "r6", "r7", "r8", "r9", \
					"lr", "ip", "sl", "fp"); \
		__ISB (); \
		__DSB (); \
		__DMB (); \
		__set_MSP ((lword)(STACK_START)); \
	} while (0)
#endif

#define	SET_RELEASE_POINT	CNOP
		
// ============================================================================
					
void            __pi_release () __attribute__ ((noreturn));

#define	hard_reset	SysCtrlSystemReset ()

#define	nodata		do { } while (0)

#define	INLINE		inline

#define	REQUEST_EXTERNAL(p) __asm__ (".global " #p)

#endif
