/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Departures from default pre-initialization

// 0 = RF SI OUT
// 1 = RF CLK OUT
// 2 = RF SO IN
// 3 = RF GDO2 unused, but must be input
// 4 = RF GDO0 IN
// 5 = RF CSN OUT
// 6,7 Pushable buttons
#define	PIN_DEFAULT_P1DIR	0x23

// 0, 1, 7 hang loose
// 2, 3, 4, 5, 6 = joystick
#define	PIN_DEFAULT_P2DIR	0x83

// 0 doubles with RXD1 (as CTS, and is in the way, so must be input)
// 1 doubles with TXD0 (and is in the way, so must be input)
// 2 doubles with RXD0 (and is in the way, so must be input)
// 3 doubles with TXD1 ....
// 4 is TXD0 (output)
// 5 is RXD0 (input)
// 6 is TXD1
// 7 is RXD1
#define	PIN_DEFAULT_P3DIR	0x50
//#define	PIN_DEFAULT_P3DIR	0xC9

// 1, 2, 3 = LEDs, 0, 4-7 = unused by Nokia LCD
#define PIN_DEFAULT_P4DIR	0xFF

#if LCDG_N6100P
#define	PIN_DEFAULT_P4OUT	0x9E	// CS+RST up (off), LEDs off
#else
#define	PIN_DEFAULT_P4OUT	0x0E	// LEDs off (high) by default
#endif

// 0 = STORAGE CS OUT
// 1 = STORAGE SI OUT
// 2 = STORAGE SO IN
// 3 = STORAGE SCK OUT
// 4 = general and unused by default, 5, 6, 7 hang loose
#define	PIN_DEFAULT_P5DIR	0xFB
//#define	PIN_DEFAULT_P5OUT	0x01	// Default CS is up
#define	PIN_DEFAULT_P5OUT	0x0B	// Default CS is up

// #define	EEPROM_INIT_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

#if ADC_SAMPLER

#define	PIN_DEFAULT_P6DIR	0x00

#define	PIN_LIST	{ }

#define	PIN_MAX			0	// Number of pins
#define	PIN_MAX_ANALOG		0	// Number of available analog pins
#define	PIN_DAC_PINS		0

#else	/* NO SAMPLER */

// P6.7 used for the buzzer
#define	PIN_DEFAULT_P6DIR	0xFF

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
	PIN_DEF	(P6, 3),	\
	PIN_DEF	(P6, 4),	\
	PIN_DEF	(P6, 5),	\
	PIN_DEF	(P6, 6),	\
}

#define	PIN_MAX			8	// Number of pins
#define	PIN_MAX_ANALOG		8	// Number of available analog pins
#define	PIN_DAC_PINS		0x0706	// Two DAC pins: #6 and #7

// Buttons and joystick

#define	P1_PINS_INTERRUPT_MASK	0xc0
#define	P2_PINS_INTERRUPT_MASK	0x7c

#define	PRESSED_BUTTON0		((P1IN & 0x40) == 0)
#define	PRESSED_BUTTON1		((P1IN & 0x80) == 0)
#define	JOYSTICK_N		((P2IN & 0x40) == 0)
#define	JOYSTICK_E		((P2IN & 0x10) == 0)
#define	JOYSTICK_S		((P2IN & 0x08) == 0)
#define	JOYSTICK_W		((P2IN & 0x20) == 0)
#define	JOYSTICK_PUSH		((P2IN & 0x04) == 0)

#define	buttons_init()		do { \
					_BIS (P1IES, P1_PINS_INTERRUPT_MASK); \
					_BIS (P1IE, P1_PINS_INTERRUPT_MASK); \
					_BIS (P2IES, P2_PINS_INTERRUPT_MASK); \
					_BIS (P2IE, P2_PINS_INTERRUPT_MASK); \
				} while (0)

#define	BUTTON_PRESSED_EVENT	((word)(&P1IES))

//+++ "p2irq.c"
REQUEST_EXTERNAL (p2irq);

#define	buzzer_signal_up	_BIS (P6OUT, 0x80)
#define	buzzer_signal_down	_BIC (P6OUT, 0x80)

#define	buzz(a)			do { \
					byte cnt = (a); \
					while (cnt--) { \
						buzzer_signal_up; \
						udelay (3000); \
						buzzer_signal_down; \
						udelay (3000); \
					} \
				} while (0)

#endif /* SAMPLER or no SAMPLER */
