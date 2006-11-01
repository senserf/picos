
#undef	LED0_ON
#undef	LED1_ON
#undef	LED2_ON
#undef	LED3_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	LED2_OFF
#undef	LED3_OFF

/* Positive polarity, two leds only */

#define	LED0_ON		ZZ_LEDS47 (0x0001)
#define	LED1_ON		ZZ_LEDS03 (0x1000)
#define	LED2_ON		do { } while (0)
#define	LED3_ON		do { } while (0)

#define	LED0_OFF	ZZ_LEDS47 (0x0002)
#define	LED1_OFF	ZZ_LEDS03 (0x2000)
#define	LED2_OFF	do { } while (0)
#define	LED3_OFF	do { } while (0)

#define	leds_enable	do { \
			  rg.io.gp0_3_out = \
			    IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_CLR3_MASK; \
			  rg.io.gp4_7_out = \
			    IO_GP4_7_OUT_EN4_MASK | IO_GP4_7_OUT_CLR4_MASK; \
			} while (0)
