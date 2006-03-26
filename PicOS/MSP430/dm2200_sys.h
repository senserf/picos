#ifndef	__pg_dm2200_sys_h
#define	__pg_dm2200_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "dm2200irq.c" "p2irq.c"

/* ================================================================== */

/*
 * Pin assignment
 *
 *  TR8100                      MSP430Fxxx
 * ===========================================
 *  RXDATA			P2.2 (P2.5)	        in
 *  RXDCLK			P2.6			in
 *  CFG                         P4.0                    out
 *  TXMOD			OUT0 (P2.7)		out (no change)
 *  CFGDAT (CNTRL1)		P5.0			in/out (no change)
 *  CFGCLK (CNTRL0)		P5.1			out (no change)
 *  RSSI			A0 (P6.0)		analog in (no change)
 *  RSSI POWER-UP		P2.0			up on 1 (no change)
 *
 */

/*
 * CFG set high and then can be set low - this select the serial mode which
 * remains active until power down. In serial mode, CFG up starts clocking
 * sequence for serial data transfer.
 */

/*
 * Transmission rate: this is determined as the number of SLCK ticks per
 * physical bit time. Remember that SLCK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 */
#define	BIT_RATE	9600

#define	ini_regs	do { \
				_BIC (P4OUT, 0x01); \
				_BIS (P4DIR, 0x01); \
				_BIC (P6DIR, 0xfe); \
				_BIC (P5OUT, 0x03); \
				_BIS (P5DIR, 0x03); \
				_BIC (P2OUT, 0xff); \
				_BIC (P2DIR, 0x7C); \
				_BIS (P2DIR, 0x81); \
				_BIS (P2SEL, 0x80); \
				_BIC (P2IES, 0x58); \
				_BIC (P1DIR, 0x07); \
			} while (0)
/*
 * Access to GP pins on the board:
 *
 * GP0-7 == P6.0 - P6.7
 *
 * CFG0 == P1.0
 * CFG1 == P1.1
 * CFG2 == P1.2 (will be 2.2 in the target version)
 * CFG3 == P1.3 (used for reset)
 */

#define	VERSA2_RESET_KEY_PRESSED	((P1IN & 0x08) == 0)

#if 0	/* For the target */

#define	pin_sethigh(p)		do { \
					if ((p) < 8) { \
						_BIS (P6DIR, 1 << (p)); \
						_BIS (P6OUT, 1 << (p)); \
					} else if ((p) != 10) { \
						_BIS (P1DIR, 1 << ((p)-8)); \
						_BIS (P1OUT, 1 << ((p)-8)); \
					} else { \
						_BIS (P2DIR, 0x04); \
						_BIS (P2OUT, 0x04); \
					} \
				} while (0)

#define	pin_setlow(p)		do { \
					if ((p) < 8) { \
						_BIS (P6DIR, 1 << (p)); \
						_BIC (P6OUT, 1 << (p)); \
					} else if ((p) != 11) { \
						_BIS (P1DIR, 1 << ((p)-8)); \
						_BIC (P1OUT, 1 << ((p)-8)); \
					} else { \
						_BIS (P2DIR, 0x04); \
						_BIC (P2OUT, 0x04); \
					} \
				} while (0)

static inline pin_value (word p) {

	if (p < 8) {
		_BIC (P6DIR, 1 << p);
		return (P6IN & (1 << p));
	}
	if (p != 10) {
		_BIC (P1DIR, 1 << (p - 8));
		return (P1IN & (1 << (p - 8)));
	}
	_BIC (P2DIR, 0x04);
	return (P2IN & 0x04);
};

#define	rcv_sig_high	(P2IN & 0x20)

#else	/* For the temporary board with wrong RXDATA */

#define	pin_sethigh(p)		do { \
					if ((p) < 8) { \
						_BIS (P6DIR, 1 << (p)); \
						_BIS (P6OUT, 1 << (p)); \
					} else { \
						_BIS (P1DIR, 1 << ((p)-8)); \
						_BIS (P1OUT, 1 << ((p)-8)); \
					} \
				} while (0)

#define	pin_setlow(p)		do { \
					if ((p) < 8) { \
						_BIS (P6DIR, 1 << (p)); \
						_BIC (P6OUT, 1 << (p)); \
					} else { \
						_BIS (P1DIR, 1 << ((p)-8)); \
						_BIC (P1OUT, 1 << ((p)-8)); \
					} \
				} while (0)

static inline pin_value (word p) {

	if (p < 8) {
		_BIC (P6DIR, 1 << p);
		return (P6IN & (1 << p));
	}
	_BIC (P1DIR, 1 << (p - 8));
	return (P1IN & (1 << (p - 8)));
};

#define	rcv_sig_high	(P2IN & 0x04)

#endif	/* TARGET */

#define	pin_setint(p)		do { \
					_BIC (P2IES, 0x18); \
					_BIS (P2IE, 1 << ((p)+2)); \
					_BIC (P2IFG, 0x18); \
				} while (0)

#define	pin_clrint		do { \
					_BIC (P2IE, 0x18); \
					_BIC (P2IFG, 0x18); \
				} while (0)

#define	pin_interrupt		(P2IFG & 0x18)

/*
 * DM2200 signal operations. Timer's A Capture/Compare Block is used for signal
 * insertion (transmission).
 */

#define	cfg_up		_BIS (P4OUT, 0x01)
#define	cfg_down	_BIC (P4OUT, 0x01)

#define	ser_up		_BIS (P5OUT, 0x01)
#define	ser_down	_BIC (P5OUT, 0x01)
#define	ser_out		_BIS (P5DIR, 0x01)
#define	ser_in		_BIC (P5DIR, 0x01)
#define	ser_data	(P5IN & 0x1)

#define	ser_clk_up	_BIS (P5OUT, 0x02)
#define	ser_clk_down	_BIC (P5OUT, 0x02)

#define	rssi_on		_BIS (P2OUT, 0x01)
#define	rssi_off	_BIC (P2OUT, 0x01)

#define	rcv_interrupt	(P2IFG & 0x40)
#define	rcv_clrint	_BIC (P2IFG, 0x40)
#define	rcv_enable	_BIS (P2IE, 0x40)
#define	rcv_disable	_BIC (P2IE, 0x40)

/*
 * The timer runs in the up mode setting up TAIFG whenever the count is
 * reached.
 *
 * For transmission, the timer triggers comparator interrupts whenever it
 * reaches the value in TACCR0. These interrupts strobe signal level flips.
 *
 * For reception, we use the data clock extracted by TR8100.
 *
 */

#if CRYSTAL2_RATE

// Use XTL2 (should be high speed)

#define	TAFREQ		CRYSTAL2_RATE
#define	TASSEL_RADIO	TASSEL_SMCLK

#else	/* No XTL2 */

// If XTL1 is high-speed, use it; otherwise, use SMCLK == MCLK == DCO

#if CRYSTAL_RATE != 32768
#define	TASSEL_RADIO	TASSEL_ACLK
#define	TAFREQ		CRYSTAL_RATE
#else
#define	TASSEL_RADIO	TASSEL_SMCLK
#define	TAFREQ		4700000
#endif

#endif	/* CRYSTAL2_RATE */

#define	DM_RATE			(TAFREQ/BIT_RATE)

#define	timer_init		do { TACTL = TASSEL_RADIO | TACLR; } while (0)

#define DM_RATE_X1		DM_RATE
#define DM_RATE_X2		(DM_RATE + DM_RATE)
#define DM_RATE_X3		(DM_RATE + DM_RATE + DM_RATE)
#define DM_RATE_X4		(DM_RATE + DM_RATE + DM_RATE + DM_RATE)

#define	enable_xmt_timer	do { \
					TACCR0 = DM_RATE_X1; \
					TACCTL1 = 0; \
					TACCTL0 = CCIE | OUT; \
					_BIS (TACTL, MC_2 | TACLR); \
				} while (0)

#define	set_signal_length(v)	TACCR0 = zzv_tmaux + (v)

#define	toggle_signal		do { \
					TACCTL0 ^= OUT; \
					zzv_tmaux = TACCR0; \
				} while (0)

#define	current_signal_level	(TACCTL0 & OUT)
					
/*
 * This stops the transmitter timer (and interrupts)
 */
#define	disable_xmt_timer	do { \
					TACCTL0 = 0; \
					TACCTL1 = 0; \
					_BIC (TACTL, MC_3); \
				} while (0)

// Needed by xcvcommon
#define	disable_xcv_timer	rcv_disable

#define	hard_lock		do { \
					_BIC (TACCTL0, CCIE); \
					_BIC (TACCTL1, CCIE); \
					rcv_disable; \
				} while (0)

#define	hard_drop		do { \
					if (zzv_status == HSTAT_RCV) \
						rcv_enable; \
					else if (zzv_status == HSTAT_XMT) \
						_BIS (TACCTL0, CCIE); \
				} while (0)
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

/*
 * ADC configuration for polled sample collection
 */
#define	adc_config_read(p,r)	do { \
					_BIC (ADC12CTL0, ENC); \
					_BIC (P6DIR, 1 << (p)); \
					_BIS (P6SEL, 1 << (p)); \
					ADC12CTL1 = ADC12DIV_7 + ADC12SSEL_3; \
					ADC12MCTL0 = EOS + SREF_1 + (p); \
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

#endif
