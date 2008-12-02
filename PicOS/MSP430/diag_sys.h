#ifndef __pg_diag_sys_h
#define	__pg_diag_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "mach.h"

#ifdef	DIAG_IMPLEMENTATION

#if	DIAG_IMPLEMENTATION == 0
// Direct UART
#define	diag_wchar(c,a)		TXBUF_A = (byte)(c)
#define	diag_wait(a)		while ((IFG_A & UTXIFG_A) == 0)

#define	diag_disable_int(a,u)	do { \
					(u) = IE_A & (URXIE_A + UTXIE_A); \
					(u) |= (READ_SR & GIE); \
					cli; \
					_BIC (IE_A, URXIE_A + UTXIE_A); \
					if ((u) & GIE) \
						sti; \
					(u) &= ~GIE; \
				} while (0)
					
#define	diag_enable_int(a,u)	do { \
					(u) |= (READ_SR & GIE); \
					cli; \
					_BIS (IE_A, (u) & (URXIE_A + UTXIE_A));\
					if ((u) & GIE) \
						sti; \
				} while (0)
#endif	/* DIAG_IMPLEMENTATION == 0 */

#if	DIAG_IMPLEMENTATION == 1
// UART requiring driver-specific functions (these two stay the same)
#define	diag_wchar(c,a)		TXBUF_A = (byte)(c)
#define	diag_wait(a)		while ((IFG_A & UTXIFG_A) == 0)

#define	ZZ_DIAG_UNUMBER_a	0
#define	ZZ_DIAG_UNUMBER_b	1

// These two must be provided by the PHY
void zz_diag_init (int), zz_diag_stop (int);

#define	diag_disable_int(a,u)	zz_diag_init (ZZ_DIAG_UNUMBER_ ## a )
#define	diag_enable_int(a,u)	zz_diag_stop (ZZ_DIAG_UNUMBER_ ## a )

#endif	/* DIAG_IMPLEMENTATION == 1 */

#if	DIAG_IMPLEMENTATION == 2
// LCD
#define	diag_disable_int(a,u)	lcd_clear (0, 0)
#define	diag_wchar(c,a)		lcd_putchar (c)
#define	diag_wait(a)		CNOP
#define	diag_enable_int(a,u)	CNOP

#endif	/* DIAG_IMPLEMENTATION == 2 */

#ifndef	diag_wait
// Void
#define	diag_disable_int(a,u)	CNOP
#define	diag_wchar(c,a)		CNOP
#define	diag_wait(a)		CNOP
#define	diag_enable_int(a,u)	CNOP
#endif	/* void */

#endif	/* DIAG_IMPLEMENTATION */
// ============================================================================

#endif
