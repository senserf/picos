// The formula BD = (16384 / f) - 1, where f is the desired frequency
#define	BUZZER_DIVIDER		5

#define	buzzer_init()	do { \
				TA1CTL = TACLR | TASSEL0; \
				TA1CCR0 = BUZZER_DIVIDER; \
				TA1CCTL0 = OUTMOD_4; \
			} while (0)

#define	buzzer_on()	do { \
				_BIS (TA1CTL, MC0); \
				_BIS (P2SEL, 0x80); \
			} while (0)

#define	buzzer_off()	do { \
				_BIC (TA1CTL, MC0 | MC1); \
				_BIC (P2SEL, 0x80); \
			} while (0)
