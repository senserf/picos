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

// #define	RESET_ON_KEY_PRESSED	((P2IN & 0x10) == 0)

// SD card on P6.3-P6.6
// P6.7 used for the buzzer
#define	PIN_DEFAULT_P6DIR	0xF7
#define	PIN_DEFAULT_P6OUT	0x60	// CS and DI high

#define	PIN_LIST	{	\
	PIN_DEF	(P6, 0),	\
	PIN_DEF	(P6, 1),	\
	PIN_DEF	(P6, 2),	\
}

#define	PIN_MAX			3	// Number of pins
#define	PIN_MAX_ANALOG		3	// Number of available analog pins
#define	PIN_DAC_PINS		0x0

// Buttons and joystick

#define	BUTTON_LIST	{	\
		BUTTON_DEF (1, 0x40, 0), \
		BUTTON_DEF (1, 0x80, 0), \
		BUTTON_DEF (2, 0x40, 1), \
		BUTTON_DEF (2, 0x10, 1), \
		BUTTON_DEF (2, 0x08, 1), \
		BUTTON_DEF (2, 0x20, 1), \
		BUTTON_DEF (2, 0x04, 0)  \
	}
//
#define	BUTTON_0		0
#define	BUTTON_1		1
#define	JOYSTICK_N		2
#define	JOYSTICK_E		3
#define	JOYSTICK_S		4
#define	JOYSTICK_W		5
#define	JOYSTICK_PUSH		6

#define	IS_JOYSTICK(b)		((b) >= JOYSTICK_N && (b) <= JOYSTICK_PUSH)

#define	P1_PINS_INTERRUPT_MASK	0xc0
#define	P2_PINS_INTERRUPT_MASK	0x7c

#define	BUTTON_DEBOUNCE_DELAY	64
#define	BUTTON_REPEAT_DELAY	630
#define	BUTTON_REPEAT_INTERVAL	256

#define	BUTTON_PRESSED_LOW	1

// ============================================================================

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
