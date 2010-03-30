#ifndef __pg_mach_h
#define	__pg_mach_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "portnames.h"

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Hardware-specific definitions                                              */
/*                                                                            */

#ifndef	__MSP430__
#error "S: this must be compiled with mspgcc!!!"
#endif

// Meaning we are gcc
#define	INTERNAL_FUNCTIONS_ALLOWED	1

#ifdef		__MSP430_148__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#endif

#ifdef		__MSP430_149__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#endif

#ifdef		__MSP430_1611__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2800	// 10240
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#endif

#ifdef		__MSP430_449__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#endif

#ifdef		__MSP430_G4617__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
// Different pins
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#endif

#ifdef		__MSP430_G4618__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#endif

#ifdef		__MSP430_G4619__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#endif

#ifdef		__CC430_6137__
#define		RAM_START	0x1C00
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_6xx__
#define       	__CC430_6xx__
#define		__CC430__
#define		__PORTMAPPER__
#define		__UART_CONFIG__	2
#define		__TCI_CONFIG__	2
#endif

#ifndef		RAM_SIZE
#error	"S: untried yet CPU type: check MSP430/mach.h"
#endif

// ============================================================================
// ============================================================================

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
#error "S: CRYSTAL2_RATE must be >= 1000000"
#endif
#endif

// ============================================================================

// This is the target rate of the delay clock, i.e., 1024 ticks per second
#define	TCI_HIGH_PER_SEC	1024

// Clock timer subdivision for high clock rate (clockup); the timer is going
// to be pre-divided by 8, so this subdivision yields the clock rate of 1024
// (TCT_HIGH_PER_SEC) ticks per second
#define	TCI_HIGH_DIV	(CRYSTAL_RATE/(8*TCI_HIGH_PER_SEC))

// ============================================================================

#if	CRYSTAL_RATE != 32768

#if	CRYSTAL_RATE < 1000000
#error "S: CRYSTAL_RATE can be 32768 or >= 1000000"
#endif

// The number of slow ticks per second
#define	TCI_LOW_PER_SEC		16

#define	HIGH_CRYSTAL_RATE 	1

#if SEPARATE_SECONDS_CLOCK
#error "S: SEPARATE_SECONDS_CLOCK is incompatible with HIGH_CRYSTAL_RATE"
#endif

// No clockdown/clockup modes with HIGH_CRYSTAL_RATE (power savings are not
// possible, anyway)
#define	clockup()	CNOP
#define	clockdown()	CNOP

#define	TCI_LOW_DIV	TCI_HIGH_DIV

#else	/* Low crystal rate */

#if	WATCHDOG_ENABLED
// If we want to take advantage of the watchdog in power-down mode, the clock
// must run at least 2 times per second to clear the watchdog, which, at its
// slowest, goes off after 1 second. Note that with HIGH_CRYSTAL_RATE,
// watchdog is impossible in power down mode (hopefully, it won't be entered
// then) because at, say, 8MHz ACLK rate, the watchdog will go off after
// ca. 1/250 s. Well, we could slow down ACLK, but the whole point of having
// a high speed crystal for ACLK is to run it at a high rate.
#define	TCI_LOW_PER_SEC		2
#else
#define	TCI_LOW_PER_SEC		1
#endif

#define	HIGH_CRYSTAL_RATE	0

// ============================================================================
// clockdown/clockup is enabled only if HIGH_CRYSTAL_RATE == 0
// ============================================================================

#if SEPARATE_SECONDS_CLOCK

// They are functions
void clockup (void), clockdown (void);

#else

// They are macros
#define	clockup()	(TCI_CCR = TCI_INIT_HIGH)
#define	clockdown()	(TCI_CCR = TCI_INIT_LOW)

#endif

// ============================================================================

#define	TCI_LOW_DIV		(CRYSTAL_RATE/(8*TCI_LOW_PER_SEC))

#endif	/* CRYSTAL_RATE != 32768 */

// ============================================================================

#if	__TCI_CONFIG__ == 1

// Timer for clock configuration 1 = TIMER_B

// Delay interrupt counter
#define	TCI_CCR		TBCCR0
// Seconds interrupt counter (if separate)
#define	TCI_CCS		TBCCR1
#define	TCI_VAL		TBR
#define	TCI_CTL		TBCTL
#define	TCI_VECTOR	TIMERB0_VECTOR
#define	TCI_VECTOR_S	TIMERB1_VECTOR

// To acknowledge seconds interrupts (any access to TBIV will do)
#define	ack_sec	(TBIV = 0)
#define	sti_sec	_BIS (TBCCTL1, CCIE)
#define	cli_sec	_BIC (TBCCTL1, CCIE)
#define sti_tim	_BIS (TBCCTL0, CCIE)
#define cli_tim	_BIC (TBCCTL0, CCIE)
#define	dis_tim _BIC (TBCTL, MC0 + MC1)

#if SEPARATE_SECONDS_CLOCK
// Timer running in continuous mode
#define	ena_tim _BIS (TBCTL, MC1      )
#else
// Timer running in "up" mode
#define	ena_tim _BIS (TBCTL, MC0      )
#endif

#endif

#if	__TCI_CONFIG__ == 2

// Timer for clock configuration 2 = TIMER A0

#define	TCI_CCR		TA0CCR0
#define	TCI_CCS		TA0CCR1
#define	TCI_VAL		TA0R
#define	TCI_CTL		TA0CTL
#define	TCI_VECTOR	TIMER0_A0_VECTOR
#define	TCI_VECTOR_S	TIMER0_A1_VECTOR

#define	ack_sec	(TA0IV = 0)
#define	sti_sec	_BIS (TA0CCTL1, CCIE)
#define	cli_sec	_BIC (TA0CCTL1, CCIE)
#define sti_tim	_BIS (TA0CCTL0, CCIE)
#define cli_tim	_BIC (TA0CCTL0, CCIE)
#define	dis_tim _BIC (TA0CTL, MC0 + MC1)

#if SEPARATE_SECONDS_CLOCK
#define	ena_tim _BIS (TA0CTL, MC1      )
#else
#define	ena_tim _BIS (TA0CTL, MC0      )
#endif

#endif

// ============================================================================

// Subdivision of the seconds clock timer (exactly one tick per second)
#define	TCI_SEC_DIV	(CRYSTAL_RATE/8)

// Initializers for the timer in "up" mode, i.e., when there is no
// separate seconds clock
#define	TCI_INIT_HIGH	(TCI_HIGH_DIV - 1)
#define	TCI_INIT_LOW  	(TCI_LOW_DIV - 1)

// ============================================================================

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

#define	STACK_SENTINEL	0xB779

#if STACK_GUARD
#define	check_stack_overflow \
			 do { \
				if (*(((word*)STACK_END)-1) != STACK_SENTINEL) \
					syserror (ESTACK, "st"); \
			} while (0)
#else
#define	check_stack_overflow	CNOP
#endif

#ifdef	__PORTMAPPER__

typedef	struct {

	byte *pm_base;
	const byte map [8];

} portmap_t;

#define	portmap_entry(a,b,c,d,e,f,g,h,i) \
		{ (byte*) (&(a)), \
			(byte) (b), (byte) (c), (byte) (d), (byte) (e), \
			(byte) (f), (byte) (g), (byte) (h), (byte) (i)  }

#endif	/* __PORTMAPPER__ */

#include "uart_def.h"
						// LWA of stack
#if	UART_DRIVER

typedef struct	{
/* ============================== */
/* UART with two circular buffers */
/* ============================== */
#if UART_DRIVER > 1
	byte selector;
#endif
	volatile byte flags;
	byte out;
#if	UART_INPUT_BUFFER_LENGTH < 2
	byte in;
#else
	byte in [UART_INPUT_BUFFER_LENGTH];
	byte ib_in, ib_out, ib_count;
#endif
} uart_t;

#if UART_DRIVER > 1
#define	__usel(u,a,b)	do { if ((u)->selector) { a; } else { b; } } while (0)
#define	__ualt(u,a,b)	((u)->selector?(a):(b))
#else
#define	__usel(u,a,b)	do { b; } while (0)
#define	__ualt(u,a,b)	(b)
#endif

extern uart_t zz_uart [];

#define	UART_BASE		UART_A

#define	UART_FLAGS_IN		0x80
#define	UART_FLAGS_OUT		0x40
#define	UART_FLAGS_LOCK		0x20

#endif	/* UART_DRIVER */

// Also needed by UART_TCV (if UART_DRIVER is 0)
#define	UART_RATE_MASK		0x0F

// ============================================================================

#define	sti	_EINT ()
#define	cli	_DINT ()

/* =================== */
/* Watchdog operations */
/* =================== */

#define	WATCHDOG_STOP		WDTCTL = WDTPW + WDTHOLD

#if WATCHDOG_ENABLED

#define	WATCHDOG_HOLD		WATCHDOG_STOP

// 1 second at 32kHz

// ============================================================================
#ifdef	WDTIS_4
// x54xx WDT
#define	WATCHDOG_START		WDTCTL = WDTPW + WDTCNTCL + WDTIS_4 + \
								WDTSSEL__ACLK
#else
#define	WATCHDOG_START		WDTCTL = WDTPW + WDTCNTCL + WDTSSEL
#endif
// ============================================================================

#define	WATCHDOG_CLEAR		WDTCTL = WDTPW + WDTCNTCL

#else

#define	WATCHDOG_HOLD		CNOP
#define	WATCHDOG_START		CNOP
#define	WATCHDOG_CLEAR		CNOP

#endif

#define	WATCHDOG_RESUME		WATCHDOG_START

#ifdef	MONITOR_PIN_CPU

#define	CPU_MARK_IDLE	_PVS (MONITOR_PIN_CPU, 0)
#define	CPU_MARK_BUSY	_PVS (MONITOR_PIN_CPU, 1)

#else

#define	CPU_MARK_IDLE	CNOP
#define	CPU_MARK_BUSY	CNOP

#endif

#define	power_down_mode	(zz_systat.pdmode)
#define	clock_down_mode (TCI_CCR == TCI_INIT_LOW)

#define	SLEEP	do { \
			CPU_MARK_IDLE; \
			if (power_down_mode) { \
				cli; \
				if (zz_systat.evntpn) { \
					sti; \
				} else { \
					_BIS_SR (LPM3_bits + GIE); \
				} \
			} else { \
				cli; \
				if (zz_systat.evntpn) { \
					sti; \
				} else { \
					_BIS_SR (LPM0_bits + GIE); \
				} \
			} \
			zz_systat.evntpn = 0; \
			CPU_MARK_BUSY; \
		} while (0)

#if 1
// used to be: #if NESTED_INTERRUPTS
/*
 * Although it may appear a bit more costly, this way of triggering scheduler
 * events from interrupts should be preferred. This is because this version of
 * RISE_N_SHINE can be called from a nested function called from the proper
 * interrupt function. Also, there is no harm when called from a non-interrupt
 * (and may be sometimes useful). Besides, with NESTED_INTERRUPTS, this is the
 * only formally correct way, as the RISE_N_SHINE status must be conveyed down
 * to the very bottom of the interrupt stack.
 *
 */
#define	RISE_N_SHINE	do { zz_systat.evntpn = 1; } while (0)
#define	RTNI		do { \
				if (zz_systat.evntpn) \
					_BIC_SR_IRQ (LPM4_bits); \
				return; \
			} while (0)
#else	/* dead code */

#define	RISE_N_SHINE	do { \
				zz_systat.evntpn = 1; \
				_BIC_SR_IRQ (LPM4_bits); \
			} while (0)
#define	RTNI		return

#endif	/* dead code, was: NESTED_INTERRUPTS */


#if LEDS_DRIVER
#include "leds_sys.h"
#endif

#endif
