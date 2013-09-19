/*
 * Pin assignment (AT45XXX)
 *
 *  AT45XXX		        CC430
 *  =================================
 *  CS				P2.2 STE1
 *  SI				P2.5 SIMO1	DATA ->
 *  SO				P2.3 SOMI1	DATA <-
 *  SCK				P2.4 UCLK1
 */

#define	STORAGE_AT45_TYPE	413	// Select the model used (D41E)

#include "pins.h"
#include "storage_at45xxx.h"

#ifdef	EE_USE_UART
#if	EE_USE_UART
#error	"S: EEPROM connection does not admit EE_USE_UART"
#endif
#else
#define	EE_USE_UART	0
#endif

// Preinitialized in board_pins.h
#define	ee_bring_up	CNOP

#define	ee_inp		(P2IN & 0x08)

#define	ee_outh		_BIS (P2OUT, 0x20)
#define	ee_outl		_BIC (P2OUT, 0x20)

#define	ee_clkh		do { _BIS (P2OUT, 0x10); } while (0)
#define	ee_clkl		do { _BIC (P2OUT, 0x10); } while (0)

#define	ee_start	do { _BIC (P2OUT, 0x04); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P2OUT, 0x04); ee_clkh; } while (0)

#define	ee_bring_down	CNOP
