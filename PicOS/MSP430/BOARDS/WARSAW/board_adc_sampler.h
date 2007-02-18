#ifndef	__pg_board_adc_sampler_h
#define __pg_board_adc_sampler_h

/*
 * Seven channels sampled in sequence P6.0, P.2-P.7
 */

#define	ADCS_REFERENCE		SREF_AVCC_AVSS	// from 0 to Vcc

#if 0
#define	ADCS_AC_CLOCK_SOURCE	ADC12SSEL_ACLK
#define	ADCS_AC_CLOCK_DIVIDER	ADC12DIV_0
#define	ADCS_AC_SAMPLE_CYCLES	SHT0_DIV4 + SHT1_DIV4
#endif

#define	ADCS_AC_CLOCK_SOURCE	ADC12SSEL_SMCLK
#define	ADCS_AC_CLOCK_DIVIDER	ADC12DIV_7
#define	ADCS_AC_SAMPLE_CYCLES	SHT0_DIV192 + SHT1_DIV192

#define	ADCS_INT_BIT		(1 << 6)	// On last sample

// The channels are sampled cyclically in a round robin fashion with an
// interrupt being triggered on the last one.

#define	adcs_init_regs 	do { \
	_BIC (P6DIR, 0xfd); \
	_BIS (P6SEL, 0xfd); \
	_BIC (ADC12CTL0, ENC); \
	ADC12CTL1 = SHS_0 + SHP + ADCS_AC_CLOCK_DIVIDER + \
		ADCS_AC_CLOCK_SOURCE + CONSEQ_REPEAT_SEQUENCE; \
	ADC12CTL0 = ADCS_AC_SAMPLE_CYCLES + MSC; \
	ADC12MCTL0 = ADCS_REFERENCE + 0; \
	ADC12MCTL1 = ADCS_REFERENCE + 1; \
	ADC12MCTL2 = ADCS_REFERENCE + 2; \
	ADC12MCTL3 = ADCS_REFERENCE + 3; \
	ADC12MCTL4 = ADCS_REFERENCE + 4; \
	ADC12MCTL5 = ADCS_REFERENCE + 5; \
	ADC12MCTL6 = ADCS_REFERENCE + 6; \
	ADC12MCTL7 = ADCS_REFERENCE + 7 + EOS; \
} while (0)

#define	adcs_start_sys	do { \
	ADC12IFG = 0; \
	_BIS (ADC12CTL0, ADC12ON); \
	_BIS (ADC12CTL0, ENC + ADC12SC); \
	ADC12IE = ADCS_INT_BIT; \
} while (0)

#define	adcs_stop_sys	do { \
	ADC12IE = 0; \
	_BIC (ADC12CTL0, ENC); \
	_BIS (ADC12CTL0, ADC12ON); \
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

#define	ADCS_SAMPLE_LENGTH	8	// In words

#endif
