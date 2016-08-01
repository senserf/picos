// ============================================================================
// 2 leds on P4.5, P4.7, polarity 0, posing for three LEDs Red == 0, Blue == 2
// ============================================================================

#define	LED0_ON		_BIC (P4OUT, 0x80)
#define	LED2_ON		_BIC (P4OUT, 0x20)

#define	LED0_OFF	_BIS (P4OUT, 0x80)
#define	LED2_OFF	_BIS (P4OUT, 0x20)


#define	LEDS_SAVE(a)	(a) = (P4OUT & 0xA0)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0xA0) | ((a) & 0xA0); \
			} while (0)
