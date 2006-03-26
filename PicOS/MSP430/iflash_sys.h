#ifndef	__pg_iflash_sys_h
#define	__pg_iflash_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#define	IF_PAGE_SIZE	64	// in words

/*
 * MOV #FWKEY+FSSEL1+FN0,&FCTL2 ; SMCLK/12 (assumes 4.7MHz SMCLK)
 * MOV #FWKEY,&FCTL3 ; Clear LOCK
 * MOV #FWKEY+WRT,&FCTL1 ; Enable write
 * MOV #0123h,&0FF1Eh ; 0123h ?> 0FF1Eh
 * MOV #FWKEY,&FCTL1 ; Done. Clear WRT
 * MOV #FWKEY+LOCK,&FCTL3 ; Set LOCK
 */
#define	if_write_sys(w,a) __asm__ __volatile__ ( \
			"mov %0,%1\n\t" \
			"mov %2,%3\n\t" \
			"mov %4,%5\n\t" \
			"mov %6,%7\n\t" \
			"mov %2,%5\n\t" \
			"mov %8,%3\n\t" : : \
			"i"((int)(FWKEY+FSSEL1+11)), "m"(FCTL2), \
			"i"((int)(FWKEY)), "m"(FCTL3), \
			"i"((int)(FWKEY+WRT)), "m"(FCTL1), \
			"m"((int)(w)),"m"((IFLASH[a])), \
			"i"((int)(FWKEY+LOCK)) )

#define	if_erase_sys(a)	__asm__ __volatile__ ( \
			"mov %0,%1\n\t" \
			"mov %2,%3\n\t" \
			"mov %4,%5\n\t" \
			"clr 0(%6)\n\t" \
			"mov %7,%3\n\t" : : \
			"i"((int)(FWKEY+FSSEL1+11)), "m"(FCTL2), \
			"i"((int)(FWKEY)), "m"(FCTL3), \
			"i"((int)(FWKEY+ERASE)), "m"(FCTL1), \
			"r"((int)(a)), \
			"i"((int)(FWKEY+LOCK)) )
#endif
