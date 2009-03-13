#ifndef	__pg_buttons_sys_h
#define	__pg_buttons_sys_h

#include "pins.h"

#ifndef	BUTTON_PRESSED_LOW
#define	BUTTON_PRESSED_LOW	0
#endif

#define	button_pin_status(b)	(((BUTTON_PORT (b) == 2) ? P2IN : P1IN) & \
					BUTTON_PIN (b))

#define	button_int_status(b)	(((BUTTON_PORT (b) == 2) ? P2IFG : P1IFG) & \
					BUTTON_PIN (b))

// ============================================================================

#if	BUTTON_PRESSED_LOW

#define	buttons_iedge_p1	_BIS (P1IES, P1_PINS_INTERRUPT_MASK)
#define	buttons_iedge_p2	_BIS (P2IES, P2_PINS_INTERRUPT_MASK)
#define	button_still_pressed(b) (button_pin_status(b) == 0)

#else

#define	buttons_iedge_p1	_BIC (P1IES, P1_PINS_INTERRUPT_MASK)
#define	buttons_iedge_p2	_BIC (P2IES, P2_PINS_INTERRUPT_MASK)
#define	button_still_pressed(b) button_pin_status(b)

#endif	/* BUTTON_PRESSED_LOW */

// ============================================================================

#ifdef	P1_PINS_INTERRUPT_MASK

#define	buttons_enable_p1	do { \
					_BIC (P1IFG, P1_PINS_INTERRUPT_MASK); \
					_BIS (P1IE, P1_PINS_INTERRUPT_MASK); \
				} while (0)

#define	buttons_disable_p1	_BIC (P1IE, P1_PINS_INTERRUPT_MASK)

#define	buttons_init_p1		do { \
					buttons_iedge_p1; \
					_BIC (P1IFG, P1_PINS_INTERRUPT_MASK); \
				} while (0)

REQUEST_EXTERNAL (p1irq);
//+++ "p1irq.c"
#else

#define buttons_enable_p1	CNOP
#define	buttons_disable_p1	CNOP
#define	buttons_init_p1		CNOP

#endif	/* P1 */


#ifdef	P2_PINS_INTERRUPT_MASK

#define	buttons_enable_p2	do { \
					_BIC (P2IFG, P2_PINS_INTERRUPT_MASK); \
					_BIS (P2IE, P2_PINS_INTERRUPT_MASK); \
				} while (0)

#define	buttons_disable_p2	_BIC (P2IE, P2_PINS_INTERRUPT_MASK)

#define	buttons_init_p2		do { \
					buttons_iedge_p2; \
					_BIC (P2IFG, P2_PINS_INTERRUPT_MASK); \
				} while (0)
REQUEST_EXTERNAL (p2irq);
//+++ "p2irq.c"
#else

#define buttons_enable_p2	CNOP
#define	buttons_disable_p2	CNOP
#define	buttons_init_p2		CNOP

#endif	/* P1 */

#define	buttons_enable()	do { \
					buttons_enable_p1; buttons_enable_p2;\
				} while (0)

#define	buttons_disable()	do { \
					buttons_disable_p1; buttons_disable_p2;\
				} while (0)

#define	buttons_init()		do { \
					buttons_init_p1; buttons_init_p2;\
				} while (0)

#define	button_pressed(b)	button_int_status (b)


#endif
