// No need to put conditions here; included by arch.h to fix problems with
// symbolic constants that can be inconsistent across different mspgcc
// compiler versions

#if !defined(SREF_AVCC_AVSS) && defined(SREF_0)
#define	SREF_AVCC_AVSS  		SREF_0
#endif

#if !defined(SREF_VREF_AVSS) && defined(SREF_1)
#define	SREF_VREF_AVSS			SREF_1
#endif

#if !defined(SREF_VEREF_AVSS) && defined(SREF_2)
#define	SREF_VEREF_AVSS 		SREF_2
#endif

#if !defined(ADC12SSEL_ADC12OSC) && defined(ADC12SSEL_0)
#define	ADC12SSEL_ADC12OSC		ADC12SSEL_0
#endif

#if !defined(SSEL_SMCLK) && defined(SSEL1)
#define	SSEL_SMCLK			SSEL1
#endif

#if !defined(INCH_VCC2) && defined(ADC12INCH_11)
#define	INCH_VCC2			ADC12INCH_11
#endif

#if !defined(INCH_VCC2) && defined(INCH_11)
#define	INCH_VCC2			INCH_11
#endif

#if !defined(INCH_TEMP) && defined(ADC12INCH_10)
#define	INCH_TEMP			ADC12INCH_10
#endif

#if !defined(INCH_TEMP) && defined(INCH_10)
#define	INCH_TEMP			INCH_10
#endif

#if !defined(ADC12SSEL_ADC12OSC) && defined(ADC12SSEL_0)
#define	ADC12SSEL_ADC12OSC		ADC12SSEL_0
#endif

#if !defined(SELM_DCOCLK) && defined(SELM_0)
#define	SELM_DCOCLK			SELM_0
#endif

// ============================================================================

#if !defined(P1IN_) && defined(PAIN_)
#define	P1IN_				PAIN_
#endif

#if !defined(P1SEL_) && defined(PASEL_)
#define	P1SEL_				PASEL_
#endif

#if !defined(P1DIR_) && defined(PADIR_)
#define	P1DIR_				PADIR_
#endif

#if !defined(P1REN_) && defined(PAREN_)
#define	P1REN_				PAREN_
#endif

#if !defined(P1DS_) && defined(PADS_)
#define	P1DS_				PADS_
#endif

#if !defined(P1OUT_) && defined(PAOUT_)
#define	P1OUT_				PAOUT_
#endif

// ============================================================================

#if !defined(P2IN_) && defined(PAIN_)
#define	P2IN_				(PAIN_+1)
#endif

#if !defined(P2SEL_) && defined(PASEL_)
#define	P2SEL_				(PASEL_+1)
#endif

#if !defined(P2DIR_) && defined(PADIR_)
#define	P2DIR_				(PADIR_+1)
#endif

#if !defined(P2REN_) && defined(PAREN_)
#define	P2REN_				(PAREN_+1)
#endif

#if !defined(P2DS_) && defined(PADS_)
#define	P2DS_				(PADS_+1)
#endif

#if !defined(P2OUT_) && defined(PAOUT_)
#define	P2OUT_				(PAOUT_+1)
#endif

// ============================================================================

#if !defined(P3IN_) && defined(PBIN_)
#define	P3IN_				PBIN_
#endif

#if !defined(P3SEL_) && defined(PBSEL_)
#define	P3SEL_				PBSEL_
#endif

#if !defined(P3DIR_) && defined(PBDIR_)
#define	P3DIR_				PBDIR_
#endif

#if !defined(P3REN_) && defined(PBREN_)
#define	P3REN_				PBREN_
#endif

#if !defined(P3DS_) && defined(PBDS_)
#define	P3DS_				PBDS_
#endif

#if !defined(P3OUT_) && defined(PBOUT_)
#define	P3OUT_				PBOUT_
#endif

// ============================================================================

#if !defined(P4IN_) && defined(PBIN_)
#define	P4IN_				(PBIN_+1)
#endif

#if !defined(P4SEL_) && defined(PBSEL_)
#define	P4SEL_				(PBSEL_+1)
#endif

#if !defined(P4DIR_) && defined(PBDIR_)
#define	P4DIR_				(PBDIR_+1)
#endif

#if !defined(P4REN_) && defined(PBREN_)
#define	P4REN_				(PBREN_+1)
#endif

#if !defined(P4DS_) && defined(PBDS_)
#define	P4DS_				(PBDS_+1)
#endif

#if !defined(P4OUT_) && defined(PBOUT_)
#define	P4OUT_				(PBOUT_+1)
#endif

// ============================================================================

#if !defined(P5IN_) && defined(PCIN_)
#define	P5IN_				PCIN_
#endif

#if !defined(P5SEL_) && defined(PCSEL_)
#define	P5SEL_				PCSEL_
#endif

#if !defined(P5DIR_) && defined(PCDIR_)
#define	P5DIR_				PCDIR_
#endif

#if !defined(P5REN_) && defined(PCREN_)
#define	P5REN_				PCREN_
#endif

#if !defined(P5DS_) && defined(PCDS_)
#define	P5DS_				PCDS_
#endif

#if !defined(P5OUT_) && defined(PCOUT_)
#define	P5OUT_				PCOUT_
#endif
