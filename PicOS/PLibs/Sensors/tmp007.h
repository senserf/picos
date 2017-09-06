#ifndef __pg_tmp007_h
#define	__pg_tmp007_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2017                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This address is selected by grounding both address definition pins; 7 bits
// only (MSB must be zero)
#ifndef	TMP007_ADDR
// This is the default selected by connecting both address bits to ground
#define	TMP007_ADDR	0x40		// 1000000
#endif

// Registers
#define	TMP007_REG_SVOL			0x00	// Sensor voltage
#define	TMP007_REG_TDIE			0x01	// Local temperature
#define	TMP007_REG_CONF			0x02	// Configuration
#define	TMP007_REG_TOBJ			0x03	// Object temperature
#define	TMP007_REG_STAT			0x04	// Status
#define	TMP007_REG_SMEN			0x05	// Status mask & enable
#define	TMP007_REG_TOHL			0x06	// Object temp high limit
#define	TMP007_REG_TOLL			0x07	// Object temp low limit
#define	TMP007_REG_TDHL			0x08	// Local temp high limit
#define	TMP007_REG_TDLL			0x09	// Local temp low limit
#define TMP007_REG_S0			0x0a	// Coefficients
#define TMP007_REG_A1			0x0b
#define TMP007_REG_A2			0x0c
#define TMP007_REG_B0			0x0d
#define TMP007_REG_B1			0x0e
#define TMP007_REG_B2			0x0f
#define TMP007_REG_C2			0x10
#define TMP007_REG_TC0			0x11
#define TMP007_REG_TC1			0x12
#define	TMP007_REG_MNID			0x1e	// Manufacturer ID
#define	TMP007_REG_DVID			0x1f	// Device ID
#define	TMP007_REG_MADD			0x2a	// Memory address

//
//
//	CONF reg:
//		bit 5 	  :	INT mode (0), COMP mode (1) [limits]	[0]
//		bit 6	  :	TC (correction enable)			[1]
//		bit 7	  :	ALERT FLAG (RO) cleared when read	[x]
//		bit 8	  :	ALRTEN (ALERT -> INT, trig low)		[0]
//		bits 9-11 :	averages per conv			[010]
//		bit 12	  :	power up/down (0 - down)		[1]
//

#define	TMP007_CONFIG_COMP		0x0020		// Compare mode
#define	TMP007_CONFIG_ALERT		0x0100		// Trigger int
#define	TMP007_CONFIG_CORRECT		0x0040		// TC
#define	TMP007_CONFIG_AV1		0x0000		// Single sample
#define	TMP007_CONFIG_AV2		0x0200		// Two
#define	TMP007_CONFIG_AV4		0x0400		// Four (default)
#define	TMP007_CONFIG_AV8		0x0600		// Four (default)
#define	TMP007_CONFIG_AV16		0x0800		// 16
#define	TMP007_CONFIG_AV1_LP		0x0a00		// 1 + idle
#define	TMP007_CONFIG_AV2_LP		0x0c00		// 2 + idle
#define	TMP007_CONFIG_AV4_LP		0x0e00		// 4 + idle
#define	TMP007_CONFIG_PD		0x0000		// Power down

#define	TMP007_ENABLE_ALERT		0x8000		// Trigger
#define	TMP007_ENABLE_CRT		0x4000		// Conversion ready
#define	TMP007_ENABLE_OH		0x2000		// Object temp high
#define	TMP007_ENABLE_OL		0x1000		// Object temp low
#define	TMP007_ENABLE_LH		0x0800		// Local temp high
#define	TMP007_ENABLE_LL		0x0400		// Local temp low
#define	TMP007_ENABLE_DV		0x0200		// Data invalid
#define	TMP007_ENABLE_MC		0x0100		// Memory corrupt

typedef struct {
//
// Sensor value: two temperature readings
//
	word	tdie, tobj;
} tmp007_data_t;

#ifndef __SMURPH__

#include "pins.h"
//+++ "tmp007.c"

void tmp007_init (void);
void tmp007_read (word, const byte*, address);

extern byte tmp007_status;

#define	TMP007_STATUS_ON	0x01
#define	TMP007_STATUS_WAIT	0x02
#define	TMP007_STATUS_EVENT	0x04

void tmp007_on (word, word);		// Mode, enable
void tmp007_off ();
void tmp007_setlimits (wint, wint, wint, wint);	// OH, OL, LH, LL
word tmp007_rreg (byte);
void tmp007_wreg (byte, word);

#else	/* __SMURPH__ */

#define	tmp007_on(a,b)		emul (9, "TMP007_ON: %04x %04x", a, b)
#define	tmp007_off()		emul (9, "TMP007_OFF: <>")
#define	tmp007_setlimits(a,b,c,d) \
				emul (9, "TMP007_SETLIMITS: %1d %1d %1d %1d", \
					a, b, c, d)
#define	tmp007_rreg(r)		0
#define	tmp007_wreg(r,v)	emul (9, "TMP007_WREG: %02x %04x", r, v)

#endif	/* __SMURPH__ */


#endif
