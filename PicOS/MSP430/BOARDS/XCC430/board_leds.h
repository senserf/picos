#undef	LED0_ON
#undef	LED1_ON
#undef	LED0_OFF
#undef	LED1_OFF
#undef	leds_save
#undef	leds_off
#undef	leds_restore

#define	LED0_ON		ZZ_LEDON (P1, 0x01)
#define	LED1_ON		ZZ_LEDON (P3, 0x40)

#define	LED0_OFF	ZZ_LEDOFF (P1, 0x01)
#define	LED1_OFF	ZZ_LEDOFF (P3, 0x40)

#define	leds_save()	((P1OUT & 0x01) | (P3OUT & 0x40))
#define	leds_off()	do { LED0_OFF; LED1_OFF; } while (0)
#define	leds_restore(w)	do { \
				ZZ_LEDON (P1, (w) & 0x01); \
				ZZ_LEDON (P3, (w) & 0x40); \
			} while (0)
