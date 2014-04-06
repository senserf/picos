#ifndef __pg_bmp085_h
#define	__pg_bmp085_h

#ifdef	__SMURPH__

// ============================================================================
// Dummies for VUEE models (auto-calibrate version only)
// ============================================================================

typedef struct {
//
// Sensor value
//
	unsigned int temp:8, press:24;

} bmp085_data_t;

#define	bmp085_oversample(b) emul (10, "BMP085_OVERSAMPLE: %1d", b)

// ============================================================================
#else	/* Real life */
// ============================================================================

#include "bmp085_sys.h"
//+++ "bmp085.c"

void bmp085_read (word, const byte*, address);
void bmp085_init ();

extern byte bmp085_osrs;

#define	bmp085_oversample(b) bmp085_osrs = (byte)(b)

#ifdef	BMP085_AUTO_CALIBRATE

typedef struct {
//
// Calibration data
//
	sint ac1;
	sint ac2;
	sint ac3;
	word ac4;
	word ac5;
	word ac6;
	sint b1;
	sint b2;
	sint mb;
	sint mc;
	sint md;      		   

} bmp_085_calibration_data_t;

typedef struct {
//
// Sensor value
//
	lword press;
	sint temp;

} bmp085_data_t;

#else

void bmp085_read_calib (word, const byte*, address);

typedef struct {
//
// Sensor value
//
	word press;
	sint temp;

} bmp085_data_t;

#endif	/* BMP085_AUTO_CALIBRATE */

#endif	/* __SMURPH__ or real life */

#endif
