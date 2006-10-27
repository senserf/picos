#ifndef	__pg_pins_sys_h
#define	__pg_pins_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

#define	PIN_DEF(p,n)	{ p ## IN_ - P1IN_ , n }
#define PIN_RESERVED	{ 0xff, 0 }

/*
 * ADC configuration for polled sample collection
 */
#define	adc_config_read(p,r)	do { \
				  _BIC (ADC12CTL0, ENC); \
				  _BIC (P6DIR, 1 << (p)); \
				  _BIS (P6SEL, 1 << (p)); \
				  ADC12CTL1 = ADC12DIV_7 + ADC12SSEL_3; \
				  ADC12MCTL0 = EOS + \
				  ((r) > 1 ? SREF_AVCC_AVSS : SREF_VREF_AVSS) +\
				    (p); \
				  ADC12CTL0 = ((r) ? REF2_5V : 0) + \
				    REFON + ADC12ON; \
				} while (0)

#define	adc_wait	do { \
				while (ADC12CTL1 & ADC12BUSY); \
				adc_disable; \
			} while (0)

#define	adc_inuse	(ADC12CTL0 & ADC12ON)
#define	adc_value	ADC12MEM0
#define	adc_rcvmode	((ADC12CTL1 & ADC12DIV_1) == 0)
#define	adc_disable	do { \
				_BIC (ADC12CTL0, ENC); \
				_BIC (ADC12CTL0, ADC12ON); \
			} while (0)

#define	adc_start	do { \
				_BIC (ADC12CTL0, ENC); \
				_BIS (ADC12CTL0, ADC12ON); \
				_BIS (ADC12CTL0, ADC12SC + ENC); \
			} while (0)

#define	adc_stop	_BIC (ADC12CTL0, ADC12SC)

#define	RSSI_MIN	0x0000	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x0fff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

/*
 * DAC configuration for immediate voltage setup
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
					
/* ========================================================================== */
/*                                 V E R S A 2                                */
/* ========================================================================== */
#if TARGET_BOARD == BOARD_VERSA2

//+++ p2irq.c

#ifndef	VERSA2_TARGET_BOARD
#define	VERSA2_TARGET_BOARD		1
#endif

// Departures from default pre-initialization (0xFF for power savings)
#if VERSA2_TARGET_BOARD
#define	PIN_DEFAULT_P1DIR	0xF4
#else
#define	PIN_DEFAULT_P1DIR	0xF0
#endif
#define	PIN_DEFAULT_P2DIR	0x02
#define	PIN_DEFAULT_P4DIR	0xF0
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P6DIR	0x00

#define	PINS_BOARD_FOUND		1
#define	MONITOR_PINS_SEND_INTERRUPTS	1

/*
 * Access to GP pins on the board:
 *
 * GP0-7 == P6.0 - P6.7
 *
 * CFG0 == P1.0
 * CFG1 == P1.1
 * CFG2 == P1.2 (2.2 in the target version)
 * CFG3 == P1.3 (used for reset)
 */

// Reset on CFG3 low (must be pulled up for normal operation)
#define	VERSA2_RESET_KEY_PRESSED	((P1IN & 0x08) == 0)

#if VERSA2_TARGET_BOARD	
	// Stupid two versions of the Versa board !!
#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
	PIN_DEF (P1, 0),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P2, 2),	\
	PIN_RESERVED,		\
}

#else	/* VERSA2_TARGET_BOARD */

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
	PIN_DEF (P1, 0),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P1, 2),	\
	PIN_RESERVED,		\
}

#endif	/* VERSA2_TARGET_BOARD */

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC

/*
 * ADC12 used for RSSI collection: source on A0 == P6.0, single sample,
 * triggered by ADC12SC
 * 
 *	ADC12CTL0 =      REFON +	// Internal reference
 *                     REF2_5V +	// 2.5 V for reference
 *		       ADC12ON +	// ON
 *			   ENC +	// Enable conversion
 *
 *	ADC10CTL1 = ADC12DIV_6 +	// Clock (SMCLK) divided by 7
 *		   ADC12SSEL_3 +	// SMCLK
 *
 *      ADC12MCTL0 =       EOS +	// Last word
 *		        SREF_1 +	// Vref - Vss
 *                      INCH_0 +	// A0
 *
 * The total sample time is equal to 16 + 13 ACLK ticks, which roughly
 * translates into 0.9 ms. The shortest packet at 19,200 takes more than
 * twice that long, so we should be safe.
 */

#define	adc_config_rssi		do { \
					_BIC (ADC12CTL0, ENC); \
					_BIC (P6DIR, 1 << 0); \
					_BIS (P6SEL, 1 << 0); \
					ADC12CTL1 = ADC12DIV_6 + ADC12SSEL_3; \
					ADC12MCTL0 = EOS + SREF_1 + INCH_0; \
					ADC12CTL0 = REF2_5V + ADC12ON; \
				} while (0)

#define	pin_book_cnt	do { \
				_BIC (P6DIR, 0x02); \
				_BIC (P6SEL, 0x02); \
			} while (0);

#define	pin_release_cnt	do { } while (0)

#define	pin_book_not	do { \
				_BIC (P6DIR, 0x04); \
				_BIC (P6SEL, 0x04); \
			} while (0);

#define	pin_release_not	do { } while (0)

#define	pin_interrupt	(P2IFG & 0x18)
#define	pin_int_cnt	(P2IFG & 0x08)
#define	pin_int_not	(P2IFG & 0x10)
#define	pin_trigger_cnt	_BIS (P2IFG, 0x08)
#define	pin_trigger_not	_BIS (P2IFG, 0x10)
#define	pin_enable_cnt	_BIS (P2IE, 0x08)
#define	pin_enable_not	_BIS (P2IE, 0x10)
#define	pin_disable_cnt	_BIC (P2IE, 0x08)
#define	pin_disable_not	_BIC (P2IE, 0x10)
#define	pin_clrint	_BIC (P2IFG, 0x18)
#define	pin_clrint_cnt	_BIC (P2IFG, 0x08)
#define	pin_clrint_not	_BIC (P2IFG, 0x10)
#define	pin_value_cnt	(P2IN & 0x08)
#define	pin_value_not	(P2IN & 0x10)
#define	pin_edge_cnt	(P2IES & 0x08)
#define	pin_edge_not	(P2IES & 0x10)

#define	pin_setedge_cnt	do { \
				if ((pmon.stat & PMON_CNT_EDGE_UP)) \
					_BIC (P2IES, 0x08); \
				else \
					_BIS (P2IES, 0x08); \
			} while (0)

#define	pin_revedge_cnt	do { \
				if ((pmon.stat & PMON_CNT_EDGE_UP)) \
					_BIS (P2IES, 0x08); \
				else \
					_BIC (P2IES, 0x08); \
			} while (0)

#define	pin_setedge_not	do { \
				if ((pmon.stat & PMON_NOT_EDGE_UP)) \
					_BIC (P2IES, 0x10); \
				else \
					_BIS (P2IES, 0x10); \
			} while (0)

#define	pin_revedge_not	do { \
				if ((pmon.stat & PMON_NOT_EDGE_UP)) \
					_BIS (P2IES, 0x10); \
				else \
					_BIC (P2IES, 0x10); \
			} while (0)
/*
 * Note: IES == 0 means low-high. pin_vedge_... means that the signal is
 * pending, i.e., its value corresponds to the end of triggering transition.
 */
#define	pin_vedge_cnt	(pin_edge_cnt != pin_value_cnt)
#define	pin_vedge_not	(pin_edge_not != pin_value_not)

#endif 	/* BOARD_VERSA2 */

/* ========================================================================== */
/*                               G E N E S I S                                */
/* ========================================================================== */
#if TARGET_BOARD == BOARD_GENESIS

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80
#define	PIN_DEFAULT_P3DIR	0xCF
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P6DIR	0x00

#define	PINS_BOARD_FOUND		1
#define	MONITOR_PINS_SEND_INTERRUPTS	0
#define	GENESIS_RESET_KEY_PRESSED	((P6IN & 0x01) == 0)

#if LEDS_DRIVER

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_RESERVED,		\
	PIN_RESERVED,		\
	PIN_RESERVED,		\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
}

#else	/* LEDS_DRIVER */

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
}

#endif	/* LEDS_DRIVER */

#define	PIN_MAX			8	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC

#define	adc_config_rssi		adc_disable

#define	pin_book_cnt	do { \
				_BIC (P6DIR, 0x02); \
				_BIC (P6SEL, 0x02); \
			} while (0);

#define	pin_release_cnt	do { } while (0)

#define	pin_book_not	do { \
				_BIC (P6DIR, 0x04); \
				_BIC (P6SEL, 0x04); \
			} while (0);

#define	pin_release_not	do { } while (0)

#define	pin_trigger_cnt	do { } while (0)
#define	pin_trigger_not	do { } while (0)
#define	pin_clrint_cnt	do { } while (0)
#define	pin_clrint_not	do { } while (0)
#define	pin_enable_cnt	do { } while (0)
#define	pin_enable_not	do { } while (0)
#define	pin_disable_cnt	do { } while (0)
#define	pin_disable_not	do { } while (0)

#define	pin_value_cnt	(P6IN & 0x02)
#define	pin_value_not	(P6IN & 0x04)

#define	pin_setedge_cnt	do { } while (0)
#define	pin_setedge_not	do { } while (0)

#define	pin_vedge_cnt	(((pmon.stat & PMON_CNT_EDGE_UP) == 0) == \
				(pin_value_cnt == 0))
#define	pin_vedge_not	(((pmon.stat & PMON_NOT_EDGE_UP) == 0) == \
				(pin_value_not == 0))
#endif	/* BOARD_GENESIS */

/* ========================================================================== */
/*                               D M 2 1 0 0                                  */
/* ========================================================================== */
#if TARGET_BOARD == BOARD_DM2100

//+++ p2irq.c

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0xF0
#define	PIN_DEFAULT_P2DIR	0x22
#define	PIN_DEFAULT_P3DIR	0x00
#define	PIN_DEFAULT_P4DIR	0xF0
#define	PIN_DEFAULT_P5DIR	0xFC
#define	PIN_DEFAULT_P6DIR	0x00

#define	PINS_BOARD_FOUND		1
#define	MONITOR_PINS_SEND_INTERRUPTS	1

#define	PIN_LIST	{ 	\
	PIN_RESERVED,		\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
	PIN_DEF (P1, 0),	\
	PIN_DEF (P1, 1),	\
	PIN_DEF (P1, 2),	\
	PIN_DEF (P1, 3),	\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0	// No DAC
					// P6.0 is reserved for RSSI

#define	adc_config_rssi		do { \
					_BIC (ADC12CTL0, ENC); \
					_BIC (P6DIR, 1 << 0); \
					_BIS (P6SEL, 1 << 0); \
					ADC12CTL1 = ADC12DIV_6 + ADC12SSEL_3; \
					ADC12MCTL0 = EOS + SREF_1 + INCH_0; \
					ADC12CTL0 = REF2_5V + ADC12ON; \
				} while (0)

#define	pin_book_cnt	do { \
				_BIC (P6DIR, 0x02); \
				_BIC (P6SEL, 0x02); \
			} while (0);

#define	pin_release_cnt	do { } while (0)

#define	pin_book_not	do { \
				_BIC (P6DIR, 0x04); \
				_BIC (P6SEL, 0x04); \
			} while (0);

#define	pin_release_not	do { } while (0)

#define	pin_interrupt	(P2IFG & 0x18)
#define	pin_int_cnt	(P2IFG & 0x08)
#define	pin_int_not	(P2IFG & 0x10)
#define	pin_trigger_cnt	_BIS (P2IFG, 0x08)
#define	pin_trigger_not	_BIS (P2IFG, 0x10)
#define	pin_enable_cnt	_BIS (P2IE, 0x08)
#define	pin_enable_not	_BIS (P2IE, 0x10)
#define	pin_disable_cnt	_BIC (P2IE, 0x08)
#define	pin_disable_not	_BIC (P2IE, 0x10)
#define	pin_clrint	_BIC (P2IFG, 0x18)
#define	pin_clrint_cnt	_BIC (P2IFG, 0x08)
#define	pin_clrint_not	_BIC (P2IFG, 0x10)
#define	pin_value_cnt	(P2IN & 0x08)
#define	pin_value_not	(P2IN & 0x10)
#define	pin_edge_cnt	(P2IES & 0x08)
#define	pin_edge_not	(P2IES & 0x10)

#define	pin_setedge_cnt	do { \
				if ((pmon.stat & PMON_CNT_EDGE_UP)) \
					_BIC (P2IES, 0x08); \
				else \
					_BIS (P2IES, 0x08); \
			} while (0)

#define	pin_revedge_cnt	do { \
				if ((pmon.stat & PMON_CNT_EDGE_UP)) \
					_BIS (P2IES, 0x08); \
				else \
					_BIC (P2IES, 0x08); \
			} while (0)

#define	pin_setedge_not	do { \
				if ((pmon.stat & PMON_NOT_EDGE_UP)) \
					_BIC (P2IES, 0x10); \
				else \
					_BIS (P2IES, 0x10); \
			} while (0)

#define	pin_revedge_not	do { \
				if ((pmon.stat & PMON_NOT_EDGE_UP)) \
					_BIS (P2IES, 0x10); \
				else \
					_BIC (P2IES, 0x10); \
			} while (0)

#define	pin_vedge_cnt	(pin_edge_cnt != pin_value_cnt)
#define	pin_vedge_not	(pin_edge_not != pin_value_not)

#endif	/* BOARD_DM2100 */

/* ========================================================================== */
/*                        S F U    P R O T O T Y P E                          */
/* ========================================================================== */
#if TARGET_BOARD == BOARD_SFU_PROTOTYPE

// Departures from default pre-initialization
#define	PIN_DEFAULT_P1DIR	0x80
#define	PIN_DEFAULT_P3DIR	0xCF
#define	PIN_DEFAULT_P5DIR	0xE0
#define	PIN_DEFAULT_P6DIR	0x00

#define	PINS_BOARD_FOUND		1

#define	PIN_LIST	{ 	\
				\
	PIN_DEF (P6, 0),	\
	PIN_DEF (P6, 1),	\
	PIN_DEF (P6, 2),	\
	PIN_DEF (P6, 3),	\
	PIN_DEF (P6, 4),	\
	PIN_DEF (P6, 5),	\
	PIN_DEF (P6, 6),	\
	PIN_DEF (P6, 7),	\
				\
	PIN_DEF (P2, 0),	\
	PIN_DEF (P2, 1),	\
	PIN_DEF (P2, 2),	\
	PIN_DEF (P2, 3),	\
	PIN_DEF (P2, 4),	\
	PIN_DEF (P2, 5),	\
	PIN_DEF (P2, 6),	\
	PIN_DEF (P2, 7),	\
				\
	PIN_DEF (P4, 0),	\
	PIN_DEF (P4, 1),	\
	PIN_DEF (P4, 2),	\
	PIN_DEF (P4, 3),	\
	PIN_DEF (P4, 4),	\
	PIN_DEF (P4, 5),	\
	PIN_DEF (P4, 6),	\
	PIN_DEF (P4, 7),	\
				\
	PIN_DEF (P5, 0),	\
	PIN_DEF (P5, 1),	\
	PIN_DEF (P5, 2),	\
	PIN_DEF (P5, 3),	\
	PIN_DEF (P5, 4),	\
	PIN_DEF (P5, 5),	\
	PIN_DEF (P5, 6),	\
	PIN_DEF (P5, 7),	\
}

#define	PIN_MAX			32	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7

#define	adc_config_rssi		adc_disable

#endif	/* BOARD_SFU_PROTOTYPE */
/* ========================================================================== */
/* ========================================================================== */
/* ========================================================================== */

#ifdef	PINS_BOARD_FOUND

typedef struct {
	byte poff, pnum;
} pind_t;


#define	PSEL_off	(P3SEL_ - P3IN_)
#define	PDIR_off	(P1DIR_ - P1IN_)
#define	POUT_off	(P1OUT_ - P1IN_)

/*
 * GP pin operations
 */
bool zz_pin_available (word);
bool zz_pin_adc_available (word);
word zz_pin_ivalue (word);
word zz_pin_ovalue (word);
bool zz_pin_adc (word);
bool zz_pin_output (word);
void zz_pin_set (word);
void zz_pin_clear (word);
void zz_pin_set_input (word);
void zz_pin_set_output (word);
void zz_pin_set_adc (word);

#if PIN_DAC_PINS != 0

bool zz_pin_dac_available (word);
bool zz_pin_dac (word);
void zz_clear_dac (word);
void zz_set_dac (word);
void zz_write_dac (word, word, word);

#else

#define	zz_pin_dac_available(a)	NO
#define	zz_pin_dac(a)		NO
#define	zz_clear_dac(a)		CNOP
#define	zz_set_dac(a)		CNOP
#define	zz_write_dac(a,b,c)	CNOP

#endif	/* PIN_DAC_PINS */

#else	/* PINS_BOARD_FOUND */

#define	adc_config_rssi		adc_disable


#endif	/* PINS_BOARD_FOUND */

#endif
