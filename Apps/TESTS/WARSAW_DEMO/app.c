/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN	80

#define	RS_INIT		0
#define	FS_LOOP	0

char *Flash;
byte led;

thread (flasheur)

  word pos, led, sta;

  entry (FS_LOOP)

	if (Flash == NULL || ((sta = strlen (Flash)) == 0))
		finish;

	if (sta >= 32)
		pos = 0;
	else {
		sta = 32 - sta;
		while (1) {
			pos = rnd () & 0x1f;
			if (pos <= sta)
				break;
		}
	}

	led++;

	for (sta = 0; sta < 3; sta++) {
		if ((led & (1 << sta)))
			leds (sta, 1);
		else
			leds (sta, 0);
	}
		
	lcd_clear (0, 0);
	lcd_write (pos, Flash);
	delay (1024, FS_LOOP);

endthread

static void buttons (word but) {

	switch (but) {

		case 0:
			Flash = "ZERO";
			break;
		case 1:
			Flash = "ONE";
			break;
		case 2:
			Flash = "TWO";
			break;
		case 4:
			Flash = "FOUR";
			break;
	}
}

thread (root)

	static char *ibuf, *sbuf;
	static int n, k;
	static word a, b, c;

  entry (RS_INIT)

	lcd_on (0);
	Flash = "OLSONET";
	runthread (flasheur);
	buttons_action (buttons);

	finish;

endthread
