/*
 * Pin assignment (AT45XXX <-> MSP430-xxx)
 *
 *  AT45XXX		        MSP430
 *  =================================
 *  CS				P5.0 STE1
 *  SI				P5.1 SIMO1	DATA ->
 *  SO				P5.2 SOMI1	DATA <-
 *  SCK				P5.3 UCLK1
 *
 * Pin assignment (SDCARD)
 *
 *  CS				P6.6
 *  DI				P6.5
 *  SCK				P6.4
 *  DO				P6.3
 *
 */

#define	STORAGE_AT45_TYPE	41	// Select the actual model used

#include "board_pins.h"
#include "storage_at45xxx.h"
#include "sdcard.h"

// ===========================================================================

#define	ee_ini_regs	do { \
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

// ============================================================================

#define	SD_DELAY	CNOP

// Initialized statically in board_pins.h
#define sd_ini_regs	CNOP

#define	sd_stop		do { _BIS (P6OUT, 0x40); SD_DELAY; } while (0)
#define	sd_start	do { _BIC (P6OUT, 0x40); SD_DELAY; } while (0)

#define	sd_clkh		do { _BIS (P6OUT, 0x10); SD_DELAY; } while (0)
#define	sd_clkl		do { _BIC (P6OUT, 0x10); SD_DELAY; } while (0)

#define	sd_outh		_BIS (P6OUT, 0x20)
#define	sd_outl		_BIC (P6OUT, 0x20)

#define	sd_inp		(P6IN & 0x08)

// ============================================================================
