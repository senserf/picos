// 2 leds: red on GPIO6, green on GPIO7

#define	LED0_ON		GPIO_setDio (6)
#define	LED0_OFF	GPIO_clearDio (6)

#define	LED1_ON		GPIO_setDio (7)
#define	LED1_OFF	GPIO_clearDio (7)

// Don't do LEDS_SAVE / LEDS_RESTORE which is obsolete (only required for
// glacier)
