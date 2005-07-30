#ifndef	__pg_dm2100_sys_h
#define	__pg_dm2100_sys_h	1

//+++ "dm2100irq.c" "p1irq.c"

/* ================================================================== */

/*
 * Pin assignment (see IM2100):
 *
 *  TR1000                      MSP430Fxxx
 * ===========================================
 *  RXDATA			CCI0B (P2.2)		in
 *  TXMOD			OUT0 (P2.7)		out
 *  CNTRL0			P5.1			out
 *  CNTRL1			P5.0			out
 *  RSSI			A0 (P6.0)		analog in
 *  RSSI POWER-UP		P2.0			up on zero
 */


/*
 * Transmission rate: this is determined as the number of SLCK ticks per
 * physical bit time. Remember that SLCK runs at 4.5MHz. The math is simple:
 * one physical bit = 2/3 of real bit (excluding the preamble and checksum).
 */
#define	DM_RATE		700

#define	ini_regs	do { \
				_BIC (P2OUT, 0x85); \
				_BIS (P2DIR, 0x81); \
				_BIC (P2DIR, 0x04); \
				_BIS (P2SEL, 0x84); \
				_BIC (P5OUT, 0x03); \
				_BIS (P5DIR, 0x03); \
				_BIS (P4DIR, 0x0f); \
				_BIC (P6DIR, 0xfe); \
				_BIC (P1DIR, 0x0f); \
			} while (0)
/*
 * Access to GP pins on the board
 */
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

#define	pin_setinput(p)		do { \

#define	pin_value(p)		((p) < 8 ? (P6IN & (1 << (p))) : \
						(P1IN & (1 << ((p)-8))))

#define	pin_setint(p)		do { \
					_BIC (P1IES, 0x0f); \
					_BIS (P1IE, 1 << ((p)-8)); \
				} while (0)

#define	pin_clrint		_BIC (P1IE, 0x0f)


#define	chipcon_int		(P1IFG & 0x01)
#define	clear_chipcon_int	P1IFG &= ~0x01
/*
 * DM2100 signal operations. Timer's A Capture/Compare Block is used for signal
 * insertion/extraction.
 */
#define	c0up		_BIS (P5OUT, 0x02)
#define	c0down		_BIC (P5OUT, 0x02)
#define	c1up		_BIS (P5OUT, 0x01)
#define	c1down		_BIC (P5OUT, 0x01)
#define	rssi_on		_BIC (P2OUT, 0x01)
#define	rssi_off	_BIS (P2OUT, 0x01)

/*
 * Timer setting. We are using SMCLK running at 4.5 MHz.
 *
 *	TACTL = TASSEL_SMCLK | TACLR; 	// Timer A driven by SMCLK at the
 *                                      // CPU clock frequency
 *
 * The timer runs in the up mode setting up TAIFG whenever the count is
 * reached.
 *
 * For transmission, the timer triggers comparator interrupts whenever it
 * reaches the value in TACCR0. These interrupts strobe signal level flips.
 * For reception, signal transitions on CCI0B trigger capture interrupts,
 * with the time of the transition returned in TACCR0. Additionally, TACCR1
 * is used to trigger a timeout interrupt if the signal does not change for
 * a longish while.
 *
 */
#define	timer_init		do { TACTL = TASSEL_SMCLK | TACLR; } while (0)

#define DM_RATE_X1		DM_RATE
#define DM_RATE_X2		(DM_RATE + DM_RATE)
#define DM_RATE_X3		(DM_RATE + DM_RATE + DM_RATE)
#define DM_RATE_X4		(DM_RATE + DM_RATE + DM_RATE + DM_RATE)

#define	DM_RATE_DELTA		(DM_RATE / 2)

#define	DM_RATE_R1		(DM_RATE_X1 + DM_RATE_DELTA)
#define	DM_RATE_R2		(DM_RATE_X2 + DM_RATE_DELTA)
#define	DM_RATE_R3		(DM_RATE_X3 + DM_RATE_DELTA)
#define	DM_RATE_R4		(DM_RATE_X4 + DM_RATE_DELTA)
#define	DM_RATE_R5		(DM_RATE_X4 + DM_RATE_X1 + DM_RATE_DELTA)

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
#define	disable_xcv_timer	do { \
					TACCTL0 = 0; \
					TACCTL1 = 0; \
					_BIC (TACTL, MC_3); \
				} while (0)

/*
 * The reception is trickier. I tried capture on both slopes simultaneously,
 * but I couldn't get the signal right from SCCI (don't know why, perhaps the
 * input voltage from RXDATA tends to be flaky). So I have settled for 
 * switching the slope after every capture. This is done by starting with
 * CM_1 (meaning transition from low to high) and then switching to CM_2
 * (accomplished by adding CM_1 to what was there), then back to CM_1, and
 * so on. This way the actual perceived signal is implied from the last
 * transition and determined by the contents of CM.
 */
#define	enable_rcv_timer	do { \
					zzv_tmaux = 0; \
					TACCTL0 = CM_1 | CCIS_1 | SCS | CAP \
						| CCIE; \
					_BIS (TACTL, MC_2 | TACLR); \
				} while (0)

#define	enable_rcv_timeout	do { TACCTL1 = CCIE; } while (0)
#define	disable_rcv_timeout	do { TACCTL1 =    0; } while (0)

#define get_signal_params(t,v)	do { \
					(t) = ((word) TACCR0) - zzv_tmaux; \
					zzv_tmaux = (word) TACCR0; \
					if (((v) = (TACCTL0 & CM_1))) \
						TACCTL0 += CM_1; \
					else \
						TACCTL0 -= CM_1; \
				} while (0)

#define	set_rcv_timeout		do { TACCR1 = TACCR0 + SH5; } while (0)

#define	hard_lock		do { \
					_BIC (TACCTL0, CCIE); \
					_BIC (TACCTL1, CCIE); \
				} while (0)

#define	hard_drop		do { \
					if (zzv_status) { \
						_BIS (TACCTL0, CCIE); \
						if (zzv_status == HSTAT_RCV) \
							_BIS (TACCTL1, CCIE); \
					} \
				} while (0)

// The LED configuration corrsponds to the three LEDs on DM2100. A led is
// switched on by setting its output pin low. LED 0 isn't there, but we
// ignore this fact.
#define LEDI(n,s)	do { \
				if (s) \
					_BIC (P4OUT, 1 << (n)); \
				else \
					_BIS (P4OUT, 1 << (n)); \
			} while (0)

/*
 * ADC12 used for RSSI collection: source on A0 == P6.0, single sample,
 * triggered by ADC12SC
 * 
 *	ADC12CTL0 =     SHT0_2 +	// 16 ADC12CLOCKs sample holding time
 *		        SHT1_2 +	// 16 ADC12CLOCKs sample holding time
 *	                 REFON +	// Internal reference
 *                     REF2_5V +	// 2.5 V for reference
 *		       ADC12ON +	// ON
 *			   ENC +	// Enable conversion
 *
 *	ADC10CTL1 = ADC12DIV_0 +	// Clock (ACLK) divided by 1
 *		   ADC12SSEL_1 +	// ACLK
 *                         SHP +	// Pulse sampling
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
			ADC12CTL1 = ADC12DIV_0 + ADC12SSEL_1 + SHP; \
			ADC12MCTL0 = EOS + SREF_1 + INCH_0; \
			ADC12CTL0 = SHT0_2 + SHT1_2 + REF2_5V + ADC12ON; \
				} while (0)

/*
 * ADC configuration for polled sample collection
 */
#define	adc_config_read(p,r,t)	do { \
			_BIC (ADC12CTL0, ENC); \
			_BIC (P6DIR, 1 << (p)); \
			_BIS (P6SEL, 1 << (p)); \
			ADC12CTL1 = ADC12DIV_0 + ADC12SSEL_1 + SHP; \
			ADC12MCTL0 = EOS + SREF_1 + (p); \
			ADC12CTL0 = ((t) <=  4 ?  0 : \
				    ((t) <=  8 ?  1 : \
				    ((t) <= 16 ?  2 : \
				    ((t) <= 32 ?  3 : \
				     4 ))) ) + \
				    ((r) ? REF2_5V : 0) + \
				    REFON + ADC12ON; \
				} while (0)


#define	adc_wait	do { } while (ADC12CTL1 & ADC12BUSY)

#define adc_enable	ADC12CTL0 |=  (REFON + ENC)

#define	adc_disable	do { \
				_BIC (ADC12CTL0, ENC); \
				_BIC (ADC12CTL0, REFON); \
			} while (0)

#define	adc_inuse	(ADC12CTL0 & REFON)
#define	adc_start	_BIS (ADC12CTL0, ADC12SC)
#define	adc_stop	do { } while (0)
#define	adc_value	ADC12MEM0

#define	RSSI_MIN	0x0000	// Minimum and maximum RSSI values (for scaling)
#define	RSSI_MAX	0x0fff
#define	RSSI_SHF	4	// Shift bits to fit into a (unsigned) byte

#endif
