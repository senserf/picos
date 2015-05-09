#define	LED0_ON		_BIC (P4OUT, 0x10)
#define	LED0_OFF	_BIS (P4OUT, 0x10)
#define	LED1_ON		_BIC (P4OUT, 0x20)
#define	LED1_OFF	_BIS (P4OUT, 0x20)

#define	LEDS_SAVE(a)	(a) = (P4OUT & 0x30)
#define	LEDS_RESTORE(a)	do { \
				P4OUT = (P4OUT & ~0x30) | ((a) & 0x30); \
			} while (0)
