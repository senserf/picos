#ifndef	__pg_dm2200_sys_h
#define	__pg_dm2200_sys_h	1
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	FCC_TEST_MODE	0

//+++ "dm2200irq.c" "p2irq.c" "dm2200_pins.c"

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
 * Access to GP pins on the board:
 *
 * GP0-7 == P6.0 - P6.7
 *
 * CFG0 == P1.0
 * CFG1 == P1.1
 * CFG2 == P1.2 (will be 2.2 in the target version)
 * CFG3 == P1.3 (used for reset)
 */

// Set this to 1 to direct CFG2 to P2.2 and RXDATA to P2.5
#define	VERSA2_TARGET_BOARD		1

// Reset on CFG3 low (must be pulled up for normal operation)
#define	VERSA2_RESET_KEY_PRESSED	((P1IN & 0x08) == 0)
/*
 * Ignore other CFG pins for now (we leave them open for future requirements).
 * The available (to the application) pins are GP0-GP7. Two of them: GP1 and
 * GP2, are dedicated to COUNTER and NOTIFIER.
 */

/*
 * Transmission rate: this is determined as the number of SLCK ticks per
 * physical bit time. Remember that SLCK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 */
#define	BIT_RATE	9600

#define	PIN_MAX			12	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins

#define	VERSA2_PIN_MASK		(PIN_STDP_GP | PIN_CNTR_GP)

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

// FIXME: configure unused pins as output (the manual says this reduces power
// consumption)

#if FCC_TEST_MODE
#define	fcc_test_send		((P1IN & 0x01) != 0)
#endif

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
/*
 * Note: IES == 0 means low-high. pin_vedge_... means that the signal is
 * pending, i.e., its value corresponds to the end of triggering transition.
 */
#define	pin_vedge_cnt	(pin_edge_cnt != pin_value_cnt)
#define	pin_vedge_not	(pin_edge_not != pin_value_not)

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

#if VERSA2_TARGET_BOARD
#define	rcv_sig_high	(P2IN & 0x20)
#else
#define	rcv_sig_high	(P2IN & 0x04)
#endif
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
// FIXME: experiment with conversion timing
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
