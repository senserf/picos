/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "ecog1.h"

extern void lcd_send (int data) ;


void _lcd_out (const char *m) {

 char *p;

 for (p=(char*)m; (*p) !=0; p++) {

	/* Data */
	rg.io.gp8_11_out = IO_GP8_11_OUT_SET8_MASK;

	rg.io.gp8_11_out = IO_GP8_11_OUT_CLR9_MASK;
	/* Set E high to enable write */
	rg.io.gp8_11_out = IO_GP8_11_OUT_SET10_MASK;

	/* Second stage */
	lcd_send (*p);
  }

}

//never giving up the CPU to keep up with those faster simulated devices!
void sim_lcd (const char *m) {

  _lcd_out("\nLCD: ");
  _lcd_out(m);
  _lcd_out("\n");
}
