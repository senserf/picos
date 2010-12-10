#ifndef	__pg_iflash_sys_h
#define	__pg_iflash_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

/*
 * MOV #FWKEY+FSSEL1+FN0,&FCTL2 ; SMCLK/12 (assumes 4.7MHz SMCLK)
 * MOV #FWKEY,&FCTL3 ; Clear LOCK
 * MOV #FWKEY+WRT,&FCTL1 ; Enable write
 * MOV #0123h,&0FF1Eh ; 0123h ?> 0FF1Eh
 * MOV #FWKEY,&FCTL1 ; Done. Clear WRT
 * MOV #FWKEY+LOCK,&FCTL3 ; Set LOCK
 */

// FIXME: actively touching the flash stops the watchdog. Should we revert it
// to the previous state?
#define	if_write_sys(w,a)	do { \
					WATCHDOG_STOP; \
					__FLASH_OP_WRITE__ (w, a); \
				} while (0)

#define	if_erase_sys(a)		do { \
					WATCHDOG_STOP; \
					__FLASH_OP_ERASE__ (a); \
				} while (0)
#endif
