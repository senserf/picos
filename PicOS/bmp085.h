#ifndef __pg_bmp085_h
#define	__pg_bmp085_h

#include "bmp085_sys.h"
//+++ "bmp085.c"

void bmp085_read (word, const byte*, address);
void bmp085_read_calib (word, const byte*, address);

extern byte bmp085_osrs;

#define	bmp085_oversample(b) bmp085_osrs = (byte)(b)

#endif
