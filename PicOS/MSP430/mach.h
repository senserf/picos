#ifndef __pg_mach_h
#define	__pg_mach_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "portnames.h"

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Hardware-specific definitions                                              */
/*                                                                            */

#ifndef	__MSP430__
#error "this must be compiled with mspgcc!!!"
#endif

#ifdef        	__MSP430_148__
#define       	__MSP430_1xx__
#endif
#ifdef        	__MSP430_149__
#define       	__MSP430_1xx__
#endif
#ifdef		__MSP430_1611__
#define		__MSP430_1xx__
#endif

#ifdef		__MSP430_148__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#endif

#ifdef		__MSP430_149__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#endif

#ifdef		__MSP430_449__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#endif

#ifdef		__MSP430_1611__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2800	// 10240
#endif

#ifndef		RAM_SIZE
#error	"untried yet CPU type: check MSP430/mach.h"
#endif

#include "arch.h"

// ACLK crystal parameters

#ifndef	CRYSTAL_RATE
#define	CRYSTAL_RATE		32768
#endif

#ifndef	CRYSTAL2_RATE
#define	CRYSTAL2_RATE		0
#endif

#if	CRYSTAL2_RATE != 0
#if	CRYSTAL2_RATE < 1000000
#error "CRYSTAL2_RATE must be >= 1000000"
#endif
#endif

#if	CRYSTAL_RATE != 32768

#if	CRYSTAL_RATE < 1000000
#error "CRYSTAL_RATE can be 32768 or >= 1000000"
#endif

// The number of slow ticks per second
#define	TIMER_B_LOW_PER_SEC	16
#define	HIGH_CRYSTAL_RATE 	1

#else	/* Low crystal rate */

#if	WATCHDOG_ENABLED
// If we want to take advantage of the watchdog in power-down mode, the clock
// must run at least 2 times per second to clear the watchdog, which, at its
// slowest, goes off after 1 second. Note that with HIGH_CRYSTAL_RATE,
// watchdog is impossible in power down mode (hopefully, it won't be entered
// then) because at, say, 8MHz ACLK rate, the watchdog will go off after
// ca. 1/250 s. Well, we could slow down ACLK, but the whole point of having
// a high speed crystal for ACLK is to run it at a high rate.
#define	TIMER_B_LOW_PER_SEC	2
#else
#define	TIMER_B_LOW_PER_SEC	1
#endif

#define	HIGH_CRYSTAL_RATE	0

#endif	/* CRYSTAL_RATE != 32768 */

#define	TIMER_B_INIT_HIGH	((CRYSTAL_RATE/8192) - 1)
#define	TIMER_B_INIT_LOW  	((CRYSTAL_RATE/(8*TIMER_B_LOW_PER_SEC)) - 1)

#define	LITTLE_ENDIAN	1
#define	BIG_ENDIAN	0

#if	INFO_FLASH
#define	IFLASH_SIZE	128			// This is in words
#define	IFLASH_HARD_ADDRESS	((word*)0x1000)
#endif

#define	RAM_END		(RAM_START + RAM_SIZE)
#define	STACK_SIZE	256			// Bytes
#define	STACK_START	((byte*)RAM_END)	// FWA + 1 of stack
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
#define	UART_FLAGS_LOCK		0x20
#define	UART_RATE_MASK		0x07

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

#if WATCHDOG_ENABLED

#define	WATCHDOG_HOLD		WATCHDOG_STOP
#define	WATCHDOG_START		WDTCTL = WDTPW + WDTCNTCL + WDTSSEL
#define	WATCHDOG_CLEAR		WDTCTL = WDTPW + WDTCNTCL

#else

#define	WATCHDOG_HOLD		CNOP
#define	WATCHDOG_START		CNOP
#define	WATCHDOG_CLEAR		CNOP

#endif

#define	WATCHDOG_RESUME		WATCHDOG_START

/* =============================== */
/* Enable/disable clock interrupts */
/* =============================== */
#define sti_tim	_BIS (TBCCTL0, CCIE)
#define cli_tim	_BIC (TBCCTL0, CCIE)
#define	dis_tim _BIC (TBCTL, MC0 + MC1)
#define	ena_tim _BIS (TBCTL, MC0      )


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

#ifdef	MONITOR_PIN_CPU

#define	CPU_MARK_IDLE	_PVS (MONITOR_PIN_CPU, 0)
#define	CPU_MARK_BUSY	_PVS (MONITOR_PIN_CPU, 1)

#else

#define	CPU_MARK_IDLE	do { } while (0)
#define	CPU_MARK_BUSY	do { } while (0)

#endif

#define	SLEEP	do { \
			CPU_MARK_IDLE; \
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
			CPU_MARK_BUSY; \
		} while (0)

#define	RISE_N_SHINE	_BIC_SR_IRQ (LPM4_bits)

#if LEDS_DRIVER
#include "leds_sys.h"
#endif

#endif
