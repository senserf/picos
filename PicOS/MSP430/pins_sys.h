#ifndef	__pg_pins_sys_h
#define	__pg_pins_sys_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

// ============================================================================
// This is for the "legacy" (the so-called unified access) pin interface ======
// ============================================================================

#define	PIN_DEF(p,n)	{ p ## ORD__ , n }
#define PIN_RESERVED	{ 0xff, 0 }

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

#ifndef	PIN_MAX_ANALOG
#define	PIN_MAX_ANALOG			0
#endif

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#ifndef	MONITOR_PINS_SEND_INTERRUPTS
#define	MONITOR_PINS_SEND_INTERRUPTS	0
#endif

#ifdef	PULSE_MONITOR
//+++ pins_sys.c
#endif

#if	MONITOR_PINS_SEND_INTERRUPTS
//+++ p2irq.c
REQUEST_EXTERNAL (p2irq);
#endif

#ifdef PIN_LIST

//+++ pins_sys.c

typedef struct {
	byte poff, pnum;
} pind_t;


// Assumes that these offsets are the same for all ports
#define	PSEL_off	(P3SEL_ - P3IN_)
#define	PDIR_off	(P3DIR_ - P3IN_)
#define	POUT_off	(P3OUT_ - P3IN_)

/*
 * GP pin operations
 */
Boolean __pi_pin_available (word);
Boolean __pi_pin_adc_available (word);
word __pi_pin_ivalue (word);
word __pi_pin_ovalue (word);
Boolean __pi_pin_adc (word);
Boolean __pi_pin_output (word);
void __pi_pin_set (word);
void __pi_pin_clear (word);
void __pi_pin_set_input (word);
void __pi_pin_set_output (word);
void __pi_pin_set_adc (word);

#else	/* NO PIN_LIST */

#define __pi_pin_available(a)		0
#define __pi_pin_adc_available(a)	0
#define __pi_pin_ivalue(a)		0
#define __pi_pin_ovalue(a)		0
#define __pi_pin_adc(a)			0
#define __pi_pin_output(a)		0
#define __pi_pin_set(a)			CNOP
#define __pi_pin_clear(a)		CNOP
#define __pi_pin_set_input(a)		CNOP
#define __pi_pin_set_output(a)		CNOP
#define __pi_pin_set_adc(a)		CNOP

#define	adc_config_rssi		adc_disable

#endif	/* PIN_LIST or NO PIN_LIST */

#ifndef	PIN_DAC_PINS
#define	PIN_DAC_PINS			0
#endif

#if PIN_DAC_PINS

Boolean __pi_pin_dac_available (word);
Boolean __pi_pin_dac (word);
void __pi_clear_dac (word);
void __pi_set_dac (word);
void __pi_write_dac (word, word, word);

#else

#define	__pi_pin_dac_available(a)	NO
#define	__pi_pin_dac(a)			NO
#define	__pi_clear_dac(a)		CNOP
#define	__pi_set_dac(a)			CNOP
#define	__pi_write_dac(a,b,c)		CNOP

#endif	/* PIN_DAC_PINS */

// ============================================================================
// This is for the sensor/actuator-type pin interface =========================
// ============================================================================

#define	INPUT_PIN(p,n,e)	{ n, e, p ## ORD__ }
#define	OUTPUT_PIN(p,n,e)	{ n, e, p ## ORD__ }

#if defined(INPUT_PIN_LIST) || defined(OUTPUT_PIN_LIST)

typedef struct {

	word	pnum:3,		// Pin number (0-7)
		edge:1,		// Polarity 0: on==1, 1: on==0
		poff:12;	// Offset of the port address

} piniod_t;

#endif

// ============================================================================

#endif
