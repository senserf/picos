// Two LEDs, polarity 1

#define	LED0_GPIO_BIT	0x0001
#define	LED1_GPIO_BIT	0x1000

#define	LED0_ON		ZZ_LEDS47 (LED0_GPIO_BIT)
#define	LED1_ON		ZZ_LEDS03 (LED1_GPIO_BIT)

#define	LED0_OFF	ZZ_LEDS47 (LED0_GPIO_BIT << 1)
#define	LED1_OFF	ZZ_LEDS03 (LED1_GPIO_BIT << 1)

#define	leds_enable	do { \
			  rg.io.gp0_3_out = \
			    IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_CLR3_MASK; \
			  rg.io.gp4_7_out = \
			    IO_GP4_7_OUT_EN4_MASK | IO_GP4_7_OUT_CLR4_MASK; \
			} while (0)

#define	LEDS_SAVE(a)	(a) = ((rg.io.gp4_7_out & LED0_GPIO_BIT) | \
			 (rg.io.gp0_3_out & LED0_GPIO_BIT))

#define LEDS_RESTORE(w)	do { \
				if (((w) & LED0_GPIO_BIT)) LED0_ON; \
				if (((w) & LED1_GPIO_BIT)) LED1_ON; \
			} while (0)
