// ============================================================================
// 3 leds on P4, polarity low, driven by changing pin direction
// ============================================================================

// Red
#define	LED2_ON		_BIC (P4OUT, 0x80)
// Green
#define	LED1_ON		_BIC (P4OUT, 0x40)
// Yellow
#define	LED0_ON		_BIC (P4OUT, 0x20)

#define	LED2_OFF	_BIS (P4OUT, 0x80)
#define	LED1_OFF	_BIS (P4OUT, 0x40)
#define	LED0_OFF	_BIS (P4OUT, 0x20)

#define	LEDS_SAVE(a)	(a) = (P4DIR & 0xE0)
#define	LEDS_RESTORE(a)	P4DIR = (P4DIR & ~0xE0) | ((a) & 0xE0)
