#ifndef __pg_diag_sys_h
#define	__pg_diag_sys_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "mach.h"

#ifdef	DIAG_IMPLEMENTATION

#if	DIAG_IMPLEMENTATION == 0
// Direct UART
#define	diag_wchar(c,a)		uart_a_write ((byte)(c))
#define	diag_wait(a)		while (uart_a_get_write_int == 0)

#define	diag_disable_int(a,u)	do { \
					(u) = uart_a_get_int_stat; \
					(u) |= (READ_SR & GIE); \
					cli; \
					uart_a_disable_int; \
					if ((u) & GIE) \
						sti; \
					(u) &= ~GIE; \
				} while (0)
					
#define	diag_enable_int(a,u)	do { \
					(u) |= (READ_SR & GIE); \
					cli; \
					uart_a_set_int_stat (u); \
					if ((u) & GIE) \
						sti; \
				} while (0)
#endif	/* DIAG_IMPLEMENTATION == 0 */

#if	DIAG_IMPLEMENTATION == 1
// UART requiring driver-specific functions (these two stay the same)
#define	diag_wchar(c,a)		uart_a_write ((byte)(c))
#define	diag_wait(a)		while (uart_a_get_write_int == 0)

#define	__pi_DIAG_UNUMBER_a	0
#define	__pi_DIAG_UNUMBER_b	1

// These two must be provided by the PHY
void __pi_diag_init (int), __pi_diag_stop (int);

#define	diag_disable_int(a,u)	__pi_diag_init (__pi_DIAG_UNUMBER_ ## a )
#define	diag_enable_int(a,u)	__pi_diag_stop (__pi_DIAG_UNUMBER_ ## a )

#endif	/* DIAG_IMPLEMENTATION == 1 */

#if	DIAG_IMPLEMENTATION == 2

#include "board_lcd.h"

#ifndef	lcd_diag_start
#error	"S: DIAG_IMPLEMENTATION == 2 required lcd_diag_start ... in board_lcd.h"
#endif

// LCD for diag
#define	diag_disable_int(a,u)	lcd_diag_start
#define	diag_wchar(c,a)		lcd_diag_wchar (c)
#define	diag_wait(a)		lcd_diag_wait
#define	diag_enable_int(a,u)	lcd_diag_stop

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
