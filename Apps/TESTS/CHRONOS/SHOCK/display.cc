#include "display.h"
#include "butts.h"

// Flag == 24h clock; perhaps we should use a byte of bit flags?
static Boolean W24H;

// ============================================================================

static void s2d (word seg, byte w, Boolean blink) {
//
// Display two digits in the indicated segment, optionally blinking
//
	word attr = blink ? LCD_MODE_BLINK : LCD_MODE_SET;
	ezlcd_item (seg++, (w % 10) | attr);
	w /= 10;
	if (seg == LCD_SEG_L2_5 || seg == LCD_SEG_L1_3) {
		// In these segments, zero is blank
		if (!w)
			attr = LCD_MODE_CLEAR;
	}
	ezlcd_item (seg, w | attr);
}

static void clear_lcd (word s, word t) {

	while (1) {
		ezlcd_item (s, LCD_MODE_CLEAR);
		if (s == t)
			return;
		if (s < t)
			s++;
		else
			s--;
	}
}

// ============================================================================
// A minute watch in the upper row
// ============================================================================

static void mw_show () {
//
	word am, pm, hr;

	// Make the colon blink
	ezlcd_item (LCD_SEG_L1_COL, LCD_MODE_BLINK);

	hr = RTC.hour;
	am = pm = LCD_MODE_CLEAR;

	if (!W24H) {
		pm = LCD_MODE_SET;
		if (hr > 12)
			hr -= 12;
		else
			am = LCD_MODE_SET;
	}

	ezlcd_item (LCD_SYMB_AM, am);
	ezlcd_item (LCD_SYMB_PM, pm);

	s2d (LCD_SEG_L1_2, (byte)hr, NO);
	s2d (LCD_SEG_L1_0, RTC.minute, NO);
}

static void mw_clear () {

	clear_lcd (LCD_SEG_L1_COL, LCD_SEG_L1_COL);
	clear_lcd (LCD_SYMB_AM, LCD_SYMB_PM);
	clear_lcd (LCD_SEG_L1_0, LCD_SEG_L1_3);
}

static void mw_update () {
//
// To be queued for 1 min updates
//
	if (RTC.minute == 0)
		mw_show ();
	else
		s2d (LCD_SEG_L1_0, RTC.minute, NO);
}

// ============================================================================
// Seconds displayed in second row (update is same as show, 1 sec intervals)
// ============================================================================

static void sw_show () {
	s2d (LCD_SEG_L2_0, RTC.second, NO);
}

static void sw_clear () {

	clear_lcd (LCD_SEG_L2_0, LCD_SEG_L2_1);
}

// ============================================================================
// Calendar displayed in second row
// ============================================================================

static void cw_show () {

	ezlcd_item (LCD_SEG_L2_COL0, LCD_MODE_SET);
	ezlcd_item (LCD_SEG_L2_COL1, LCD_MODE_SET);
	s2d (LCD_SEG_L2_4, RTC.month, NO);
	s2d (LCD_SEG_L2_2, RTC.day, NO);
	s2d (LCD_SEG_L2_0, RTC.year, NO);
}

static void cw_clear () {

	clear_lcd (LCD_SEG_L2_COL1, LCD_SEG_L2_COL0);
	clear_lcd (LCD_SEG_L2_0, LCD_SEG_L2_5);
}

static void cw_update () {
//
// Every minute
//
	if (RTC.hour == 0 && RTC.minute == 0)
		// The only moment when the calendar changes
		cw_show ();
}

// ============================================================================
// ============================================================================
// ============================================================================

// ============================================================================
// Setting the watch ==========================================================
// ============================================================================

static rtc_time_t SRTC;	// SRTC <- RTC before setting
static Boolean w24h;

//
// WatchBeingSet indicates what we are setting:
//
//	0 - seconds
//	1 - minutes
//	2 - AM/PM/24H
//	3 - hours
//	4 - day
//	5 - month
//	6 - year
//

// ============================================================================
// Minute watch set
// ============================================================================

void mwset_show () {
//
	word am, pm, hr;

	// Solid, no blinking
	ezlcd_item (LCD_SEG_L1_COL, LCD_MODE_SET);

	ezlcd_item (LCD_SYMB_ARROW_UP,
		WatchBeingSet == 2 ? LCD_MODE_BLINK : LCD_MODE_CLEAR);

	hr = SRTC.hour;
	am = pm = LCD_MODE_CLEAR;

	if (!w24h) {
		pm = LCD_MODE_SET;
		if (hr > 12)
			hr -= 12;
		else
			am = LCD_MODE_SET;
	}

	if (WatchBeingSet == 2) {
		am |= LCD_MODE_BLINK;
		pm |= LCD_MODE_BLINK;
	} 

	ezlcd_item (LCD_SYMB_AM, am);
	ezlcd_item (LCD_SYMB_PM, pm);

	s2d (LCD_SEG_L1_2, (byte)hr, WatchBeingSet == 3);
	s2d (LCD_SEG_L1_0, SRTC.minute, WatchBeingSet == 1);
}

void mwset_clear () {

	mw_clear ();
	ezlcd_item (LCD_SYMB_ARROW_UP, LCD_MODE_CLEAR);
}

Boolean mwset_button () {

	sint upd;

	if (TheButton == BUTTON_RT)
		upd = 1;
	else if (TheButton == BUTTON_RB)
		upd = -1;
	else
		return NO;

	switch (WatchBeingSet) {

		case 1:

			upd += SRTC.minute;
			if (upd < 0)
				upd = 59;
			else if (upd > 59)
				upd = 0;

			SRTC.minute = (byte)upd;

			break;

		case 2:

			if (w24h) {
				if (SRTC.hour >= 12) 
					SRTC.hour -= 12;
				w24h = NO;
				if (upd > 0)
					SRTC.hour += 12;
			} else if (upd > 0) {
				if (SRTC.hour >= 12)
					w24h = YES;
				else
					SRTC.hour += 12;
			} else {
				if (SRTC.hour >= 12)
					SRTC.hour -= 12;
				else
					w24h = YES;
			}

			break;

		case 3:

			upd += SRTC.hour;
			if (upd < 0)
				upd = 23;
			else if (upd > 23)
				upd = 0;

			SRTC.hour = (byte)upd;

			break;

		default:

			return NO;
	}

	mwset_show ();

	return YES;
}

// ============================================================================
// Seconds watch set
// ============================================================================

void swset_show () {

	s2d (LCD_SEG_L2_0, SRTC.second, WatchBeingSet == 0);
}

Boolean swset_button () {

	sint upd;

	if (TheButton == BUTTON_RT)
		upd = 1;
	else if (TheButton == BUTTON_RB)
		upd = -1;
	else
		return NO;

	if (WatchBeingSet != 0)
		return NO;

	upd += SRTC.second;
	if (upd < 0)
		upd = 59;
	else if (upd > 59)
		upd = 0;

	SRTC.second = (byte) upd;

	swset_show ();

	return YES;
}

// ============================================================================
// Calendar watch set
// ============================================================================

void cwset_show () {

	ezlcd_item (LCD_SEG_L2_COL0, LCD_MODE_SET);
	ezlcd_item (LCD_SEG_L2_COL1, LCD_MODE_SET);
	s2d (LCD_SEG_L2_4, SRTC.month, WatchBeingSet == 5);
	s2d (LCD_SEG_L2_2, SRTC.day, WatchBeingSet == 4);
	s2d (LCD_SEG_L2_0, SRTC.year, WatchBeingSet == 6);
}

Boolean cwset_button () {

	sint upd, max;

	if (TheButton == BUTTON_RT)
		upd = 1;
	else if (TheButton == BUTTON_RB)
		upd = -1;
	else
		return NO;

	switch (WatchBeingSet) {

		case 4:

			if (SRTC.month == 2) {
				max = 28;
				if ((SRTC.year & 0x3) == 0)
					max++;
			} else {
				if ((SRTC.month > 7 && (SRTC.month & 1) == 0)
				 || (SRTC.month < 8 && (SRTC.month & 1) != 0))
					max = 31;
				else
					max = 30;
			}

			upd += SRTC.day;
			if (upd <= 0)
				upd = max;
			else if (upd > max)
				upd = 1;

			SRTC.day = (byte) upd;

			break;

		case 5:

			upd += SRTC.month;
			if (upd > 12)
				upd = 1;
			else if (upd < 1)
				upd = 12;

			SRTC.month = (byte) upd;

			break;

		case 6:

			upd += SRTC.year;
			if (upd > 99)
				upd = 14;
			else if (upd < 14)
				upd = 99;

			SRTC.year = (byte) upd;

			break;

		default:

			return NO;
	}

	cwset_show ();
	return YES;
}

// ============================================================================

void display_setwatch () {

	memcpy (&RTC, &SRTC, sizeof (RTC));
	rtc_set (&RTC);
	W24H = w24h;

	display (DISPLAY_MWATCH);
	display (DISPLAY_CWATCH);
}

void display_startset () {

	memcpy (&SRTC, &RTC, sizeof (SRTC));
	w24h = W24H;

	WatchBeingSet = 0;
	display (DISPLAY_MWATCH_SET);
	display (DISPLAY_SWATCH_SET);
}

// ============================================================================
// ============================================================================
// ============================================================================

static const display_t views [N_DISPLAYS] = {

	{ NULL, mw_update, mw_show, mw_clear, NULL, 0 },	// Minute watch
	{ sw_show, NULL, sw_show, sw_clear, NULL, 1 },		// Scnds watch
	{ NULL, cw_update, cw_show, cw_clear, NULL, 1 },	// Calendar
	{ NULL, NULL, mwset_show, mwset_clear, mwset_button, 0 },	// Set
	{ NULL, NULL, swset_show, sw_clear, swset_button, 1 },	
	{ NULL, NULL, cwset_show, cw_clear, cwset_button, 1 },
};

static const display_t	*current [N_WINDOWS];

// ============================================================================

void display (byte what) {

	const display_t *new, *old;
	byte where;

	where = (new = views + what) -> where;

	if ((old = current [where]) != new) {
		// We are changing the display; otherwise it is just a refresh
		if (old) {
			watch_dequeue (old->cb_sec);
			watch_dequeue (old->cb_min);
			if (old->fn_clear)
				old->fn_clear ();
		}
		watch_queue (new->cb_sec, YES);
		watch_queue (new->cb_min, NO);
		current [where] = new;
	}

	if (new->fn_show)
		new->fn_show ();

}

Boolean display_button () {

	word i;

	for (i = 0; i < N_WINDOWS; i++)
		if (current [i] && current [i] -> fn_butt &&
		    current [i] -> fn_butt ())
			// Handled
			return YES;
	return NO;
}

byte display_content (byte which) {

	byte res;

	return current [which] == NULL ? BNONE :
		(byte) (current [which] - views);
}
