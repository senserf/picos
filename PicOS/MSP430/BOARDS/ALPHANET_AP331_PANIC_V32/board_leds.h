#define	LED0_ON		_BIC (P4OUT, 0x40)
#define	LED0_OFF	_BIS (P4OUT, 0x40)
#define	LED3_ON		_BIC (P4OUT, 0x20)
#define	LED3_OFF	_BIS (P4OUT, 0x20)

#define	LEDS_SAVE(a)	(a) = (P4OUT & 0x60)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0x60) | ((a) & 0x60); \
			} while (0)