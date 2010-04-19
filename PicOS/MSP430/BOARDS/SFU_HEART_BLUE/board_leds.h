// ============================================================================
// 2 leds on P3, polarity 0 ===================================================
// ============================================================================

#define	LED1_ON		_BIC (P3OUT, 0x04)
#define	LED2_ON		_BIC (P3OUT, 0x08)
#define	LEDS_ON		_BIC (P3OUT, 0x0C)

#define	LED1_OFF	_BIS (P3OUT, 0x04)
#define	LED2_OFF	_BIS (P3OUT, 0x08)
#define	LEDS_OFF	_BIS (P3OUT, 0x0C)

#define	LEDS_SAVE(a)	(a) = P3OUT & 0xC
#define	LEDS_RESTORE(a)	P3OUT = (P3OUT & ~0xC) | (a)
