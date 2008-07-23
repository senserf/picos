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
#define	adc_config_read(p,r,t)	do { \
				  _BIC (ADC12CTL0, ENC); \
				  _BIC (P6DIR, 1 << (p)); \
				  _BIS (P6SEL, 1 << (p)); \
				  ADC12CTL1 = ADC12DIV_7 + ADC12SSEL_0 + \
				  ((t) != 0 ? SHP : 0); \
				  ADC12MCTL0 = EOS + \
				  ((r) > 2 ? SREF_VEREF_AVSS : ((r) == 2 ? \
				    SREF_AVCC_AVSS : SREF_VREF_AVSS)) + (p); \
				  ADC12CTL0 = ADC12ON + \
				   ((r) == 1 ? REF2_5V : 0) + \
				   ((r) < 2 ? REFON : 0) + \
				   (((t) & 0xf) << 12) + (((t) & 0xf) << 8); \
				} while (0)

/*
 * Defines internal functions for pin monitor with interrupts:
 *
 *    This assumes that the pins are on P2
 *
 *    pic   == counter pin number, e.g., 1 for P2.1
 *    pin   == notifier pin
 */
#define PINS_MONITOR_INT(pic,pin) 				       	\
									\
	static int inline pin_interrupt () {				\
		return (P2IFG & ((1 << (pic)) | (1 << (pin))));		\
	}								\
									\
	static int inline pin_int_cnt () {				\
		return (P2IFG & (1 << (pic)));				\
	}								\
									\
	static int inline pin_int_not () {				\
		return (P2IFG & (1 << (pin)));				\
	}								\
									\
	static void inline pin_trigger_cnt () {				\
		_BIS (P2IFG, 1 << (pic));				\
	}								\
									\
	static void inline pin_trigger_not () {				\
		_BIS (P2IFG, 1 << (pin));				\
	}								\
									\
	static void inline pin_enable_cnt () {				\
		_BIS (P2IE, 1 << (pic));				\
	}								\
									\
	static void inline pin_enable_not () {				\
		_BIS (P2IE, 1 << (pin));				\
	}								\
									\
	static void inline pin_disable_cnt () {				\
		_BIC (P2IE, 1 << (pic));				\
	}								\
									\
	static void inline pin_disable_not () {				\
		_BIC (P2IE, 1 << (pin));				\
	}								\
									\
	static void inline pin_clrint () {				\
		_BIC (P2IFG, (1 << (pic)) | (1 << (pin)));		\
	}								\
									\
	static void inline pin_clrint_cnt () {				\
		_BIC (P2IFG, 1 << (pic));				\
	}								\
									\
	static void inline pin_clrint_not () {				\
		_BIC (P2IFG, 1 << (pin));				\
	}								\
									\
	static int inline pin_value_cnt () {				\
		return (P2IN & (1 << (pic)));				\
	}								\
									\
	static int inline pin_value_not () {				\
		return (P2IN & (1 << (pin)));				\
	}								\
									\
	static int inline pin_edge_cnt () {				\
		return (P2IES & (1 << (pic)));				\
	}								\
									\
	static int inline pin_edge_not () {				\
		return (P2IES & (1 << (pin)));				\
	}								\
									\
	static void inline pin_setedge_cnt () {				\
		if ((pmon.stat & PMON_CNT_EDGE_UP))			\
			_BIC (P2IES, 1 << (pic));			\
		else							\
			_BIS (P2IES, 1 << (pic));			\
	}								\
									\
	static void inline pin_revedge_cnt () {				\
		if ((pmon.stat & PMON_CNT_EDGE_UP))			\
			_BIS (P2IES, 1 << (pic));			\
		else							\
			_BIC (P2IES, 1 << (pic));			\
	}								\
									\
	static void inline pin_setedge_not () {				\
		if ((pmon.stat & PMON_NOT_EDGE_UP))			\
			_BIC (P2IES, 1 << (pin));			\
		else							\
			_BIS (P2IES, 1 << (pin));			\
	}								\
									\
	static void inline pin_revedge_not () {				\
		if ((pmon.stat & PMON_NOT_EDGE_UP))			\
			_BIS (P2IES, 1 << (pin));			\
		else							\
			_BIC (P2IES, 1 << (pin));			\
	}								\
									\
	static int inline pin_vedge_cnt () {				\
		return pin_edge_cnt () != pin_value_cnt ();		\
	}								\
									\
	static int inline pin_vedge_not () {				\
		return pin_edge_not () != pin_value_not ();		\
	}								\

/*
 * Defines internal functions for pin monitor without interrupts:
 *
 *    poc   == counter port, e.g., P2
 *    pic   == counter pin number, e.g., 1 for P2.1
 *    pon   == notifier port
 *    pin   == notifier pin
 */
#define PINS_MONITOR(poc,pic,pon,pin)			       		\
									\
	static void inline pin_trigger_cnt () {				\
	}								\
									\
	static void inline pin_trigger_not () {				\
	}								\
									\
	static void inline pin_enable_cnt () {				\
	}								\
									\
	static void inline pin_enable_not () {				\
	}								\
									\
	static void inline pin_disable_cnt () {				\
	}								\
									\
	static void inline pin_disable_not () {				\
	}								\
									\
	static void inline pin_clrint_cnt () {				\
	}								\
									\
	static void inline pin_clrint_not () {				\
	}								\
									\
	static int inline pin_value_cnt () {				\
		return (poc ## IN & (1 << (pic)));			\
	}								\
									\
	static int inline pin_value_not () {				\
		return (pon ## IN & (1 << (pin)));			\
	}								\
									\
	static void inline pin_setedge_cnt () {				\
	}								\
									\
	static void inline pin_setedge_not () {				\
	}								\
									\
	static int inline pin_vedge_cnt () {				\
		return (((pmon.stat & PMON_CNT_EDGE_UP) == 0) == 	\
				(pin_value_cnt () == 0));		\
	}								\
									\
	static int inline pin_vedge_not () {				\
		return (((pmon.stat & PMON_NOT_EDGE_UP) == 0) == 	\
				(pin_value_not () == 0));			\
	}								\

#include "board_pins.h"

#ifndef	PIN_MAX
#define	PIN_MAX				0
#endif

#ifndef	PIN_MAX_ANALOG
#define	PIN_MAX_ANALOG			0
#endif

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#ifndef	MONITOR_PINS_SEND_INTERRUPTS
#define	MONITOR_PINS_SEND_INTERRUPTS	0
#endif

#if	PIN_MAX 
//+++ pins_sys.c
#endif

#ifdef	PULSE_MONITOR
//+++ pins_sys.c
#endif

#if	MONITOR_PINS_SEND_INTERRUPTS
//+++ p2irq.c
REQUEST_EXTERNAL (p2irq);
#endif

#ifdef	PIN_ADC_RSSI

#define	adc_config_rssi		do { \
					_BIC (ADC12CTL0, ENC); \
					_BIC (P6DIR, 1 << PIN_ADC_RSSI); \
					_BIS (P6SEL, 1 << PIN_ADC_RSSI); \
					ADC12CTL1 = ADC12DIV_6 + ADC12SSEL_3; \
					ADC12MCTL0 = EOS + SREF_1 + INCH_0; \
					ADC12CTL0 = REF2_5V + ADC12ON + REFON; \
				} while (0)

#else	/* NO ADC RSSI */

#define	adc_config_rssi		adc_disable

#endif	/* PIN_ADC_RSSI */

// Result not available
#define	adc_busy	(ADC12CTL1 & ADC12BUSY)

// Wait for result
#define	adc_wait	do { } while (adc_busy)

// ADC is on
#define	adc_inuse	(ADC12CTL0 & ADC12ON)

// ADC reading
#define	adc_value	ADC12MEM0

// ADC operating for the RF receiver; this is heuristic, and perhaps
// not needed (used in pin_read.c to save a status bit that would have to be
// stored somewhere)
#define	adc_rcvmode	((ADC12CTL1 & ADC12DIV_1) == 0)

// Explicit end of sample indication
#define	adc_stop	_BIC (ADC12CTL0, ADC12SC)

// Off but possibly less than disable
#define	adc_off		_BIC (ADC12CTL0, ENC)

// Complete off, including REF voltage
#define	adc_disable	do { \
				adc_off; \
				_BIC (ADC12CTL0, ADC12ON + REFON); \
			} while (0)

// Start measurement
#define	adc_start	do { \
				_BIC (ADC12CTL0, ENC); \
				_BIS (ADC12CTL0, ADC12ON); \
				_BIS (ADC12CTL0, ADC12SC + ENC); \
			} while (0)

// Start measurement with RFON. This is a mess, but RFON takes current, so I
// want to make sure that it is always off after adc_disable. This requires
// whoever invokes adc_start to know. Otherwise, we would need a separate 
// in-memory flag to tell what should be the case and, of course, we would
// need conditions in all these macros.
#define	adc_start_refon	do { \
				_BIC (ADC12CTL0, ENC); \
				_BIS (ADC12CTL0, ADC12ON + REFON); \
				_BIS (ADC12CTL0, ADC12SC + ENC); \
			} while (0)

// Anything needed to keep it happy (like skipping samples on eCOG)
#define	adc_advance	CNOP

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

#if PIN_MAX

typedef struct {
	byte poff, pnum;
} pind_t;


#define	PSEL_off	(P3SEL_ - P3IN_)
#define	PDIR_off	(P1DIR_ - P1IN_)
#define	POUT_off	(P1OUT_ - P1IN_)

/*
 * GP pin operations
 */
Boolean zz_pin_available (word);
Boolean zz_pin_adc_available (word);
word zz_pin_ivalue (word);
word zz_pin_ovalue (word);
Boolean zz_pin_adc (word);
Boolean zz_pin_output (word);
void zz_pin_set (word);
void zz_pin_clear (word);
void zz_pin_set_input (word);
void zz_pin_set_output (word);
void zz_pin_set_adc (word);

#else	/* PIN_MAX == 0 */

#define zz_pin_available(a)		0
#define zz_pin_adc_available(a)		0
#define zz_pin_ivalue(a)		0
#define zz_pin_ovalue(a)		0
#define zz_pin_adc(a)			0
#define zz_pin_output(a)		0
#define zz_pin_set(a)			CNOP
#define zz_pin_clear(a)			CNOP
#define zz_pin_set_input(a)		CNOP
#define zz_pin_set_output(a)		CNOP
#define zz_pin_set_adc(a)		CNOP

#define	adc_config_rssi		adc_disable

#endif	/* PIN_MAX == 0 */

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#if PIN_DAC_PINS

Boolean zz_pin_dac_available (word);
Boolean zz_pin_dac (word);
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

#endif
