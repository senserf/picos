/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __pg_mach_h
#define	__pg_mach_h		1

#include <stdarg.h>
#include "supplement.h"
#include "portnames.h"

/* ========================================================================== */
/*                     PicOS                                                  */
/*                                                                            */
/* Hardware-specific definitions                                              */
/*                                                                            */

#ifndef	__MSP430__
#error "S: this must be compiled with mspgcc!!!"
#endif

// The default power down mode for MSP430 is level 1, i.e., there is only
// one power down mode, e.g., in contrast to CC13XX
#define	DEFAULT_PD_MODE	1

#define	LITTLE_ENDIAN	1
#define	BIG_ENDIAN	0

// ============================================================================
// CPU type dependencies ======================================================
// ============================================================================

#ifdef		__msp430f148__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		__FLASH_TYPE__	1
#endif

#ifdef		__msp430f149__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		__FLASH_TYPE__	1
#endif

#ifdef		__msp430f1611__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2800	// 10240
#define       	__MSP430_1xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		__FLASH_TYPE__	1
#endif

#ifdef		__msp430f449__
#define		RAM_START	0x200
#define		RAM_SIZE	0x800	// 2048
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		__FLASH_TYPE__	1
#endif

#ifdef		__msp430fG4617__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
// Different pins
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#define		__FLASH_TYPE__	1
#endif

#ifdef		__MSP430fG4618__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x2000	// 8K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#define		__FLASH_TYPE__	1
#endif

#ifdef		__msp430fG4619__
#define		RAM_START	0x1100
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_4xx__
#define		__UART_CONFIG__	1
#define		__ADC_CONFIG__	1
#define		__TCI_CONFIG__	1
#define		UART_PREINIT_A	_BIS (P4SEL, 0x03)
#define		UART_PREINIT_B	CNOP
#define		__SWAPPED_UARTS__
#define		__FLASH_TYPE__	1
#endif

#ifdef		__cc430f6137__
#define		RAM_START	0x1C00
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_6xx__
#define       	__CC430_6xx__
#define		__CC430__
#define		__PORTMAPPER__
#define		__UART_CONFIG__	2
#define		__ADC_CONFIG__	3	// Extended ADC with REF module
#define		__TCI_CONFIG__	2
#define		__FLASH_TYPE__	2
// DCO/FLL parameters
#define		FLL_DIVIDER	FLLD_1	// fDCO = fDCOCLKDIV * 2
#define		DCO_MULTIPLIER	366
// This yields 12,025,856 Hz
#define		SMCLK_RATE	(CRYSTAL_RATE * (DCO_MULTIPLIER + 1))
#endif

#ifdef		__cc430f5137__
#define		RAM_START	0x1C00
#define		RAM_SIZE	0x1000	// 4K
#define       	__MSP430_5xx__
#define       	__CC430_5xx__
#define		__CC430__
#define		__PORTMAPPER__
#define		__NO_PORT_P4
#define		__UART_CONFIG__	2
#define		__ADC_CONFIG__	3	// Extended ADC with REF module
#define		__TCI_CONFIG__	2
#define		__FLASH_TYPE__	2
// DCO/FLL parameters
#define		FLL_DIVIDER	FLLD_1	// fDCO = fDCOCLKDIV * 2
#define		DCO_MULTIPLIER	366
// This yields 12,025,856 Hz
#define		SMCLK_RATE	(CRYSTAL_RATE * (DCO_MULTIPLIER + 1))
#endif

#ifndef		RAM_SIZE
#error	"S: unknown (yet) CPU type: check MSP430/mach.h"
#endif

#include "arch.h"

// ============================================================================

#if __FLASH_TYPE__ == 1
//
// FIM type 1 starting at 0x1000, two pages 128 bytes each
//
#define	IFLASH_PAGESIZE	64			// This is in words
#define	CFLASH_PAGESIZE	256			// Also in words
#define	IFLASH_NPAGES	2
#define	IFLASH_START	((address)0x1000)

#define	__FLASH_OP_WRITE__(w,a)	cli; \
				__asm__ __volatile__ ( \
				"mov %0,%1\n\t" \
				"mov %2,%3\n\t" \
				"mov %4,%5\n\t" \
				"mov %6,%7\n\t" \
				"mov %2,%5\n\t" \
				"mov %8,%3\n\t" : : \
				"i"((int)(FWKEY+FSSEL1+11)), "m"(FCTL2), \
				"i"((int)(FWKEY)), "m"(FCTL3), \
				"i"((int)(FWKEY+WRT)), "m"(FCTL1), \
				"m"((int)(w)),"m"((*((word*)(a)))), \
				"i"((int)(FWKEY+LOCK)) ); \
				sti

#define	__FLASH_OP_ERASE__(a)	cli; \
				__asm__ __volatile__ ( \
				"mov %0,%1\n\t" \
				"mov %2,%3\n\t" \
				"mov %4,%5\n\t" \
				"clr 0(%6)\n\t" \
				"mov %7,%3\n\t" : : \
				"i"((int)(FWKEY+FSSEL1+11)), "m"(FCTL2), \
				"i"((int)(FWKEY)), "m"(FCTL3), \
				"i"((int)(FWKEY+ERASE)), "m"(FCTL1), \
				"r"((int)(a)), \
				"i"((int)(FWKEY+LOCK)) ); \
				sti

#define	__FLASH_FIM_UNLOCK__	CNOP
#define	__FLASH_FIM_RELOCK__	CNOP

#endif

#if __FLASH_TYPE__ == 2
//
// CC430: 4 pages at 0x1800
//
#define	IFLASH_PAGESIZE	64
#define	CFLASH_PAGESIZE	256
#define	IFLASH_NPAGES	4
#define	IFLASH_START	((address)0x1800)

#define	__FLASH_OP_WRITE__(w,a)	cli; \
				__asm__ __volatile__ ( \
				"mov %0,%1\n\t" \
				"mov %2,%3\n\t" \
				"mov %4,%5\n\t" \
				"mov %0,%3\n\t" \
				"mov %6,%1\n\t" : : \
				"i"((int)(FWKEY)), "m"(FCTL3), \
				"i"((int)(FWKEY+WRT)), "m"(FCTL1), \
				"m"((int)(w)),"m"((*((word*)(a)))), \
				"i"((int)(FWKEY+LOCK)) ); \
				sti

#define	__FLASH_OP_ERASE__(a)	cli; \
				FCTL3 = FWKEY + LOCKA; \
				__asm__ __volatile__ ( \
				"mov %0,%1\n\t" \
				"mov %2,%3\n\t" \
				"clr 0(%4)\n\t" \
				"mov %5,%1\n\t" : : \
				"i"((int)(FWKEY+LOCKA)), "m"(FCTL3), \
				"i"((int)(FWKEY+ERASE)), "m"(FCTL1), \
				"r"((int)(a)), \
				"i"((int)(FWKEY+LOCK)) ); \
				sti

#define	__FLASH_FIM_UNLOCK__	do { \
					if ((FCTL3 & LOCKA)) \
						FCTL3 = FWKEY + LOCKA; \
				} while (0)

#define	__FLASH_FIM_RELOCK__	do { \
					if ((FCTL3 & LOCKA) == 0) \
						FCTL3 = FWKEY + LOCKA; \
				} while (0)
#endif

#define	IFLASH_SIZE	(IFLASH_NPAGES * IFLASH_PAGESIZE)

// ============================================================================

#define	RAM_END		(RAM_START + RAM_SIZE)
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
		{ (byte*) (&(a)), { \
			(byte) (b), (byte) (c), (byte) (d), (byte) (e), \
			(byte) (f), (byte) (g), (byte) (h), (byte) (i) }}

#endif	/* __PORTMAPPER__ */

// ============================================================================
// Crystals and timers ========================================================
// ============================================================================

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
// ============================================================================
// High crystal rate (1MHz)
// ============================================================================

#if	CRYSTAL_RATE < 1000000
#error "S: CRYSTAL_RATE can be 32768 or >= 1000000"
#endif

#define	HIGH_CRYSTAL_RATE 	1

// We have the high-rate 1MHz crystal
#ifndef	TRIPLE_CLOCK
// We need this with a high-rate crystal; no problem, ebergy savings are off
#define	TRIPLE_CLOCK	0
#endif

#if TRIPLE_CLOCK
#error "S: TRIPLE_CLOCK is incompatible with HIGH_CRYSTAL_RATE"
#endif

// The number of slow ticks per second
#define	TCI_LOW_PER_SEC		16

// No clockdown/clockup modes with HIGH_CRYSTAL_RATE (power savings are not
// possible, anyway)
#define	clockup()	CNOP
#define	clockdown()	CNOP

#define	TCI_LOW_DIV	TCI_HIGH_DIV

#else
// ============================================================================
// Low crystal rate (32768 kHz)
// ============================================================================

#define	HIGH_CRYSTAL_RATE	0

#ifndef	TRIPLE_CLOCK
// This is the default for the low-rate crystal
#define	TRIPLE_CLOCK	1
#endif

// We used this with old WATCHDOG_ENABLED
// #define	TCI_LOW_PER_SEC		2
#define	TCI_LOW_PER_SEC		1

// ============================================================================
// The maximum "hold" second delay to carry out in a single go (converted to
// msec wait) + the maximum hold delay to split into fine 1 msec delays
// ============================================================================

#define	HOLD_BREAK_DELAY_SEC	(TCI_MAXDEL >> 10)
#define	HOLD_STEP_DELAY_SEC	2

// ============================================================================

#define	TCI_LOW_DIV		(CRYSTAL_RATE/(8*TCI_LOW_PER_SEC))

#endif
// ============================================================================
// Crystal rate settled
// ============================================================================

#if	__TCI_CONFIG__ == 1

// Timer for clock configuration 1 = TIMER_B

// Delay interrupt counter
#define	TCI_CCR		TBCCR0
// Seconds interrupt counter (if separate)
#define	TCI_CCS		TBCCR1
#define	TCI_CCA		TBCCR2
#define	TCI_VAL		TBR
#define	TCI_CTL		TBCTL
#define	TCI_VECTOR	TIMERB0_VECTOR
#define	TCI_VECTOR_S	TIMERB1_VECTOR
// Tells debouncer interrupt from seconds clock
#define	TCI_AUXILIARY_TIMER_INTERRUPT	(TBIV == 4)

#define	sti_aux	_BIS (TBCCTL2, CCIE)
#define cli_aux	_BIC (TBCCTL2, CCIE)
#define	sti_sec	_BIS (TBCCTL1, CCIE)
#define	cli_sec	_BIC (TBCCTL1, CCIE)
#define sti_tim	_BIS (TBCCTL0, CCIE)
#define cli_tim	_BIC (TBCCTL0, CCIE)
#define	dis_tim _BIC (TBCTL, MC0 + MC1)

#if TRIPLE_CLOCK
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
#define	TCI_CCA		TA0CCR2
#define	TCI_VAL		TA0R
#define	TCI_CTL		TA0CTL
#define	TCI_VECTOR	TIMER0_A0_VECTOR
#define	TCI_VECTOR_S	TIMER0_A1_VECTOR
#define	TCI_AUXILIARY_TIMER_INTERRUPT	(TA0IV == 4)

#define	sti_aux	_BIS (TA0CCTL2, CCIE)
#define	cli_aux	_BIC (TA0CCTL2, CCIE)
#define	sti_sec	_BIS (TA0CCTL1, CCIE)
#define	cli_sec	_BIC (TA0CCTL1, CCIE)
#define sti_tim	_BIS (TA0CCTL0, CCIE)
#define cli_tim	_BIC (TA0CCTL0, CCIE)
#define	dis_tim _BIC (TA0CTL, MC0 + MC1)

#if TRIPLE_CLOCK
#define	ena_tim _BIS (TA0CTL, MC1      )
#else
#define	ena_tim _BIS (TA0CTL, MC0      )
#endif

#endif

// ============================================================================
// Second clock rate: it can be 1, 2, 4, 8, 16 [231028]
// ============================================================================

#ifndef	SECOND_CLOCK_RATE
#define	SECOND_CLOCK_RATE	1
#endif

#if SECOND_CLOCK_RATE != 1 && SECOND_CLOCK_RATE != 2 && SECOND_CLOCK_RATE != 4 && SECOND_CLOCK_RATE != 8 && SECOND_CLOCK_RATE != 16
#error "S: SECOND_CLOCK_RATE must be one of: 1, 2, 4, 8, 16!"
#endif

#if SECOND_CLOCK_RATE != 1 && TRIPLE_CLOCK == 0
#error "S: SECOND_CLOCK_RATE must be 1 unless TRIPLE_CLOCK is selected!"
#endif

// Timer increment for the required SECOND_CLOCK_RATE; we have the 32K crystal,
// which means that 1 sec = 32768/8 = 4K ticks; so with SECOND_CLOCK_RATE == 1,
// this should yield 4K
#define	TIMER_TICKS_PER_SECOND	(CRYSTAL_RATE/8)
// Note: for SECOND_CLOCK_RATE == 16, this overflows into 64K, which should
// rightfully produce 0 when cast to a word [231028]
#define	TCI_SEC_DIV	(TIMER_TICKS_PER_SECOND * SECOND_CLOCK_RATE)

// ============================================================================

// Initializers for the timer in "up" mode, i.e., when there is no
// separate seconds clock
#define	TCI_INIT_HIGH	(TCI_HIGH_DIV - 1)
#define	TCI_INIT_LOW  	(TCI_LOW_DIV - 1)

// Maximum value for which the delay timer can be set
#define	TCI_MAXDEL		(((word)(65535)) >> 2)

// Conversion from delay (ms) to ticks (only valid for the 32K crystal)
#define	TCI_DELTOTICKS(d)	 ((word)(d) << 2)
#define	TCI_TICKSTODEL(d)	 ((word)(d) >> 2)

// ============================================================================

#if TRIPLE_CLOCK || HIGH_CRYSTAL_RATE

#define	clockup()	CNOP
#define	clockdown()	CNOP

#else

// They are macros
#define	clockup()	(TCI_CCR = TCI_INIT_HIGH)
#define	clockdown()	(TCI_CCR = TCI_INIT_LOW)

#endif

// ============================================================================

#if TRIPLE_CLOCK

// To be invoked from snippets in irq_timer.h
#define	TCI_MARK_AUXILIARY_TIMER_ACTIVE	aux_timer_inactive = 0
#define	TCI_RUN_DELAY_TIMER		tci_run_delay_timer ()
#define	TCI_RUN_AUXILIARY_TIMER		tci_run_auxiliary_timer ()
#define	TCI_UPDATE_DELAY_TICKS(f)	tci_update_delay_ticks (f)

void tci_run_delay_timer ();
void tci_run_auxiliary_timer ();
word tci_update_delay_ticks (Boolean);

#define	cli_utims	cli_aux
#define	sti_utims	sti_aux

#else

#define	TCI_MARK_AUXILIARY_TIMER_ACTIVE	CNOP
#define	TCI_RUN_DELAY_TIMER		CNOP
#define	TCI_RUN_AUXILIARY_TIMER		CNOP
#define	TCI_UPDATE_DELAY_TICKS(f)	CNOP

#define	cli_utims	cli_tim
#define	sti_utims	sti_tim

#endif

// ============================================================================
// UART(s) ====================================================================
// ============================================================================

#include "uart_def.h"

// No other devices on MSP430
#define	MAX_DEVICES	UART_DRIVER

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

extern uart_t __pi_uart [];

#define	UART_BASE		UART_A

#define	UART_FLAGS_IN		0x80
#define	UART_FLAGS_OUT		0x40
#define	UART_FLAGS_NOTRANS	0x20

#endif	/* UART_DRIVER */

// Also needed by UART_TCV (if UART_DRIVER is 0)
#define	UART_RATE_MASK		0x0F

// ============================================================================
// ADC ========================================================================
// ============================================================================

#if __ADC_CONFIG__ == 1

// MSP430F1xx, MSP430F4xx ...

#define	ADC_DEF_CSOURCE		ADC12SSEL_ADC12OSC
#define	ADC_DEF_DIV		ADC12DIV_7
#define	ADC_FLG_ENC		ENC
#define	ADC_FLG_EOS		EOS
#define	ADC_FLG_SHP		SHP
#define	ADC_FLG_REFON		REFON
#define	ADC_FLG_REF25		REF2_5V
#define	ADC_PRT_DIR		P6DIR
#define	ADC_PRT_SEL		P6SEL
#define	ADC_SREF_EVSS		SREF_VEREF_AVSS
#define	ADC_SREF_VVSS		SREF_AVCC_AVSS
#define	ADC_SREF_RVSS		SREF_VREF_AVSS

#define	ADC_CTL2_SET		CNOP

#endif

// ============================================================================

#ifndef	ADC_REFERENCE_OUT
#define	ADC_REFERENCE_OUT	1	// By default, the reference goes out
#endif

#if ADC_REFERENCE_OUT == 1
#define	ADC_REF_OUT	ADC12REFOUT
#else
#define	ADC_REF_OUT	0
#endif

#if __ADC_CONFIG__ == 2 || __ADC_CONFIG__ == 3

// CC430F[56]xx (some fields have different names: this is an "extended ADC")

#define	ADC_DEF_CSOURCE		ADC12SSEL_0
#define	ADC_DEF_DIV		ADC12DIV_7
#define	ADC_FLG_ENC		ADC12ENC
#define	ADC_FLG_EOS		ADC12EOS
#define	ADC_FLG_SHP		ADC12SHP
#define	ADC_FLG_REFON		ADC12REFON
#define	ADC_FLG_REF25		ADC12REF2_5V
#define	ADC_PRT_DIR		P2DIR
#define	ADC_PRT_SEL		P2SEL
#define	ADC_SREF_EVSS		ADC12SREF_2
#define	ADC_SREF_VVSS		ADC12SREF_0
#define	ADC_SREF_RVSS		ADC12SREF_1

// We have a third control register: resolution = 12 bits, slower rate,
// reference out; we may still want to set the burst mode, but, as we
// switch the reference off after measurement, the savings wouldn't be
// terrific; on F6xx, there is also a separate REF module for generating
// reference voltages, which we disable for now (that way the reference
// is controlled the old-fashioned way)
#if __ADC_CONFIG__ == 3
#define	ADC_CTL2_SET	do { \
			    REFCTL0 = 0; \
			    ADC12CTL2 = ADC12RES_2 + ADC12SR + ADC_REF_OUT; \
			} while (0)
#else
#define	ADC_CTL2_SET	ADC12CTL2 = ADC12RES_2 + ADC12SR + ADC_REF_OUT
#endif

#endif

// ============================================================================

//
// The defaults are:
//
//	Internal oscillator (lousy stability: 3.7-6.3 MHz with 5MHz target)
//	divided by 8, yielding about 1.6us sampling unit
//

#ifndef	ADC_CLOCK_SOURCE
#define	ADC_CLOCK_SOURCE	ADC_DEF_CSOURCE
#endif

#ifndef	ADC_CLOCK_DIVIDER
#define	ADC_CLOCK_DIVIDER	ADC_DEF_DIV
#endif

//
// Configuration for polled sample collection:
//
//      - t == 0 -> sample and hold, otherwise timed by t
//	- r == 0 -> int 1.5, 1 -> int 2.5, 2 -> Vcc, 3 -> Veref
//
//	Note: the unit of sampling time is (average) 200ns * 8 = 1.6us
//
#define	adc_config_read(p,r,t)	do { \
				  _BIC (ADC12CTL0, ADC_FLG_ENC); \
				  _BIC (ADC_PRT_DIR, 1 << (p)); \
				  _BIS (ADC_PRT_SEL, 1 << (p)); \
				  ADC_CTL2_SET; \
				  ADC12CTL1 = ADC_CLOCK_DIVIDER + \
							ADC_CLOCK_SOURCE + \
				  ((t) != 0 ? ADC_FLG_SHP : 0); \
				  ADC12MCTL0 = ADC_FLG_EOS + \
				  ((r) > 2 ? ADC_SREF_EVSS : ((r) == 2 ? \
				    ADC_SREF_VVSS : ADC_SREF_RVSS)) + (p); \
				  ADC12CTL0 = ADC12ON + \
				   ((r) == 1 ? ADC_FLG_REF25 : 0) + \
				   ((r) < 2 ? ADC_FLG_REFON : 0) + \
				   (((t) & 0xf) << 12) + (((t) & 0xf) << 8); \
				} while (0)

// For DAC setup
#define	adc_set_reference(r)	do { \
				  _BIC (ADC12CTL0, ADC_FLG_ENC); \
				  ADC12CTL0 = ADC_FLG_REFON + \
				   ((r) ? ADC_FLG_REF25 : 0); \
				} while (0)

#ifdef	PIN_ADC_RSSI
//
// ADC configuration for collecting RSSI
//
#define	adc_config_rssi		do { \
					_BIC (ADC12CTL0, ADC_FLG_ENC); \
					_BIC (ADC_PRT_DIR, 1 << PIN_ADC_RSSI); \
					_BIS (ADC_PRT_SEL, 1 << PIN_ADC_RSSI); \
				        ADC_CTL2_SET; \
					ADC12CTL1 = ADC12DIV_6 + ADC12SSEL_3; \
					ADC12MCTL0 = ADC_FLG_EOS + \
						ADC_SREF_RVSS + INCH_0; \
					ADC12CTL0 = ADC_FLG_REF25 + ADC12ON + \
						ADC_FLG_REFON; \
				} while (0)

#else	/* NO ADC RSSI */

#define	adc_config_rssi		adc_disable

#endif	/* PIN_ADC_RSSI */

// ============================================================================
// Operations =================================================================
// ============================================================================

// Result not available
#define	adc_busy	(ADC12CTL1 & ADC12BUSY)

// Wait for result
#define	adc_wait	do { } while (adc_busy)

// ADC is on
#define	adc_inuse	(ADC12CTL0 & ADC12ON)

// ADC reading
#define	adc_value	ADC12MEM0

#if 0
// ADC operating for the RF receiver; this is heuristic, and perhaps
// not needed (used in pin_read.c to save on a status bit that would have to be
// stored somewhere)
#define	adc_rcvmode	((ADC12CTL1 & ADC12DIV_1) == 0)
// I am removing this ugly hack
#endif

// Explicit end of sample indication
#define	adc_stop	_BIC (ADC12CTL0, ADC12SC)

// Off but possibly less than disable
#define	adc_off		_BIC (ADC12CTL0, ADC_FLG_ENC)

// Complete off, including REF voltage
#define	adc_disable	do { \
				adc_off; \
				_BIC (ADC12CTL0, ADC12ON + ADC_FLG_REFON); \
			} while (0)

// Start measurement
#define	adc_start	do { \
				_BIC (ADC12CTL0, ADC_FLG_ENC); \
				_BIS (ADC12CTL0, ADC12ON); \
				_BIS (ADC12CTL0, ADC12SC + ADC_FLG_ENC); \
			} while (0)

// Start measurement with RFON. This is a mess, but RFON takes current, so I
// want to make sure that it is always off after adc_disable. This requires
// whoever invokes adc_start to know. Otherwise, we would need a separate 
// in-memory flag to tell what should be the case and, of course, we would
// need conditions in all these macros.
#define	adc_start_refon	do { \
				_BIC (ADC12CTL0, ADC_FLG_ENC); \
				_BIS (ADC12CTL0, ADC12ON + ADC_FLG_REFON); \
				_BIS (ADC12CTL0, ADC12SC + ADC_FLG_ENC); \
			} while (0)

// Anything needed to keep it happy (like skipping samples on eCOG)
#define	adc_advance	CNOP

/*
 * DAC configuration for direct voltage setup
 */
#define dac_config_write(p,v,r)	do { \
				  if ((p) == 0) { \
				    _BIC (DAC12_0CTL,DAC12ENC); \
				    DAC12_0CTL = DAC12SREF_0 + \
					((r) ? 0 : DAC12IR) + \
					DAC12AMP_5; \
				    DAC12_0DAT = (v); \
				  } else { \
				    _BIC (DAC12_1CTL,DAC12ENC); \
				    DAC12_1CTL = DAC12SREF_0 + \
					((r) ? 0 : DAC12IR) + \
					DAC12AMP_5; \
				    DAC12_1DAT = (v); \
				  } \
				} while (0)

// DAC off sets voltage to zero
#define	DAC_CNF_OFF	DAC12AMP_1

// ============================================================================

#define	RSSI_MIN	0x0000	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x0fff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

// ============================================================================
// Watchdog ===================================================================
// ============================================================================

#define	WATCHDOG_STOP		WDTCTL = WDTPW + WDTHOLD
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

#define	WATCHDOG_CLEAR		WATCHDOG_START
#define	WATCHDOG_RESUME		WATCHDOG_START

// ============================================================================

#define	watchdog_start()	WATCHDOG_START
#define	watchdog_stop()		WATCHDOG_STOP
#define	watchdog_clear()	WATCHDOG_CLEAR

// ============================================================================

//#define	sti	_EINT ()
//#define	cli	_DINT ()
// Formally, nops are needed around these, only sometimes, not a big deal
// to make them always safe
#define	sti	do { __asm__ ("nop\n\t"); _EINT (); } while (0)
#define	cli	do { _DINT (); __asm__ ("nop\n\t"); } while (0)

#ifndef	LOW_POWER_SLEEP_MODE
#define	LOW_POWER_SLEEP_MODE	LPM3_bits
#endif

#define	__SLEEP	do { \
			if (__pi_systat.pdmode) { \
				cli; \
				if (__pi_systat.evntpn) { \
					sti; \
				} else { \
					_BIS_SR (LOW_POWER_SLEEP_MODE + GIE); \
				} \
			} else { \
				cli; \
				if (__pi_systat.evntpn) { \
					sti; \
				} else { \
					_BIS_SR (LPM0_bits + GIE); \
				} \
			} \
			__pi_systat.evntpn = 0; \
		} while (0)

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
#define	RISE_N_SHINE	do { __pi_systat.evntpn = 1; } while (0)
#define	RTNI		do { \
				if (__pi_systat.evntpn) \
					_BIC_SR_IRQ (LPM4_bits); \
				return; \
			} while (0)
#if 0
//
// DEAD CODE
//
// This is how it used to look
//

#define	RISE_N_SHINE	do { \
				__pi_systat.evntpn = 1; \
				_BIC_SR_IRQ (LPM4_bits); \
			} while (0)
#define	RTNI		return

#endif	/* DEAD CODE */


#if LEDS_DRIVER
#include "leds.h"
#endif

// Seconds clock
extern	lword		__pi_nseconds;

// ============================================================================
#if SECOND_CLOCK_RATE != 1
// We need to adjust the second counter [231028]
lword seconds ();
#else
#define	seconds()	__pi_nseconds
#endif
// ============================================================================

#define	setseconds(a)	(__pi_nseconds = (lword) (a));

// Power status
#define	powermode()	(__pi_systat.pdmode)

#endif
