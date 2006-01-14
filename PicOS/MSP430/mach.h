#ifndef __pg_mach_h
#define	__pg_mach_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */


/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Hardware-specific definitions                                              */
/*                                                                            */

#ifndef	__MSP430__
#error "this must be compiled with mspgcc!!!"
#endif

#include "arch.h"

// ACLK crystal parameters

#ifndef	CRYSTAL_RATE
#define	CRYSTAL_RATE		32768
#endif

#if	CRYSTAL_RATE != 32768

#if	CRYSTAL_RATE < 1000000
#error "CRYSTAL_RATE can be 32768 or >= 1000000"
#endif

// The number of slow ticks per second
#define	TIMER_B_LOW_PER_SEC	16
#define	HIGH_CRYSTAL_RATE 	1

#else	/* Low crystal rate */

#define	TIMER_B_LOW_PER_SEC	1
#define	HIGH_CRYSTAL_RATE	0

#endif	/* CRYSTAL_RATE != 32768 */

#define	TIMER_B_INIT_HIGH	((CRYSTAL_RATE/8192) - 1)
#define	TIMER_B_INIT_LOW  	((CRYSTAL_RATE/(8*TIMER_B_LOW_PER_SEC)) - 1)

#ifndef UART_INPUT_BUFFER_LENGTH
#define	UART_INPUT_BUFFER_LENGTH	0
#endif

#define	LITTLE_ENDIAN	1
#define	BIG_ENDIAN	0

#if	INFO_FLASH
#define	IFLASH_SIZE	128			// This is in words
#define	IFLASH_HARD_ADDRESS	((word*)0x1000)
#endif

#define	RAM_START	((byte*)0x200)
#define	RAM_END		(RAM_START + 2048)
#define	STACK_SIZE	256			// Bytes
#define	STACK_START	RAM_END			// FWA + 1 of stack
#define	STACK_END	(STACK_START - STACK_SIZE)

						// LWA of stack
#if	UART_DRIVER

typedef struct	{
/* ============================== */
/* UART with two circular buffers */
/* ============================== */
	byte selector;
	volatile byte flags;
	byte out;
#if	UART_INPUT_BUFFER_LENGTH < 2
	byte in;
#else
	byte in [UART_INPUT_BUFFER_LENGTH];
	byte ib_in, ib_out, ib_count;
#endif
} uart_t;

extern uart_t zz_uart [];

#define	UART_BASE		UART_A

#define	UART_FLAGS_IN		0x80
#define	UART_FLAGS_OUT		0x40
#define	UART_FLAGS_LOCK		0x01

#endif	/* UART_DRIVER */


#if	DIAG_MESSAGES || (dbg_level != 0)

#define	diag_wchar(c,a)		TXBUF0 = (byte)(c)
#define	diag_wait(a)		while ((IFG1 & UTXIFG0) == 0)





#define	diag_disable_int(a,u)	do { \
					(u) = IE1 & (URXIE0 + UTXIE0); \
					(u) |= (READ_SR & GIE); \
					cli; \
					_BIC (IE1, URXIE0 + UTXIE0); \
					if ((u) & GIE) \
						sti; \
					(u) &= ~GIE; \
				} while (0)
					
#define	diag_enable_int(a,u)	do { \
					(u) |= (READ_SR & GIE); \
					cli; \
					_BIS (IE1, (u) & (URXIE0 + UTXIE0)); \
					if ((u) & GIE) \
						sti; \
				} while (0)
#endif

#define	sti	_EINT ()
#define	cli	_DINT ()

/* =================== */
/* Watchdog operations */
/* =================== */

#define	WATCHDOG_STOP		WDTCTL = WDTPW + WDTHOLD
#define	WATCHDOG_START		WDTCL = WDTPW + WDTCNTCL + WDTSSEL

/* =============================== */
/* Enable/disable clock interrupts */
/* =============================== */
#define sti_tim	_BIS (TBCCTL0, CCIE)
#define cli_tim	_BIC (TBCCTL0, CCIE)

#define	uart_a_disable_int		_BIC (IE1, URXIE0 + UTXIE0)
#define	uart_b_disable_int		_BIC (IE2, URXIE1 + UTXIE1)
#define	uart_a_disable_read_int		_BIC (IE1, URXIE0)
#define	uart_a_disable_write_int	_BIC (IE1, UTXIE0)
#define	uart_b_disable_read_int		_BIC (IE2, URXIE1)
#define	uart_b_disable_write_int	_BIC (IE2, UTXIE1)
#define	uart_a_enable_read_int		_BIS (IE1, URXIE0)
#define	uart_a_enable_write_int		_BIS (IE1, UTXIE0)
#define	uart_b_enable_read_int		_BIS (IE2, URXIE1)
#define	uart_b_enable_write_int		_BIS (IE2, UTXIE1)

#define	SLEEP	do { \
			if (zz_systat.pdmode) { \
				_BIC_SR (GIE); \
				if (zz_systat.evntpn) { \
					zz_systat.evntpn = 0; \
					_BIS_SR (GIE); \
				} else { \
					_BIS_SR (LPM3_bits + GIE); \
					_NOP (); \
				} \
			} else { \
				_BIC_SR (GIE); \
				if (zz_systat.evntpn) { \
					zz_systat.evntpn = 0; \
					_BIS_SR (GIE); \
				} else { \
					_BIS_SR (LPM0_bits + GIE); \
					_NOP (); \
				} \
			} \
		} while (0)

#define	RISE_N_SHINE	_BIC_SR_IRQ (LPM4_bits)

#if LEDS_DRIVER
#include "leds_sys.h"
#endif

#endif
