#define	STORAGE_AT45_TYPE	410	// Select the actual model used (D)
#define	EEPROM_PDMODE_AVAILABLE	1

#include "pins.h"
#include "storage_at45xxx.h"

#ifdef	EE_USE_UART
#undef	EE_USE_UART
#endif

#define	EE_USE_UART	0

// ============================================================================
// Direct mode ================================================================
// ============================================================================

#define	ee_bring_up	do { \
				_BIS (P4OUT, 0x01); \
				_BIS (P1OUT, 0x18); \
				cswitch_on (CSWITCH_EE); \
				mdelay (10); \
			} while (0)

#define	ee_bring_down	do { \
				mdelay (10); \
				cswitch_off (CSWITCH_EE); \
				_BIC (P4OUT, 0x01); \
				_BIC (P1OUT, 0x18); \
			} while (0)

#define	ee_inp		(P1IN & 0x04)

#define	ee_outh		_BIS (P1OUT, 0x08)
#define	ee_outl		_BIC (P1OUT, 0x08)

#define	ee_clkh		_BIS (P1OUT, 0x10)
#define	ee_clkl		_BIC (P1OUT, 0x10)

#define	ee_start	do { _BIC (P4OUT, 0x01); ee_clkl; } while (0)
#define	ee_stop		do { _BIS (P4OUT, 0x01); ee_clkh; } while (0)
