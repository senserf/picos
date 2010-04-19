// ============================================================================
// 2 leds on P1 and P3, polarity 1 ============================================
// ============================================================================

#define	LED0_ON		_BIS (P1OUT, 0x01)
#define	LED1_ON		_BIS (P3OUT, 0x40)

#define	LED0_OFF	_BIC (P1OUT, 0x01)
#define	LED1_OFF	_BIC (P3OUT, 0x40)

#define	LEDS_SAVE(a)	(a) = (P1OUT & 0x01) | (P3OUT & 0x40)
#define	LEDS_RESTORE(a)	do { \
				P1OUT = (P1OUT & ~0x01) | ((a) & 0x01); \
				P3OUT = (P3OUT & ~0x40) | ((a) & 0x40); \
			} while (0)
