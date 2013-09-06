// ============================================================================
// 3 leds on P4.5, P4.6, P4.7, polarity 0
// ============================================================================

#define	LED0_ON		_BIS (P4DIR, 0x20)
#define	LED1_ON		_BIS (P4DIR, 0x40)
#define	LED2_ON		_BIS (P4DIR, 0x80)

#define	LED0_OFF	_BIC (P4DIR, 0x20)
#define	LED1_OFF	_BIC (P4DIR, 0x40)
#define	LED2_OFF	_BIC (P4DIR, 0x80)

#define	LEDS_SAVE(a)	(a) = (P4DIR & 0xE0)
#define	LEDS_RESTORE(a)	do { \
				P4DIR = (P4DIR & ~0xE0) | ((a) & 0xE0); \
			} while (0)
