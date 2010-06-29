/*
 * Pin assignment (AT45XXX <-> MSP430-xxx)
 *
 *  AT45XXX		        MSP430
 *  =================================
 *  CS				P5.0 STE1
 *  SI				P5.1 SIMO1	DATA ->
 *  SO				P5.2 SOMI1	DATA <-
 *  SCK				P5.3 UCLK1
 */

#define	STORAGE_AT45_TYPE	41	// Select the actual model used

#include "pins.h"
#include "storage_at45xxx.h"

#define	ee_bring_up	do { \
				_BIS (P5OUT, 0x01); \
				_BIS (P5DIR, 0x0b); \
			} while (0)

#define	ee_inp		(P5IN & 0x04)

#define	ee_outh		_BIS (P5OUT, 0x02)
#define	ee_outl		_BIC (P5OUT, 0x02)

#define	ee_clkh		_BIS (P5OUT, 0x08)
#define	ee_clkl		_BIC (P5OUT, 0x08)

#define	ee_start	do { _BIC (P5OUT, 0x01); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P5OUT, 0x01); ee_clkh; } while (0)

#define	ee_bring_down	CNOP
