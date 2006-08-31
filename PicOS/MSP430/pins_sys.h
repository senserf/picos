#ifndef	__pg_pins_sys_h
#define	__pg_pins_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

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

#define	adc_inuse	(ADC12CTL0 & REFON)
#define	adc_value	ADC12MEM0
#define	adc_rcvmode	((ADC12CTL1 & ADC12DIV_1) == 0)
#define	adc_disable	 _BIC (ADC12CTL0, ENC + REFON)

#define	adc_start	do { \
				adc_disable; \
				_BIS (ADC12CTL0, ADC12SC + REFON + ENC); \
			} while (0)

#define	adc_stop	_BIC (ADC12CTL0, ADC12SC)

#define	RSSI_MIN	0x0000	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x0fff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

// =========================================================== //

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
#define	PIN_DEFAULT_P1DIR	0xF4
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
				\
	{ 0xff, 0 },		\
				\
	{ P6IN_ - P1IN_, 1 },	\
	{ P6IN_ - P1IN_, 2 },	\
				\
	{ P6IN_ - P1IN_, 3 },	\
	{ P6IN_ - P1IN_, 4 },	\
	{ P6IN_ - P1IN_, 5 },	\
	{ P6IN_ - P1IN_, 6 },	\
	{ P6IN_ - P1IN_, 7 },	\
				\
	{ P1IN_ - P1IN_, 0 },	\
	{ P1IN_ - P1IN_, 1 },	\
	{ P2IN_ - P1IN_, 2 },	\
	{ 0xff, 0}		\
}

#else	/* VERSA2_TARGET_BOARD */

#define	PIN_LIST	{ 	\
				\
	{ 0xff, 0 },		\
				\
	{ P6IN_ - P1IN_, 1 },	\
	{ P6IN_ - P1IN_, 2 },	\
				\
	{ P6IN_ - P1IN_, 3 },	\
	{ P6IN_ - P1IN_, 4 },	\
	{ P6IN_ - P1IN_, 5 },	\
	{ P6IN_ - P1IN_, 6 },	\
	{ P6IN_ - P1IN_, 7 },	\
				\
	{ P1IN_ - P1IN_, 0 },	\
	{ P1IN_ - P1IN_, 1 },	\
	{ P1IN_ - P1IN_, 2 },	\
	{ 0xff, 0}		\
}

#endif	/* VERSA2_TARGET_BOARD */

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins

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

// =========================================================== //

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
				\
	{ 0xff, 0 },		\
	{ P6IN_ - P1IN_, 1 },	\
	{ P6IN_ - P1IN_, 2 },	\
	{ 0xff, 0 },		\
	{ 0xff, 0 },		\
	{ 0xff, 0 },		\
	{ P6IN_ - P1IN_, 6 },	\
	{ P6IN_ - P1IN_, 7 },	\
}

#else	/* LEDS_DRIVER */

#define	PIN_LIST	{ 	\
				\
	{ 0xff, 0 },		\
	{ P6IN_ - P1IN_, 1 },	\
	{ P6IN_ - P1IN_, 2 },	\
	{ P6IN_ - P1IN_, 3 },	\
	{ P6IN_ - P1IN_, 4 },	\
	{ P6IN_ - P1IN_, 5 },	\
	{ P6IN_ - P1IN_, 6 },	\
	{ P6IN_ - P1IN_, 7 },	\
}

#endif	/* LEDS_DRIVER */

#define	PIN_MAX			8	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins

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
				\
	{ 0xff, 0 },		\
				\
	{ P6IN_ - P1IN_, 1 },	\
	{ P6IN_ - P1IN_, 2 },	\
				\
	{ P6IN_ - P1IN_, 3 },	\
	{ P6IN_ - P1IN_, 4 },	\
	{ P6IN_ - P1IN_, 5 },	\
	{ P6IN_ - P1IN_, 6 },	\
	{ P6IN_ - P1IN_, 7 },	\
				\
	{ P1IN_ - P1IN_, 0 },	\
	{ P1IN_ - P1IN_, 1 },	\
	{ P1IN_ - P1IN_, 2 },	\
	{ P1IN_ - P1IN_, 3 }	\
}

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
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

// =========================================================== //
// =========================================================== //

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
word zz_pin_value (word);
bool zz_pin_analog (word);
bool zz_pin_output (word);
void zz_pin_set (word p);
void zz_pin_clear (word p);
void zz_pin_setinput (word p);
void zz_pin_setoutput (word p);
void zz_pin_setanalog (word p);

#define	pin_available(p)	zz_pin_available(p)
#define	pin_value(p)		zz_pin_value(p)
#define	pin_analog(p)		zz_pin_analog(p)
#define	pin_output(p)		zz_pin_output(p)
#define	pin_setvalue(p,v)	do { \
					if (v) \
						zz_pin_set (p); \
					else \
						zz_pin_clear (p); \
				} while (0)

#define	pin_setinput(p)		zz_pin_setinput (p)
#define	pin_setoutput(p)	zz_pin_setoutput (p)
#define	pin_setanalog(p)	zz_pin_setanalog (p)

#else	/* PINS_BOARD_FOUND */

#define	adc_config_rssi		do { } while (0)


#endif	/* PINS_BOARD_FOUND */

#endif
