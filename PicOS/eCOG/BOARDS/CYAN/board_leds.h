/* Negative polarity */

#define	LED0_ON		ZZ_LEDS03 (0x0002)
#define	LED1_ON		ZZ_LEDS03 (0x0020)
#define	LED2_ON		ZZ_LEDS03 (0x0200)
#define	LED3_ON		ZZ_LEDS03 (0x2000)

#define	LED0_OFF	ZZ_LEDS03 (0x0001)
#define	LED1_OFF	ZZ_LEDS03 (0x0010)
#define	LED2_OFF	ZZ_LEDS03 (0x0100)
#define	LED3_OFF	ZZ_LEDS03 (0x1000)

#define	leds_enable	( rg.io.gp0_3_out = \
			  IO_GP0_3_OUT_EN0_MASK | IO_GP0_3_OUT_SET0_MASK | \
			  IO_GP0_3_OUT_EN1_MASK | IO_GP0_3_OUT_SET1_MASK | \
			  IO_GP0_3_OUT_EN2_MASK | IO_GP0_3_OUT_SET2_MASK | \
			  IO_GP0_3_OUT_EN3_MASK | IO_GP0_3_OUT_SET3_MASK )

#define	LEDS_SAVE(a)	(a) = (rg.io.gp0_3_out & 0x2222)

#define LEDS_RESTORE(w)	( rg.io.gp0_3_out = (w) >> 1 )
