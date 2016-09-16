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

#if !defined(REF2_5V) && defined(ADC12REF2_5V)
#define	REF2_5V				ADC12REF2_5V
#endif

#if !defined(SREF_VREF_AVSS) && defined(ADC12SREF_1)
#define	SREF_VREF_AVSS			ADC12SREF_1
#endif

#if !defined(SREF_VREF_AVSS) && defined(SREF_1)
#define	SREF_VREF_AVSS			SREF_1
#endif

#if !defined(SREF_VEREF_AVSS) && defined(ADC12SREF_2)
#define	SREF_VEREF_AVSS			ADC12SREF_2
#endif

#if !defined(SREF_VEREF_AVSS) && defined(SREF_2)
#define	SREF_VEREF_AVSS			SREF_2
#endif

#if !defined(SREF_AVCC_AVSS) && defined(ADC12SREF_0)
#define	SREF_AVCC_AVSS			ADC12SREF_0
#endif

#if !defined(SREF_AVCC_AVSS) && defined(SREF_0)
#define	SREF_AVCC_AVSS			SREF_0
#endif

// ============================================================================
// Unified port presence constants (byte ports only)
// ============================================================================

#if defined(__MSP430_HAS_PORT0__) || defined(__MSP430_HAS_PORT0_R__)
#define	__PORT0_PRESENT__
#if defined(__MSP430_HAS_PORT0_R__)
#define __PORT0_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT1__) || defined(__MSP430_HAS_PORT1_R__)
#define	__PORT1_PRESENT__
#if defined(__MSP430_HAS_PORT1_R__)
#define __PORT1_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT2__) || defined(__MSP430_HAS_PORT2_R__)
#define	__PORT2_PRESENT__
#if defined(__MSP430_HAS_PORT2_R__)
#define __PORT2_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT3__) || defined(__MSP430_HAS_PORT3_R__)
#define	__PORT3_PRESENT__
#if defined(__MSP430_HAS_PORT3_R__)
#define __PORT3_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT4__) || defined(__MSP430_HAS_PORT4_R__)
#define	__PORT4_PRESENT__
#if defined(__MSP430_HAS_PORT4_R__)
#define __PORT4_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT5__) || defined(__MSP430_HAS_PORT5_R__)
#define	__PORT5_PRESENT__
#if defined(__MSP430_HAS_PORT5_R__)
#define __PORT5_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT6__) || defined(__MSP430_HAS_PORT6_R__)
#define	__PORT6_PRESENT__
#if defined(__MSP430_HAS_PORT6_R__)
#define __PORT6_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT7__) || defined(__MSP430_HAS_PORT7_R__)
#define	__PORT7_PRESENT__
#if defined(__MSP430_HAS_PORT7_R__)
#define __PORT7_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT8__) || defined(__MSP430_HAS_PORT8_R__)
#define	__PORT8_PRESENT__
#if defined(__MSP430_HAS_PORT8_R__)
#define __PORT8_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT9__) || defined(__MSP430_HAS_PORT9_R__)
#define	__PORT9_PRESENT__
#if defined(__MSP430_HAS_PORT9_R__)
#define __PORT9_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT10__) || defined(__MSP430_HAS_PORT10_R__)
#define	__PORT10_PRESENT__
#if defined(__MSP430_HAS_PORT10_R__)
#define __PORT10_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORT11__) || defined(__MSP430_HAS_PORT11_R__)
#define	__PORT11_PRESENT__
#if defined(__MSP430_HAS_PORT11_R__)
#define __PORT11_RESISTOR_PRESENT__
#endif
#endif

#if defined(__MSP430_HAS_PORTJ__) || defined(__MSP430_HAS_PORTJ_R__)
#define	__PORTJ_PRESENT__
#if defined(__MSP430_HAS_PORTJ_R__)
#define __PORTJ_RESISTOR_PRESENT__
#endif
#endif

#define	P0ORD__		0
#define	P1ORD__		1
#define	P2ORD__		2
#define	P3ORD__		3
#define	P4ORD__		4
#define	P5ORD__		5
#define	P6ORD__		6
#define	P7ORD__		7
#define	P8ORD__		8
#define	P9ORD__		9
#define	P10ORD__	10
#define	P11ORD__	11
