#ifndef	__pg_uart_def_h
#define	__pg_uart_def_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is a complete set of macros for UART operation, such that the rest can
// be CPU independent; this should be included by mach.h
//

//
// Even though CPU-specific differences may be small, it makes better sense
// to copy the whole thing into a separate chunk instead of messing up a
// given chunk with conditions
//

#include "mach.h"


#if __UART_CONFIG__ == 1

// ============================================================================
// Standard MSP430F1xx: two UARTs [shared part] ===============================
// ============================================================================

//
// Clock and rate calculation
//

typedef struct	{
	word rate;
	byte A, B;
} uart_rate_table_entry_t;

// ============================================================================

#if CRYSTAL2_RATE
// SMCLK
#define	UART_UTCTL	SSEL1
#define	UART_CLOCK_RATE	CRYSTAL2_RATE
#else
// ACLK
#define	UART_UTCTL	SSEL0
#define	UART_CLOCK_RATE	CRYSTAL_RATE
#endif

#define	uart_0_set_clock	UTCTL0 = UART_UTCTL;
#define	uart_1_set_clock	UTCTL0 = UART_UTCTL;

#define	uart_0_enable		do { \
					_BIS (ME1, UTXE0 + URXE0); \
					_BIS (UCTL0, CHAR) ; \
				} while (0)

#define	uart_1_enable		do { \
					_BIS (ME2, UTXE1 + URXE1); \
					_BIS (UCTL1, CHAR) ; \
				} while (0)

#if	UART_CLOCK_RATE == 32768
// This is the standard
#define	UART_UBR1		0

#if UART_RATE == 1200
#define	UART_UBR0		0x1B
#define	UART_UMCTL		0x03
#define	UART_RATE_INDEX	0
#endif

#if UART_RATE == 2400
#define	UART_UBR0		0x0D
#define	UART_UMCTL		0x6B
#define	UART_RATE_INDEX	1
#endif

#if UART_RATE == 4800
#define	UART_UBR0		0x06
#define	UART_UMCTL		0x6F
#define	UART_RATE_INDEX	2
#endif

#if UART_RATE == 9600
#define	UART_UBR0		0x03
#define	UART_UMCTL		0x4A
#define	UART_RATE_INDEX	3
#endif

#ifndef UART_UBR0
#error "Illegal UART_RATE, can be 1200, 2400, 4800, 9600"
#endif

#define	UART_RATE_TABLE		{ \
    					{ 12, 0x1B, 0x03 }, \
    					{ 24, 0x0D, 0x6B }, \
    					{ 48, 0x06, 0x6F }, \
    					{ 96, 0x03, 0x4A }  \
				}

#define	uart_0_set_rate(t)	do { \
					UBR00 = (t).A; \
					UBR10 = 0; \
					UMCTL0 = (t).B; \
				} while (0)

#define	uart_1_set_rate(t)	do { \
					UBR01 = (t).A; \
					UBR11 = 0; \
					UMCTL1 = (t).B; \
				} while (0)

#else	/* UART_CLOCK_RATE > 32768 */

#if UART_RATE == 1200
#define	UART_RATE_INDEX	0
#endif
#if UART_RATE == 2400
#define	UART_RATE_INDEX	1
#endif
#if UART_RATE == 4800
#define	UART_RATE_INDEX	2
#endif
#if UART_RATE == 9600
#define	UART_RATE_INDEX	3
#endif
#if UART_RATE == 14400
#define	UART_RATE_INDEX	4
#endif
#if UART_RATE == 19200
#define	UART_RATE_INDEX	5
#endif
#if UART_RATE == 28800
#define	UART_RATE_INDEX	6
#endif
#if UART_RATE == 38400
#define	UART_RATE_INDEX	7
#endif
#if UART_RATE == 76800
#define	UART_RATE_INDEX	8
#endif
#if UART_RATE == 115200
#define	UART_RATE_INDEX	9
#endif
#if UART_RATE == 256000
#define	UART_RATE_INDEX	10
#endif

#ifndef	UART_RATE_INDEX
#error "Illegal UART_RATE"
#endif

// No need to use corrections for high-speed crystals. FIXME: may need 
// correction for 115200 and more.
#define	UART_UMCTL		0
#define	UART_UBR0		((UART_CLOCK_RATE/UART_RATE) % 256)
#define	UART_UBR1		((UART_CLOCK_RATE/UART_RATE) / 256)

#define	UART_RATE_TABLE		{ \
    {	12, (UART_CLOCK_RATE/  1200) % 256, (UART_CLOCK_RATE/  1200) / 256 }, \
    {	24, (UART_CLOCK_RATE/  2400) % 256, (UART_CLOCK_RATE/  2400) / 256 }, \
    {	48, (UART_CLOCK_RATE/  4800) % 256, (UART_CLOCK_RATE/  4800) / 256 }, \
    {	96, (UART_CLOCK_RATE/  9600) % 256, (UART_CLOCK_RATE/  9600) / 256 }, \
    {  144, (UART_CLOCK_RATE/ 14400) % 256, (UART_CLOCK_RATE/ 14400) / 256 }, \
    {  192, (UART_CLOCK_RATE/ 19200) % 256, (UART_CLOCK_RATE/ 19200) / 256 }, \
    {  288, (UART_CLOCK_RATE/ 28800) % 256, (UART_CLOCK_RATE/ 28800) / 256 }, \
    {  384, (UART_CLOCK_RATE/ 38400) % 256, (UART_CLOCK_RATE/ 38400) / 256 }, \
    {  768, (UART_CLOCK_RATE/ 76800) % 256, (UART_CLOCK_RATE/ 76800) / 256 }, \
    { 1152, (UART_CLOCK_RATE/115200) % 256, (UART_CLOCK_RATE/115200) / 256 }, \
    { 2560, (UART_CLOCK_RATE/256000) % 256, (UART_CLOCK_RATE/256000) / 256 }  \
				}

#define	uart_0_set_rate(t)	do { \
					UBR00 = (t).A; \
					UBR10 = (t).B; \
					UMCTL0 = 0; \
				} while (0)

#define	uart_1_set_rate(t)	do { \
					UBR01 = (t).A; \
					UBR11 = (t).B; \
					UMCTL1 = 0; \
				} while (0)

#endif	/* UART_CLOCK_RATE */

// ============================================================================

#define	uart_0_set_rate_def	do { \
					UBR00 = UART_UBR0; \
					UBR10 = UART_UBR1; \
					UMCTL0 = UART_UMCTL; \
				} while (0)

#define	uart_1_set_rate_def	do { \
					UBR01 = UART_UBR0; \
					UBR11 = UART_UBR1; \
					UMCTL1 = UART_UMCTL; \
				} while (0)

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

#define	uart_save_ie_flags(s)		do { \
						(s) = IE1 | ((word)IE2 << 8); \
						IE1 = 0; \
						IE2 = 0; \
					} while (0)

#define	uart_restore_ie_flags(s)	do { \
						IE1 = (byte)  (s)      ; \
						IE2 = (byte) ((s) >> 8); \
					} while (0)

// ============================================================================

#ifdef __SWAPPED_UART__

// ============================================================================
// Swapped UARTs A <-> B ======================================================
// ============================================================================

#define	uart_b_disable_int		_BIC (IE1, URXIE0 + UTXIE0)
#define	uart_b_disable_read_int		_BIC (IE1, URXIE0)
#define	uart_b_disable_write_int	_BIC (IE1, UTXIE0)
#define	uart_b_enable_read_int		_BIS (IE1, URXIE0)
#define	uart_b_enable_write_int		_BIS (IE1, UTXIE0)
#define uart_b_set_read_int		_BIS (IFG1, URXIFG0)
#define uart_b_set_write_int		_BIS (IFG1, UTXIFG0)
#define uart_b_get_read_int		(IFG1 & URXIFG0)
#define uart_b_get_write_int		(IFG1 & UTXIFG0)
#define	uart_b_get_int_stat		(IE1 & (URXIE0 + UTXIE0))
#define	uart_b_set_int_stat(w)		_BIS (IE1, (w) & (URXIE0 + UTXIE0))
#define	uart_b_reset_on			_BIS (UCTL0, SWRST)
#define	uart_b_reset_off		_BIC (UCTL0, SWRST)
#define	uart_b_wait_tx			while ((UTCTL0 & TXEPT) == 0)
#define	uart_b_set_clock		uart_0_set_clock
#define	uart_b_set_rate(t)		uart_0_set_rate (t)
#define	uart_b_set_rate_def		uart_0_set_rate_def
#define	uart_b_enable			uart_0_enable
#define	uart_b_write(b)			TXBUF0 = (b)
#define	uart_b_read			RXBUF0

#define	uart_a_disable_int		_BIC (IE2, URXIE1 + UTXIE1)
#define	uart_a_disable_read_int		_BIC (IE2, URXIE1)
#define	uart_a_disable_write_int	_BIC (IE2, UTXIE1)
#define	uart_a_enable_read_int		_BIS (IE2, URXIE1)
#define	uart_a_enable_write_int		_BIS (IE2, UTXIE1)
#define uart_a_set_read_int		_BIS (IFG1, URXIFG0)
#define uart_a_set_write_int		_BIS (IFG1, UTXIFG0)
#define uart_a_get_read_int		(IFG1 & URXIFG0)
#define uart_a_get_write_int		(IFG1 & UTXIFG0)
#define	uart_a_get_int_stat		(IE2 & (URXIE1 + UTXIE1))
#define	uart_a_set_int_stat(w)		_BIS (IE2, (w) & (URXIE1 + UTXIE1))
#define	uart_a_reset_on			_BIS (UCTL1, SWRST)
#define	uart_a_reset_off		_BIC (UCTL1, SWRST)
#define	uart_a_wait_tx			while ((UTCTL1 & TXEPT) == 0)
#define	uart_a_set_clock		uart_1_set_clock
#define	uart_a_set_rate(t)		uart_1_set_rate (t)
#define	uart_a_set_rate_def		uart_1_set_rate_def
#define	uart_a_enable			uart_1_enable
#define	uart_a_write(b)			TXBUF1 = (b)
#define	uart_a_read			RXBUF1

#define	UART_B_TX_VECTOR	UART0TX_VECTOR
#define	UART_B_RX_VECTOR	UART0RX_VECTOR
#define	UART_A_TX_VECTOR	UART1TX_VECTOR
#define	UART_A_RX_VECTOR	UART1RX_VECTOR

//
// Pin initialization, can be overriden by board
//

// ============================================================================

#ifndef	UART_PREINIT_B

#define	UART_PREINIT_B	do { \
		_BIC (P3DIR, 0x20); \
		_BIS (P3DIR, 0x10); \
		_BIS (P3SEL, 0x30); \
	} while (0)

#endif

// ============================================================================

#ifndef UART_PREINIT_A

#define	UART_PREINIT_A	do { \
		_BIC (P3DIR, 0x80); \
		_BIS (P3DIR, 0x40); \
		_BIS (P3SEL, 0xc0); \
	} while (0)

#endif

// ============================================================================

#else

// ============================================================================
// Straight configuration =====================================================
// ============================================================================

#define	uart_a_disable_int		_BIC (IE1, URXIE0 + UTXIE0)
#define	uart_a_disable_read_int		_BIC (IE1, URXIE0)
#define	uart_a_disable_write_int	_BIC (IE1, UTXIE0)
#define	uart_a_enable_read_int		_BIS (IE1, URXIE0)
#define	uart_a_enable_write_int		_BIS (IE1, UTXIE0)
#define uart_a_set_read_int		_BIS (IFG1, URXIFG0)
#define uart_a_set_write_int		_BIS (IFG1, UTXIFG0)
#define uart_a_get_read_int		(IFG1 & URXIFG0)
#define uart_a_get_write_int		(IFG1 & UTXIFG0)
#define	uart_a_get_int_stat		(IE1 & (URXIE0 + UTXIE0))
#define	uart_a_set_int_stat(w)		_BIS (IE1, (w) & (URXIE0 + UTXIE0))
#define	uart_a_reset_on			_BIS (UCTL0, SWRST)
#define	uart_a_reset_off		_BIC (UCTL0, SWRST)
#define	uart_a_wait_tx			while ((UTCTL0 & TXEPT) == 0)
#define	uart_a_set_clock		uart_0_set_clock
#define	uart_a_set_rate(t)		uart_0_set_rate (t)
#define	uart_a_set_rate_def		uart_0_set_rate_def
#define	uart_a_enable			uart_0_enable
#define	uart_a_write(b)			TXBUF0 = (b)
#define	uart_a_read			RXBUF0

#define	uart_b_disable_int		_BIC (IE2, URXIE1 + UTXIE1)
#define	uart_b_disable_read_int		_BIC (IE2, URXIE1)
#define	uart_b_disable_write_int	_BIC (IE2, UTXIE1)
#define	uart_b_enable_read_int		_BIS (IE2, URXIE1)
#define	uart_b_enable_write_int		_BIS (IE2, UTXIE1)
#define uart_b_set_read_int		_BIS (IFG1, URXIFG0)
#define uart_b_set_write_int		_BIS (IFG1, UTXIFG0)
#define uart_b_get_read_int		(IFG1 & URXIFG0)
#define uart_b_get_write_int		(IFG1 & UTXIFG0)
#define	uart_b_get_int_stat		(IE2 & (URXIE1 + UTXIE1))
#define	uart_b_set_int_stat(w)		_BIS (IE2, (w) & (URXIE1 + UTXIE1))
#define	uart_b_reset_on			_BIS (UCTL1, SWRST)
#define	uart_b_reset_off		_BIC (UCTL1, SWRST)
#define	uart_b_wait_tx			while ((UTCTL1 & TXEPT) == 0)
#define	uart_b_set_clock		uart_1_set_clock
#define	uart_b_set_rate(t)		uart_1_set_rate (t)
#define	uart_b_set_rate_def		uart_1_set_rate_def
#define	uart_b_enable			uart_1_enable
#define	uart_b_write(b)			TXBUF1 = (b)
#define	uart_b_read			RXBUF1

#define	UART_A_TX_VECTOR	UART0TX_VECTOR
#define	UART_A_RX_VECTOR	UART0RX_VECTOR
#define	UART_B_TX_VECTOR	UART1TX_VECTOR
#define	UART_B_RX_VECTOR	UART1RX_VECTOR

//
// Pin initialization, can be overriden by board
//

// ============================================================================

#ifndef	UART_PREINIT_A

#define	UART_PREINIT_A	do { \
		_BIC (P3DIR, 0x20); \
		_BIS (P3DIR, 0x10); \
		_BIS (P3SEL, 0x30); \
	} while (0)

#endif

// ============================================================================

#ifndef UART_PREINIT_B

#define	UART_PREINIT_B	do { \
		_BIC (P3DIR, 0x80); \
		_BIS (P3DIR, 0x40); \
		_BIS (P3SEL, 0xc0); \
	} while (0)

#endif

// ============================================================================

#endif	/* Straight or reversed */


#endif	/* __UART_CONFIG__ == 1 */

// ============================================================================
// ============================================================================
// ============================================================================

#endif
