// ============================================================================
// 3 leds on P3, polarity 1 ===================================================
// ============================================================================

#define	LED0_ON		_BIS (P3OUT, 0x01)
#define	LED1_ON		_BIS (P3OUT, 0x02)
#define	LED2_ON		_BIS (P3OUT, 0x04)
#define	LEDS_ON		_BIS (P3OUT, 0x07)

#define	LED0_OFF	_BIC (P3OUT, 0x01)
#define	LED1_OFF	_BIC (P3OUT, 0x02)
#define	LED2_OFF	_BIC (P3OUT, 0x04)
#define	LEDS_OFF	_BIC (P3OUT, 0x07)

#define	LEDS_SAVE(a)	(a) = P3OUT & 0x7
#define	LEDS_RESTORE(a)	P3OUT = (P3OUT & ~0x7) | (a)
