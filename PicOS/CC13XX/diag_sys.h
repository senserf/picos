/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_diag_sys_h
#define	__pg_diag_sys_h		1

#include "mach.h"

#if	DIAG_MESSAGES

// ============================================================================
// Direct UART ================================================================
// ============================================================================

#if	DIAG_IMPLEMENTATION == 0

// Direct UART

#define	diag_wchar(c,a)		uart_a_write (c)
#define	diag_wait(a)		while (UARTBusy (UART0_BASE))

#define	diag_disable_int(a,u)	do { (void)(u); IntDisable (INT_UART0_COMB); }\
					while (0)

#define	diag_enable_int(a,u)	do { (void)(u); IntEnable (INT_UART0_COMB); }\
					while (0)
#endif

// ============================================================================
// UART requiring driver-specific functions ===================================
// ============================================================================

#if	DIAG_IMPLEMENTATION == 1

#define	diag_wchar(c,a)		uart_a_write (c)
#define	diag_wait(a)		while (UARTBusy (UART0_BASE))

// Assumes there is only one UART

#define	__pi_DIAG_UNUMBER_a	0
#define	__pi_DIAG_UNUMBER_b	1

// These two must be provided by the PHY
void __pi_diag_init (int), __pi_diag_stop (int);

#define	diag_disable_int(a,u)	__pi_diag_init (__pi_DIAG_UNUMBER_ ## a )
#define	diag_enable_int(a,u)	__pi_diag_stop (__pi_DIAG_UNUMBER_ ## a )

#endif	/* DIAG_IMPLEMENTATION == 1 */

#endif	/* DIAG_MESSAGES */

#endif
