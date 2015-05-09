// ===================================
// 2 leds on P4.5 and P4.7, polarity 0
// ===================================

#define	LED0_ON		_BIC (P4OUT, 0x80)
#define	LED1_ON		_BIC (P4OUT, 0x20)

#define	LED0_OFF	_BIS (P4OUT, 0x80)
#define	LED1_OFF	_BIS (P4OUT, 0x20)

#define	LEDS_SAVE(a)	(a) = (P4OUT & 0xA0)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0xA0) | ((a) & 0xA0); \
			} while (0)
