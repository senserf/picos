// ============================================================================
// 1 led on P4.7, on low
// ============================================================================

#define	LED2_ON		_BIC (P4OUT, 0x80)

#define	LED2_OFF	_BIS (P4OUT, 0x80)


#define	LEDS_SAVE(a)	(a) = (P4OUT & 0x80)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0x80) | ((a) & 0x80); \
			} while (0)
