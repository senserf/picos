/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ez430_lcd.h"
#include "rtc_cc430.h"

word dig = 0;

#define	RS_INIT		0
#define	RS_LOOP		1

rtc_time_t	D = { 10, 2, 3, 3, 19, 58, 0 };

void msg_lcd (const char *txt, word fr, word to) {
//
// Displays characters on the LCD
//
	char c;

	while (1) {
		if ((c = *txt) != '\0') {
			if (c >= 'a' && c <= 'z')
				c -= ('a' - 'A');
			ezlcd_item (fr, (word)c | LCD_MODE_SET);
			txt++;
		} else {
			ezlcd_item (fr, LCD_MODE_CLEAR);
		}
		if (fr == to)
			return;
		if (fr > to)
			fr--;
		else
			fr++;
	}
}

char msg [4];

void msg_td (byte n, word seg) {

	msg [1] = (n % 10) + '0';
	n /= 10;
	if (n > 9)
		n = 9;
	msg [0] = n + '0';

	msg_lcd (msg, seg, seg - 1);
}

thread (root)

  entry (RS_INIT)

	ezlcd_init ();
	ezlcd_on ();

	rtc_set (&D);

	bzero (&D, sizeof (D));
	ezlcd_item (LCD_SEG_L1_COL, LCD_MODE_BLINK);

  entry (RS_LOOP)

	rtc_get (&D);

	msg_td (D.minute, LCD_SEG_L1_3);
	msg_td (D.hour, LCD_SEG_L1_1);

	msg_td (D.day, LCD_SEG_L2_5);
	msg_td (D.month, LCD_SEG_L2_1);
	msg_td (D.second, LCD_SEG_L2_3);

	delay (256, RS_LOOP);

endthread
