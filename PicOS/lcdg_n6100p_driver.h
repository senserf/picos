#ifndef	__pg_lcdg_n6100p_driver_h__
#define	__pg_lcdg_n6100p_driver_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// This is the PicOS version of the combined headers for lcdg_n6100p.c. The
// same source of the driver with a different version of this file will be
// used by VUEE.

#include "kernel.h"
#include "pins.h"
#include "lcdg_n6100p.h"

#ifdef LCDG_FONT_BASE
// Need to access EEPROM
#include "storage.h"
#endif

#ifndef	LCDG_N6100P_EPSON
#define	LCDG_N6100P_EPSON	0
#endif

#if LCDG_N6100P_EPSON

// Line/column offsets
#define	LCDG_XOFF	0
#define	LCDG_YOFF	2

// ============================================================================
#define DISON     	0xAF // Display on 
#define DISOFF    	0xAE // Display off 
#define DISNOR    	0xA6 // Normal display 
#define DISINV    	0xA7 // Inverse display 
#define COMSCN    	0xBB // Common scan direction 
#define DISCTL    	0xCA // Display control 
#define SLPIN     	0x95 // Sleep in 
#define SLPOUT    	0x94 // Sleep out 
#define PASET     	0x75 // Page address set 
#define CASET     	0x15 // Column address set 
#define DATCTL    	0xBC // Data scan direction, etc. 
#define RGBSET8   	0xCE // 256-color position set 
#define RAMWR     	0x5C // Writing to memory 
#define RAMRD     	0x5D // Reading from memory 
#define PTLIN     	0xA8 // Partial display in 
#define PTLOUT    	0xA9 // Partial display out 
#define RMWIN     	0xE0 // Read and modify write 
#define RMWOUT    	0xEE // End 
#define ASCSET    	0xAA // Area scroll set 
#define SCSTART   	0xAB // Scroll start set 
#define OSCON     	0xD1 // Internal oscillation on 
#define OSCFF    	0xD2 // Internal oscillation off 
#define PWRCTR    	0x20 // Power control 
#define VOLCTR    	0x81 // Electronic volume control 
#define VOLUP     	0xD6 // Increment electronic control by 1 
#define VOLDOWN   	0xD7 // Decrement electronic control by 1 
#define TMPGRD    	0x82 // Temperature gradient set 
#define EPCTIN    	0xCD // Control EEPROM 
#define EPCOUT    	0xCC // Cancel EEPROM control 
#define EPMWR     	0xFC // Write into EEPROM 
#define EPMRD     	0xFD // Read from EEPROM 
#define EPSRRD1   	0x7C // Read register 1 
#define EPSRRD2   	0x7D // Read register 2 
#define NOP       	0x25 // NOP instruction 

#else	/* PHILIPS */
// ============================================================================

#define	LCDG_XOFF	1
#define	LCDG_YOFF	1

#define	LNOP		0x00 // nop
#define	SWRESET		0x01 // software reset
#define	BSTROFF		0x02 // booster voltage OFF
#define	BSTRON		0x03 // booster voltage ON
#define	RDDIDIF		0x04 // read display identification
#define	RDDST		0x09 // read display status
#define	SLEEPIN		0x10 // sleep in
#define	SLEEPOUT	0x11 // sleep out
#define	PTLON		0x12 // partial display mode
#define	NORON		0x13 // display normal mode
#define	INVOFF		0x20 // inversion OFF
#define	INVON		0x21 // inversion ON
#define	DALO		0x22 // all pixels OFF
#define	DAL		0x23 // all pixels ON
#define	SETCON		0x25 // write contrast
#define	DISPOFF		0x28 // display OFF
#define	DISPON		0x29 // display ON
#define	CASET		0x2A // column address set
#define	PASET		0x2B // page address set
#define	RAMWR		0x2C // memory write
#define	RGBSET		0x2D // colour set
#define	PTLAR		0x30 // partial area
#define	VSCRDEF		0x33 // vertical scrolling definition
#define	TEOFF		0x34 // test mode
#define	TEON		0x35 // test mode
#define	MADCTL		0x36 // memory access control
#define	SEP		0x37 // vertical scrolling start address
#define	IDMOFF		0x38 // idle mode OFF
#define	IDMON		0x39 // idle mode ON
#define	COLMOD		0x3A // interface pixel format
#define	SETVOP		0xB0 // set Vop
#define	BRS		0xB4 // bottom row swap
#define	TRS		0xB6 // top row swap
#define	DISCTR		0xB9 // display control
#define	DOR		0xBA // data order
#define	TCDFE		0xBD // enable/disable DF temperature compensation
#define	TCVOPE		0xBF // enable/disable Vop temp comp
#define	EC		0xC0 // internal or external oscillator
#define	SETMUL		0xC2 // set multiplication factor
#define	TCVOPAB		0xC3 // set TCVOP slopes A and B
#define	TCVOPCD		0xC4 // set TCVOP slopes c and d
#define	TCDF		0xC5 // set divider frequency
#define	DF8COLOR	0xC6 // set divider frequency 8-color mode
#define	SETBS		0xC7 // set bias system
#define	RDTEMP		0xC8 // temperature read back
#define	NLI		0xC9 // n-line inversion
#define	RDID1		0xDA // read ID1
#define	RDID2		0xDB // read ID2
#define	RDID3		0xDC // read ID3

#endif	/* PHILIPS or EPSON */
// ============================================================================

#endif
