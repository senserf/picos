#ifndef __pg_diag_sys_h
#define	__pg_diag_sys_h

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifdef	DIAG_IMPLEMENTATION

#define DUART_a_STS_TX_RDY_MASK DUART_A_STS_TX_RDY_MASK
#define DUART_b_STS_TX_RDY_MASK DUART_B_STS_TX_RDY_MASK

#if	DIAG_IMPLEMENTATION == 0
// Direct UART
#define	diag_wchar(c,a)		rg.duart. ## a ## _tx8 = (word)(c)
#define	diag_wait(a)		while ((rg.duart. ## a ## _sts & \
			        DUART_ ## a ## _STS_TX_RDY_MASK) \
					== 0);

#define	diag_disable_int(a,u)	uart_ ## a ## _disable_int

#define	diag_enable_int(a,u)	do { \
					if (zz_uart [0].lock == 0) { \
					    uart_ ## a ## _enable_read_int; \
					    uart_ ## a ## _enable_write_int; \
					} \
				} while (0)
#endif	/* DIAG_IMPLEMENTATION == 0 */

#if	DIAG_IMPLEMENTATION == 1
// UART requiring driver-specific functions (these two stay the same)
#define	diag_wchar(c,a)		rg.duart. ## a ## _tx8 = (word)(c)
#define	diag_wait(a)		while ((rg.duart. ## a ## _sts & \
			        DUART_ ## a ## _STS_TX_RDY_MASK) \
					== 0);

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

#endif
