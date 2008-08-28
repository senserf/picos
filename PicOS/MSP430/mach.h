#ifndef __pg_mach_h
#define	__pg_mach_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
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

#ifdef		__MSP430_148__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#endif

#ifdef		__MSP430_149__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#endif

#ifdef		__MSP430_1611__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2800	// 10240
#define       	__MSP430_1xx__
#endif

#ifdef		__MSP430_449__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_4xx__
#endif

#ifdef		__MSP430_G4617__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		UART_for_4xx	1
#endif

#ifdef		__MSP430_G4618__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		UART_for_4xx	1
#endif

#ifdef		__MSP430_G4619__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_4xx__
#define		UART_for_4xx	1
#endif

#ifndef		RAM_SIZE
#error	"untried yet CPU type: check MSP430/mach.h"
#endif

// ============================================================================

#ifdef	UART_for_4xx

// UART definition for 4xx boards: we assume that there is only one UART driven
// by USART1: TX = P4.0, RX = P4.1

#define		UART_1_IS_A	1

#define	UART_PREINIT_A	do { \
		_BIS (P4SEL, 0x03); \
	} while (0)

#endif	/* UART_for_4xx */

// ============================================================================

#ifndef	UART_PREINIT_A

// Pin setting for pre-initialization of UARTs

#define	UART_PREINIT_A	do { \
		_BIC (P3DIR, 0x26); \
		_BIS (P3DIR, 0x10); \
		_BIS (P3SEL, 0x30); \
	} while (0)

// Flow control add on (UART0 only)

#define	UART_PREINIT_A_FC do { \
		_BIC (P3SEL, 0xc0); \
		_BIS (P3DIR, 0x80); \
	} while (0)

// Set ready to receive
#define	UART_SET_RTR	_BIS (P3OUT, 0x80)
// Clear ready to receive
#define	UART_CLR_RTR	_BIC (P3OUT, 0x80)
// Test peer ready to receive
#define	UART_TST_RTR	((P3IN & 0x40) != 0)

#define	UART_PREINIT_B	do { \
		_BIC (P3DIR, 0x89); \
		_BIS (P3DIR, 0x40); \
		_BIS (P3SEL, 0xc0); \
	} while (0)

#endif	/* UART_PREINIT_A */

// ============================================================================

#ifdef	UART_1_IS_A

// Swapped UARTs, master is USART1

#define	UCTL_B		UCTL0
#define	UCTL_A		UCTL1
#define	UBR0_B		UBR00
#define	UBR0_A		UBR01
#define	UBR1_B		UBR10
#define	UBR1_A		UBR11
#define	UMCTL_B		UMCTL0
#define	UMCTL_A		UMCTL1
#define	UTCTL_B		UTCTL0
#define	UTCTL_A		UTCTL1
#define ME_B		ME1
#define	ME_A		ME2
#define	UTXIFG_B	UTXIFG0
#define	UTXIFG_A 	UTXIFG1
#define	IFG_B		IFG1
#define	IFG_A		IFG2
#define	TXBUF_B		TXBUF0
#define	TXBUF_A		TXBUF1
#define	RXBUF_B		RXBUF0
#define	RXBUF_A		RXBUF1
#define	URXIE_B		URXIE0
#define	UTXIE_B		UTXIE0
#define	URXIFG_A	URXIFG1
#define	UTXIFG_A	UTXIFG1
#define	URXIFG_B	URXIFG0
#define	UTXIFG_B	UTXIFG0
#define	URXIE_A		URXIE1
#define	UTXIE_A		UTXIE1
#define	UTXE_A		UTXE1
#define	URXE_A		URXE1
#define	UTXE_B		UTXE0
#define	URXE_B		URXE0
#define	IE_B		IE1
#define	IE_A		IE2

#define	UART_B_TX_VECTOR	UART0TX_VECTOR
#define	UART_B_RX_VECTOR	UART0RX_VECTOR
#define	UART_A_TX_VECTOR	UART1TX_VECTOR
#define	UART_A_RX_VECTOR	UART1RX_VECTOR

#else	/* swapped uarts */

// Assume the master uart is USART0, second uart is USART1

#define	UCTL_A		UCTL0
#define	UCTL_B		UCTL1
#define	UBR0_A		UBR00
#define	UBR0_B		UBR01
#define	UBR1_A		UBR10
#define	UBR1_B		UBR11
#define	UMCTL_A		UMCTL0
#define	UMCTL_B		UMCTL1
#define	UTCTL_A		UTCTL0
#define	UTCTL_B		UTCTL1
#define ME_A		ME1
#define	ME_B		ME2
#define	UTXIFG_A	UTXIFG0
#define	UTXIFG_B 	UTXIFG1
#define	IFG_A		IFG1
#define	IFG_B		IFG2
#define	TXBUF_A		TXBUF0
#define	TXBUF_B		TXBUF1
#define	RXBUF_A		RXBUF0
#define	RXBUF_B		RXBUF1
#define	URXIE_A		URXIE0
#define	UTXIE_A		UTXIE0
#define	URXIFG_A	URXIFG0
#define	UTXIFG_A	UTXIFG0
#define	URXIFG_B	URXIFG1
#define	UTXIFG_B	UTXIFG1
#define	URXIE_B		URXIE1
#define	UTXIE_B		UTXIE1
#define	UTXE_A		UTXE0
#define	URXE_A		URXE0
#define	UTXE_B		UTXE1
#define	URXE_B		URXE1
#define	IE_A		IE1
#define	IE_B		IE2

#define	UART_A_TX_VECTOR	UART0TX_VECTOR
#define	UART_A_RX_VECTOR	UART0RX_VECTOR
#define	UART_B_TX_VECTOR	UART1TX_VECTOR
#define	UART_B_RX_VECTOR	UART1RX_VECTOR

#endif	/* swapped uarts */

#ifndef	UART0TX_VECTOR
#define	UART0TX_VECTOR	USART0TX_VECTOR
#endif
#ifndef	UART1TX_VECTOR
#define	UART1TX_VECTOR	USART1TX_VECTOR
#endif
#ifndef	UART0RX_VECTOR
#define	UART0RX_VECTOR	USART0RX_VECTOR
#endif
#ifndef	UART1RX_VECTOR
#define	UART1RX_VECTOR	USART1RX_VECTOR
#endif

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


#if	DIAG_MESSAGES || (dbg_level != 0)

#ifdef	DIAG_MESSAGES_TO_LCD

#define	diag_disable_int(a,u)	lcd_clear (0, 0)
#define	diag_wchar(c,a)		lcd_putchar (c)
#define	diag_wait(a)		do { } while (0)
#define	diag_enable_int(a,u)	do { } while (0)

#else	/* DIAG MESSAGES TO UART */

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



#endif	/* DIAG MESSAGES TO UART */

#endif	/* DIAG MESSAGES */

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


#define	uart_a_disable_int		_BIC (IE_A, URXIE_A + UTXIE_A)
#define	uart_b_disable_int		_BIC (IE_B, URXIE_B + UTXIE_B)
#define	uart_a_disable_read_int		_BIC (IE_A, URXIE_A)
#define	uart_a_disable_write_int	_BIC (IE_A, UTXIE_A)
#define	uart_b_disable_read_int		_BIC (IE_B, URXIE_B)
#define	uart_b_disable_write_int	_BIC (IE_B, UTXIE_B)
#define	uart_a_enable_read_int		_BIS (IE_A, URXIE_A)
#define	uart_a_enable_write_int		_BIS (IE_A, UTXIE_A)
#define	uart_b_enable_read_int		_BIS (IE_B, URXIE_B)
#define	uart_b_enable_write_int		_BIS (IE_B, UTXIE_B)

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
