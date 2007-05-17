/*
 * Pin assignment (AT45XXX <-> MSP430-xxx)
 *
 *  AT45XXX		        MSP430
 *  =================================
 *  CS				P2.0
 *  SI				P2.1
 *  SO				P2.2
 *  SCK				P2.3
 *  BUSY			P2.4
 */

#define	STORAGE_AT45_TYPE	321	// Select the actual model used

#include "board_pins.h"
#include "storage_at45xxx.h"

#define	ee_ini_regs	do { \
				_BIS (P2OUT, 0x09); \
				_BIS (P2DIR, 0x0b); \
			} while (0)

#define	ee_inp		(P2IN & 0x04)
#define ee_busy		(P2IN & 0x10)

#define	ee_outh		_BIS (P2OUT, 0x02)
#define	ee_outl		_BIC (P2OUT, 0x02)

#define	ee_clkh		_BIS (P2OUT, 0x08)
#define	ee_clkl		_BIC (P2OUT, 0x08)

#define	ee_start	do { _BIC (P2OUT, 0x01); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P2OUT, 0x01); ee_clkh; } while (0)

#define	ee_postinit	CNOP
