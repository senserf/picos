#ifndef	__pg_uart_def_h
#define	__pg_uart_def_h		1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is a complete set of macros for UART operation, such that the rest can
// be CPU independent
//

//
// Even though CPU-specific differences may be small, it makes better sense
// to copy the whole thing into a separate chunk instead of messing up a
// given chunk with conditions
//

#include "mach.h"

//
// Clock and rate calculation
//

typedef struct	{
	word rate;
	byte A, B;
} uart_rate_table_entry_t;

#if __UART_CONFIG__ == 1

// ============================================================================
// Standard MSP430F1xx: two UARTs [shared part] ===============================
// ============================================================================

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
#define	uart_1_set_clock	UTCTL1 = UART_UTCTL;

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
#error "S: Illegal UART_RATE, can be 1200, 2400, 4800, 9600"
#endif

#define	UART_RATE_TABLE		{ \
    					{ 12, 0x1B, 0x03 }, \
    					{ 24, 0x0D, 0x6B }, \
    					{ 48, 0x06, 0x6F }, \
    					{ 96, 0x03, 0x4A }  \
				}

#define	UART_RATES_AVAILABLE { 12, 24, 48, 96 }

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
#error "S: Illegal UART_RATE"
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

#define	UART_RATES_AVAILABLE \
	 { 12, 24, 48, 96, 144, 192, 288, 384, 768, 1152, 2560 }

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
#define uart_a_set_read_int		_BIS (IFG2, URXIFG1)
#define uart_a_set_write_int		_BIS (IFG2, UTXIFG1)
#define uart_a_get_read_int		(IFG2 & URXIFG1)
#define uart_a_get_write_int		(IFG2 & UTXIFG1)
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
#define uart_b_set_read_int		_BIS (IFG2, URXIFG1)
#define uart_b_set_write_int		_BIS (IFG2, UTXIFG1)
#define uart_b_get_read_int		(IFG2 & URXIFG1)
#define uart_b_get_write_int		(IFG2 & UTXIFG1)
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

#if __UART_CONFIG__ == 2

// CC430 single UART on P1.5, P1.6

#ifndef	UART_FROM_SMCLK
#define	UART_FROM_SMCLK	0
#endif

#if UART_FROM_SMCLK

#ifndef	SMCLK_RATE
#error "SMCLK_RATE undefined!"
#endif

// SMCLK
#define	UART_UTCTL	UCSSEL1
#define	UART_CLOCK_RATE	SMCLK_RATE

#else

// ACLK
#define	UART_UTCTL	UCSSEL0
#define	UART_CLOCK_RATE	CRYSTAL_RATE

#endif



#define	uart_a_set_rate(t)	do { \
					UCA0BR0 = (t) . A; \
					UCA0BR1 = 0; \
					UCA0MCTL = (t) . B; \
				} while (0)

#define	uart_a_set_rate_def	do { \
					UCA0BR0 = UART_UCBR; \
					UCA0BR1 = 0; \
					UCA0MCTL = UART_UCBRS; \
				} while (0)

#define	uart_a_disable_int		_BIC (UCA0IE, UCTXIE + UCRXIE)
#define	uart_a_disable_read_int		_BIC (UCA0IE, UCRXIE)
#define	uart_a_disable_write_int	_BIC (UCA0IE, UCTXIE)
#define	uart_a_enable_read_int		_BIS (UCA0IE, UCRXIE)
#define	uart_a_enable_write_int		_BIS (UCA0IE, UCTXIE)
#define	uart_a_set_read_int		_BIS (UCA0IFG, UCRXIFG)
#define	uart_a_set_write_int		_BIS (UCA0IFG, UCTXIFG)
#define	uart_a_get_read_int		(UCA0IFG & UCRXIFG)
#define	uart_a_get_write_int		(UCA0IFG & UCTXIFG)
#define	uart_a_get_int_stat		(UCA0IE & (UCTXIE + UCRXIE))
#define	uart_a_set_int_stat(w)		_BIS (UCA0IE, (w) & (UCTXIE + UCRXIE))
#define	uart_a_reset_on			_BIS (UCA0CTL1, UCSWRST)
#define	uart_a_reset_off		_BIC (UCA0CTL1, UCSWRST)
#define	uart_a_wait_tx			while (!uart_a_get_write_int)
#define	uart_a_set_clock 		UCA0CTL1 = (UART_UTCTL | UCSWRST)

// Note: the default (reset) setting of UCAxCTL0 is fine

#define	uart_a_enable 			_BIS (UCA0IE, UCTXIE + UCRXIE)

#define	uart_a_write(b)			UCA0TXBUF = (b)
#define	uart_a_read			UCA0RXBUF

// A single vector for both TX and RX
#define	UART_A_TX_RX_VECTOR	USCI_A0_VECTOR

#define	uart_a_tx_interrupt	(UCA0IV == 4)

#ifndef	UART_PREINIT_A

#define	UART_PREINIT_A	do { \
				_BIC (P1DIR, 0x20); \
				_BIS (P1DIR, 0x40); \
				_BIS (P1SEL, 0x60); \
			} while (0)
#endif

#define	uart_save_ie_flags(s)		do { \
						(s) = UCA0IE; \
						UCA0IE = 0; \
					} while (0)

#define	uart_restore_ie_flags(s)	UCA0IE = (s)

// ============================================================================
// Macros to transform bit rates into prescaler and modulator
// ============================================================================

// Calculate rnd [ (clock / r - int (clock / r)) * 8 ]
#define	_uu_rnd8_x(r)	((((UART_CLOCK_RATE*16)/(r) & 0xF) + 1) >> 1)

// Calculate rnd [ (clock / r / 16 - int (clock / r / 16)) * 16]
#define	_uu_rndF_x(r)	((((UART_CLOCK_RATE*2)/(r) & 0x1F) + 1) >> 1)

// Modulus for non-versampling mode (shifted << 1)
#define	_uu_modu_n(r)	((_uu_rnd8_x (r) < 8 ? _uu_rnd8_x (r) : \
				_uu_rnd8_x (r) - 1) << 1)

// Modulus for oversampling mode ( << 1 | 1 )
#define _uu_modu_o(r)	(((_uu_rndF_x (r) < 16 ? _uu_rndF_x (r) : \
				_uu_rndF_x (r) - 1) << 4) | 1)

// Prescaler non-oversampling
#define _uu_pres_n(r)	(UART_CLOCK_RATE/(r))

// Prescaler oversampling
#define	_uu_pres_o(r)	((UART_CLOCK_RATE/(r)) / 16)

// Modulus general
#define	_uu_modu(r)	(((UART_CLOCK_RATE/(r)) >= 16) ? \
				_uu_modu_o (r) : _uu_modu_n (r))

// Prescaler general
#define	_uu_pres(r)	(((UART_CLOCK_RATE/(r)) >= 16) ? \
				_uu_pres_o (r) : _uu_pres_n (r))

#if	UART_CLOCK_RATE == 32768
// We go up to 9600

// Make sure the rate is standard

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

#ifndef	UART_RATE_INDEX
#error "S: illegal UART_RATE, can be 1200, 2400, 4800, 9600"
#endif

#define UART_UCBR	_uu_pres_n (UART_RATE)
#define	UART_UCBRS	_uu_modu_n (UART_RATE)

#define	UART_RATE_TABLE	{\
				{ 12, _uu_pres_n (1200), _uu_modu_n (1200) }, \
				{ 24, _uu_pres_n (2400), _uu_modu_n (2400) }, \
				{ 48, _uu_pres_n (4800), _uu_modu_n (4800) }, \
				{ 96, _uu_pres_n (9600), _uu_modu_n (9600) }  \
			}

#define	UART_RATES_AVAILABLE { 12, 24, 48, 96 }

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
#error "S: Illegal UART_RATE, see MSP430/uart_def.h"
#endif

#define	UART_UCBR	_uu_pres (UART_RATE)
#define	UART_UCBRS	_uu_modu (UART_RATE)

#define	UART_RATE_TABLE	{ \
    {	48, _uu_pres (  4800), _uu_modu (  4800) }, \
    {	96, _uu_pres (  9600), _uu_modu (  9600) }, \
    {  144, _uu_pres ( 14400), _uu_modu ( 14400) }, \
    {  192, _uu_pres ( 19200), _uu_modu ( 19200) }, \
    {  288, _uu_pres ( 28800), _uu_modu ( 28800) }, \
    {  384, _uu_pres ( 38400), _uu_modu ( 38400) }, \
    {  768, _uu_pres ( 76800), _uu_modu ( 76800) }, \
    { 1152, _uu_pres (115200), _uu_modu (115200) }, \
    { 2560, _uu_pres (256000), _uu_modu (256000) }  \
}

#define	UART_RATES_AVAILABLE \
	 { 48, 96, 144, 192, 288, 384, 768, 1152, 2560 }

#endif	/* UART_CLOCK_RATE > 32768 */

#endif	/* __UART_CONFIG__ == 2 */

// ============================================================================
// ============================================================================
// ============================================================================

#endif
