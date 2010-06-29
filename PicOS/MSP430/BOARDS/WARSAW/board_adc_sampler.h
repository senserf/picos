#ifndef	__pg_board_adc_sampler_h
#define __pg_board_adc_sampler_h

/*
 * Seven channels sampled in sequence P6.0, P.2-P.7
 */

#define	ADCS_REFERENCE		SREF_AVCC_AVSS	// from 0 to Vcc

#define	ADCS_TA_FREQUENCY	500		// Hertz
#define	ADCS_TA_SOURCE		TASSEL_SMCLK
#define	ADCS_TA_DIVIDER		0
#define	ADCS_TA_PERIOD		((word)(CRYSTAL2_RATE / (ADCS_TA_FREQUENCY*8)))

#define	ADCS_AC_CLOCK_SOURCE	ADC12SSEL_SMCLK
#define	ADCS_AC_CLOCK_DIVIDER	ADC12DIV_7
#define	ADCS_AC_SAMPLE_CYCLES	SHT0_DIV64 + SHT1_DIV64

#define	ADCS_AC_TRIGGER		SHS_TACCR1	// Timer A output 1

#define	ADCS_INT_BIT		(1 << 7)	// On last sample

// The channels are sampled cyclically in a round robin fashion with an
// interrupt being triggered on the last one.

#define	adcs_init_regs 	do { \
	_BIC (P6DIR, 0xfd); \
	_BIS (P6SEL, 0xfd); \
	_BIC (ADC12CTL0, ENC); \
	ADC12CTL1 = ADCS_AC_TRIGGER + SHP + ADCS_AC_CLOCK_DIVIDER + \
		ADCS_AC_CLOCK_SOURCE + CONSEQ_SEQUENCE; \
	ADC12CTL0 = ADCS_AC_SAMPLE_CYCLES; \
	ADC12MCTL0 = ADCS_REFERENCE + 0; \
	ADC12MCTL1 = ADCS_REFERENCE + 1; \
	ADC12MCTL2 = ADCS_REFERENCE + 2; \
	ADC12MCTL3 = ADCS_REFERENCE + 3; \
	ADC12MCTL4 = ADCS_REFERENCE + 4; \
	ADC12MCTL5 = ADCS_REFERENCE + 5; \
	ADC12MCTL6 = ADCS_REFERENCE + 6; \
	ADC12MCTL7 = ADCS_REFERENCE + 7 + EOS; \
	TACTL = ADCS_TA_SOURCE + ADCS_TA_DIVIDER + TACLR; \
} while (0)

#define	adcs_start_sys	do { \
	ADC12IFG = 0; \
	_BIS (ADC12CTL0, ADC12ON); \
	adcs_reenable; \
	ADC12IE = ADCS_INT_BIT; \
	TACCR0 = ADCS_TA_PERIOD; \
	TACCR1 = ADCS_TA_PERIOD-1; \
	TACCTL0 = 0; \
	TACCTL1 = OUTMOD_SET_RESET; \
	TACCTL2 = 0; \
	_BIS (TACTL, MC_UPTO_CCR0 + TACLR); \
} while (0)

#define	adcs_reenable	_BIS (ADC12CTL0, ENC)

#define	adcs_stop_sys	do { \
	TACCTL1 = 0; \
	_BIC (TACTL, MC_3); \
	ADC12IE = 0; \
	_BIC (ADC12CTL0, ENC); \
	_BIC (ADC12CTL0, ADC12ON); \
	ADC12IFG = 0; \
} while (0)

#define	adcs_sample(b)	do { \
	(b) [0] = ADC12MEM0; \
	(b) [1] = ADC12MEM1; \
	(b) [2] = ADC12MEM2; \
	(b) [3] = ADC12MEM3; \
	(b) [4] = ADC12MEM4; \
	(b) [5] = ADC12MEM5; \
	(b) [6] = ADC12MEM6; \
	(b) [7] = ADC12MEM7; \
} while (0)

#define	adcs_clear_sample()	do { \
		ADC12MEM0; \
		ADC12MEM1; \
		ADC12MEM2; \
		ADC12MEM3; \
		ADC12MEM4; \
		ADC12MEM5; \
		ADC12MEM6; \
		ADC12MEM7; \
	} while (0);

#define	ADCS_SAMPLE_LENGTH	8	// In words

#endif
