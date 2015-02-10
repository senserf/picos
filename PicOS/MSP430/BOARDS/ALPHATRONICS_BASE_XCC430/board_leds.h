// ============================================================================
// 3 leds on P4, active high
// ============================================================================

#define	LED0_ON		_BIS (P4OUT, 0x20)
#define	LED1_ON		_BIS (P4OUT, 0x40)
#define	LED2_ON		_BIS (P4OUT, 0x80)

#define	LED0_OFF	_BIC (P4OUT, 0x20)
#define	LED1_OFF	_BIC (P4OUT, 0x40)
#define	LED2_OFF	_BIC (P4OUT, 0x80)

#define	LEDS_SAVE(a)	(a) = (P4OUT & 0xE0)
#define	LEDS_RESTORE(a)	P4OUT = (P4OUT & ~0xE0) | ((a) & 0xE0)
