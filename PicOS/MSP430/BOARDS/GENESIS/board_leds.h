// ============================================================================
// 3 leds on P6, polarity 1 ===================================================
// ============================================================================

#define	LED0_ON		_BIS (P6OUT, 0x02)
#define	LED1_ON		_BIS (P6OUT, 0x04)
#define	LED2_ON		_BIS (P6OUT, 0x08)
#define	LEDS_ON		_BIS (P6OUT, 0x0E)

#define	LED0_OFF	_BIC (P6OUT, 0x02)
#define	LED1_OFF	_BIC (P6OUT, 0x04)
#define	LED2_OFF	_BIC (P6OUT, 0x08)
#define	LEDS_OFF	_BIC (P6OUT, 0x0E)

#define	LEDS_SAVE(a)	(a) = P6OUT & 0xE
#define	LEDS_RESTORE(a)	P6OUT = (P6OUT & ~0xE) | (a)
