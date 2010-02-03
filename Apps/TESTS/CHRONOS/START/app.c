/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ez430_lcd.h"

word dig = 0;

#define	RS_INIT		0
#define	RS_LOOP		1
#define	RS_DONE		2

thread (root)

  entry (RS_INIT)

	ezlcd_init ();
	ezlcd_on ();

  entry (RS_LOOP)

	ezlcd_item (LCD_SEG_L1_0, (word) (dig + 0x30));
	if (dig == 9)
		//dig = 0;
		proceed (RS_DONE);
	else
		dig++;
	delay (1024, RS_LOOP);
	release;

  entry (RS_DONE)

	diag ("STOP");
	finish;

endthread
