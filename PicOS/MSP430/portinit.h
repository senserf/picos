/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

//
// Port initializer
//

// ============================================================================
// Interrupts =================================================================
// ============================================================================

#ifndef	PIN_DEFAULT_P1IE
#define	PIN_DEFAULT_P1IE	0x00
#endif

#ifndef	PIN_DEFAULT_P2IE
#define	PIN_DEFAULT_P2IE	0x00
#endif

#if PIN_DEFAULT_P1IE
	// Only if nonzero
	P1IE = PIN_DEFAULT_P1IE;
#endif

#if PIN_DEFAULT_P2IE
	P2IE = PIN_DEFAULT_P2IE;
#endif

#ifndef	PIN_DEFAULT_P1IES
#define	PIN_DEFAULT_P1IES	0x00
#endif

#ifndef	PIN_DEFAULT_P2IES
#define	PIN_DEFAULT_P2IES	0x00
#endif
	// Always
	P1IES = PIN_DEFAULT_P1IES;
	P2IES = PIN_DEFAULT_P2IES;

	// Not needed, reset with PUC
	// P1IFG = P2IFG = 0x00;

// ============================================================================

#ifdef	__PORT0_PRESENT__

#ifndef	PIN_DEFAULT_P0SEL
#define	PIN_DEFAULT_P0SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P0DIR
#define	PIN_DEFAULT_P0DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P0OUT
#define	PIN_DEFAULT_P0OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P0DS		// Strength
#define	PIN_DEFAULT_P0DS	0x00
#endif
#ifndef	PIN_DEFAULT_P0REN
#define	PIN_DEFAULT_P0REN	0x00
#endif

#if PIN_DEFAULT_P0SEL >= 0		// Can be disabled per board

	P0OUT = PIN_DEFAULT_P0OUT;
#if PIN_DEFAULT_P0SEL
	P0SEL = PIN_DEFAULT_P0SEL;
#endif
#if PIN_DEFAULT_P0DIR
	P0DIR = PIN_DEFAULT_P0DIR;
#endif
#ifdef	__PORT0_RESISTOR_PRESENT__
#if PIN_DEFAULT_P0DS
	P0DS = PIN_DEFAULT_P0DS;
#endif
#if PIN_DEFAULT_P0REN
	P0REN = PIN_DEFAULT_P0REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT1_PRESENT__

#ifndef	PIN_DEFAULT_P1SEL
#define	PIN_DEFAULT_P1SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P1DIR
#define	PIN_DEFAULT_P1DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P1OUT
#define	PIN_DEFAULT_P1OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P1DS		// Strength
#define	PIN_DEFAULT_P1DS	0x00
#endif
#ifndef	PIN_DEFAULT_P1REN
#define	PIN_DEFAULT_P1REN	0x00
#endif

#if PIN_DEFAULT_P1SEL >= 0		// Can be disabled per board

	P1OUT = PIN_DEFAULT_P1OUT;
#if PIN_DEFAULT_P1SEL
	P1SEL = PIN_DEFAULT_P1SEL;
#endif
#if PIN_DEFAULT_P1DIR
	P1DIR = PIN_DEFAULT_P1DIR;
#endif
#ifdef	__PORT1_RESISTOR_PRESENT__
#if PIN_DEFAULT_P1DS
	P1DS = PIN_DEFAULT_P1DS;
#endif
#if PIN_DEFAULT_P1REN
	P1REN = PIN_DEFAULT_P1REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT2_PRESENT__

#ifndef	PIN_DEFAULT_P2SEL
#define	PIN_DEFAULT_P2SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P2DIR
#define	PIN_DEFAULT_P2DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P2OUT
#define	PIN_DEFAULT_P2OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P2DS		// Strength
#define	PIN_DEFAULT_P2DS	0x00
#endif
#ifndef	PIN_DEFAULT_P2REN
#define	PIN_DEFAULT_P2REN	0x00
#endif

#if PIN_DEFAULT_P2SEL >= 0		// Can be disabled per board

	P2OUT = PIN_DEFAULT_P2OUT;
#if PIN_DEFAULT_P2SEL
	P2SEL = PIN_DEFAULT_P2SEL;
#endif
#if PIN_DEFAULT_P2DIR
	P2DIR = PIN_DEFAULT_P2DIR;
#endif
#ifdef	__PORT2_RESISTOR_PRESENT__
#if PIN_DEFAULT_P2DS
	P2DS = PIN_DEFAULT_P2DS;
#endif
#if PIN_DEFAULT_P2REN
	P2REN = PIN_DEFAULT_P2REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT3_PRESENT__

#ifndef	PIN_DEFAULT_P3SEL
#define	PIN_DEFAULT_P3SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P3DIR
#define	PIN_DEFAULT_P3DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P3OUT
#define	PIN_DEFAULT_P3OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P3DS		// Strength
#define	PIN_DEFAULT_P3DS	0x00
#endif
#ifndef	PIN_DEFAULT_P3REN
#define	PIN_DEFAULT_P3REN	0x00
#endif

#if PIN_DEFAULT_P3SEL >= 0		// Can be disabled per board

	P3OUT = PIN_DEFAULT_P3OUT;
#if PIN_DEFAULT_P3SEL
	P3SEL = PIN_DEFAULT_P3SEL;
#endif
#if PIN_DEFAULT_P3DIR
	P3DIR = PIN_DEFAULT_P3DIR;
#endif
#ifdef	__PORT3_RESISTOR_PRESENT__
#if PIN_DEFAULT_P3DS
	P3DS = PIN_DEFAULT_P3DS;
#endif
#if PIN_DEFAULT_P3REN
	P3REN = PIN_DEFAULT_P3REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT4_PRESENT__

#ifndef	PIN_DEFAULT_P4SEL
#define	PIN_DEFAULT_P4SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P4DIR
#define	PIN_DEFAULT_P4DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P4OUT
#define	PIN_DEFAULT_P4OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P4DS		// Strength
#define	PIN_DEFAULT_P4DS	0x00
#endif
#ifndef	PIN_DEFAULT_P4REN
#define	PIN_DEFAULT_P4REN	0x00
#endif

#if PIN_DEFAULT_P4SEL >= 0		// Can be disabled per board

	P4OUT = PIN_DEFAULT_P4OUT;
#if PIN_DEFAULT_P4SEL
	P4SEL = PIN_DEFAULT_P4SEL;
#endif
#if PIN_DEFAULT_P4DIR
	P4DIR = PIN_DEFAULT_P4DIR;
#endif
#ifdef	__PORT4_RESISTOR_PRESENT__
#if PIN_DEFAULT_P4DS
	P4DS = PIN_DEFAULT_P4DS;
#endif
#if PIN_DEFAULT_P4REN
	P4REN = PIN_DEFAULT_P4REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT5_PRESENT__

#ifndef	PIN_DEFAULT_P5SEL
#define	PIN_DEFAULT_P5SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P5DIR
#define	PIN_DEFAULT_P5DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P5OUT
#define	PIN_DEFAULT_P5OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P5DS		// Strength
#define	PIN_DEFAULT_P5DS	0x00
#endif
#ifndef	PIN_DEFAULT_P5REN
#define	PIN_DEFAULT_P5REN	0x00
#endif

#if PIN_DEFAULT_P5SEL >= 0		// Can be disabled per board

	P5OUT = PIN_DEFAULT_P5OUT;
#if PIN_DEFAULT_P5SEL
	P5SEL = PIN_DEFAULT_P5SEL;
#endif
#if PIN_DEFAULT_P5DIR
	P5DIR = PIN_DEFAULT_P5DIR;
#endif
#ifdef	__PORT5_RESISTOR_PRESENT__
#if PIN_DEFAULT_P5DS
	P5DS = PIN_DEFAULT_P5DS;
#endif
#if PIN_DEFAULT_P5REN
	P5REN = PIN_DEFAULT_P5REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */


// ============================================================================

#ifdef	__PORT6_PRESENT__

#ifndef	PIN_DEFAULT_P6SEL
#define	PIN_DEFAULT_P6SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P6DIR
#define	PIN_DEFAULT_P6DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P6OUT
#define	PIN_DEFAULT_P6OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P6DS		// Strength
#define	PIN_DEFAULT_P6DS	0x00
#endif
#ifndef	PIN_DEFAULT_P6REN
#define	PIN_DEFAULT_P6REN	0x00
#endif

#if PIN_DEFAULT_P6SEL >= 0		// Can be disabled per board

	P6OUT = PIN_DEFAULT_P6OUT;
#if PIN_DEFAULT_P6SEL
	P6SEL = PIN_DEFAULT_P6SEL;
#endif
#if PIN_DEFAULT_P6DIR
	P6DIR = PIN_DEFAULT_P6DIR;
#endif
#ifdef	__PORT6_RESISTOR_PRESENT__
#if PIN_DEFAULT_P6DS
	P6DS = PIN_DEFAULT_P6DS;
#endif
#if PIN_DEFAULT_P6REN
	P6REN = PIN_DEFAULT_P6REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT7_PRESENT__

#ifndef	PIN_DEFAULT_P7SEL
#define	PIN_DEFAULT_P7SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P7DIR
#define	PIN_DEFAULT_P7DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P7OUT
#define	PIN_DEFAULT_P7OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P7DS		// Strength
#define	PIN_DEFAULT_P7DS	0x00
#endif
#ifndef	PIN_DEFAULT_P7REN
#define	PIN_DEFAULT_P7REN	0x00
#endif

#if PIN_DEFAULT_P7SEL >= 0		// Can be disabled per board

	P7OUT = PIN_DEFAULT_P7OUT;
#if PIN_DEFAULT_P7SEL
	P7SEL = PIN_DEFAULT_P7SEL;
#endif
#if PIN_DEFAULT_P7DIR
	P7DIR = PIN_DEFAULT_P7DIR;
#endif
#ifdef	__PORT7_RESISTOR_PRESENT__
#if PIN_DEFAULT_P7DS
	P7DS = PIN_DEFAULT_P7DS;
#endif
#if PIN_DEFAULT_P7REN
	P7REN = PIN_DEFAULT_P7REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT8_PRESENT__

#ifndef	PIN_DEFAULT_P8SEL
#define	PIN_DEFAULT_P8SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P8DIR
#define	PIN_DEFAULT_P8DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P8OUT
#define	PIN_DEFAULT_P8OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P8DS		// Strength
#define	PIN_DEFAULT_P8DS	0x00
#endif
#ifndef	PIN_DEFAULT_P8REN
#define	PIN_DEFAULT_P8REN	0x00
#endif

#if PIN_DEFAULT_P8SEL >= 0		// Can be disabled per board

	P8OUT = PIN_DEFAULT_P8OUT;
#if PIN_DEFAULT_P8SEL
	P8SEL = PIN_DEFAULT_P8SEL;
#endif
#if PIN_DEFAULT_P8DIR
	P8DIR = PIN_DEFAULT_P8DIR;
#endif
#ifdef	__PORT8_RESISTOR_PRESENT__
#if PIN_DEFAULT_P8DS
	P8DS = PIN_DEFAULT_P8DS;
#endif
#if PIN_DEFAULT_P8REN
	P8REN = PIN_DEFAULT_P8REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT9_PRESENT__

#ifndef	PIN_DEFAULT_P9SEL
#define	PIN_DEFAULT_P9SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P9DIR
#define	PIN_DEFAULT_P9DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P9OUT
#define	PIN_DEFAULT_P9OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P9DS		// Strength
#define	PIN_DEFAULT_P9DS	0x00
#endif
#ifndef	PIN_DEFAULT_P9REN
#define	PIN_DEFAULT_P9REN	0x00
#endif

#if PIN_DEFAULT_P9SEL >= 0		// Can be disabled per board

	P9OUT = PIN_DEFAULT_P9OUT;
#if PIN_DEFAULT_P9SEL
	P9SEL = PIN_DEFAULT_P9SEL;
#endif
#if PIN_DEFAULT_P9DIR
	P9DIR = PIN_DEFAULT_P9DIR;
#endif
#ifdef	__PORT9_RESISTOR_PRESENT__
#if PIN_DEFAULT_P9DS
	P9DS = PIN_DEFAULT_P9DS;
#endif
#if PIN_DEFAULT_P9REN
	P9REN = PIN_DEFAULT_P9REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT10_PRESENT__

#ifndef	PIN_DEFAULT_P10SEL
#define	PIN_DEFAULT_P10SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P10DIR
#define	PIN_DEFAULT_P10DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P10OUT
#define	PIN_DEFAULT_P10OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P10DS		// Strength
#define	PIN_DEFAULT_P10DS	0x00
#endif
#ifndef	PIN_DEFAULT_P10REN
#define	PIN_DEFAULT_P10REN	0x00
#endif

#if PIN_DEFAULT_P10SEL >= 0		// Can be disabled per board

	P10OUT = PIN_DEFAULT_P10OUT;
#if PIN_DEFAULT_P10SEL
	P10SEL = PIN_DEFAULT_P10SEL;
#endif
#if PIN_DEFAULT_P10DIR
	P10DIR = PIN_DEFAULT_P10DIR;
#endif
#ifdef	__PORT10_RESISTOR_PRESENT__
#if PIN_DEFAULT_P10DS
	P10DS = PIN_DEFAULT_P10DS;
#endif
#if PIN_DEFAULT_P10REN
	P10REN = PIN_DEFAULT_P10REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORT11_PRESENT__

#ifndef	PIN_DEFAULT_P11SEL
#define	PIN_DEFAULT_P11SEL	0x00
#endif
#ifndef	PIN_DEFAULT_P11DIR
#define	PIN_DEFAULT_P11DIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_P11OUT
#define	PIN_DEFAULT_P11OUT	0x00
#endif
#ifndef	PIN_DEFAULT_P11DS		// Strength
#define	PIN_DEFAULT_P11DS	0x00
#endif
#ifndef	PIN_DEFAULT_P11REN
#define	PIN_DEFAULT_P11REN	0x00
#endif

#if PIN_DEFAULT_P11SEL >= 0		// Can be disabled per board

	P11OUT = PIN_DEFAULT_P11OUT;
#if PIN_DEFAULT_P11SEL
	P11SEL = PIN_DEFAULT_P11SEL;
#endif
#if PIN_DEFAULT_P11DIR
	P11DIR = PIN_DEFAULT_P11DIR;
#endif
#ifdef	__PORT11_RESISTOR_PRESENT__
#if PIN_DEFAULT_P11DS
	P11DS = PIN_DEFAULT_P11DS;
#endif
#if PIN_DEFAULT_P11REN
	P11REN = PIN_DEFAULT_P11REN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */

// ============================================================================

#ifdef	__PORTJ_PRESENT__

#ifndef	PIN_DEFAULT_PJSEL
#define	PIN_DEFAULT_PJSEL	0x00
#endif
#ifndef	PIN_DEFAULT_PJDIR
#define	PIN_DEFAULT_PJDIR	0xFF	// For power savings, the default is OUT
#endif
#ifndef	PIN_DEFAULT_PJOUT
#define	PIN_DEFAULT_PJOUT	0x00
#endif
#ifndef	PIN_DEFAULT_PJDS		// Strength
#define	PIN_DEFAULT_PJDS	0x00
#endif
#ifndef	PIN_DEFAULT_PJREN
#define	PIN_DEFAULT_PJREN	0x00
#endif

#if PIN_DEFAULT_PJSEL >= 0		// Can be disabled per board

	PJOUT = PIN_DEFAULT_PJOUT;
#if PIN_DEFAULT_PJSEL
	// PJSEL = PIN_DEFAULT_PJSEL;	// No SEL for port J
#endif
#if PIN_DEFAULT_PJDIR
	PJDIR = PIN_DEFAULT_PJDIR;
#endif
#ifdef	__PORTJ_RESISTOR_PRESENT__
#if PIN_DEFAULT_PJDS
	PJDS = PIN_DEFAULT_PJDS;
#endif
#if PIN_DEFAULT_PJREN
	PJREN = PIN_DEFAULT_PJREN;
#endif
#endif	/* DS / REN */
#endif	/* DISABLED */
#endif	/* HAS_PORT */
