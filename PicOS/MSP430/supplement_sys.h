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
