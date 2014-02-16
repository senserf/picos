#ifndef	__pg_display_h
#define	__pg_display_h

#include "sysio.h"
#include "watch.h"

typedef	struct {
	time_callback_f	cb_sec;
	time_callback_f	cb_min;
	void	(*fn_show) ();
	void	(*fn_clear) ();
	Boolean	(*fn_butt) ();
	byte	where;
} display_t;

// ============================================================================

#define	N_DISPLAYS		6
#define	N_WINDOWS		2

#define	DISPLAY_MWATCH		0
#define	DISPLAY_SWATCH		1
#define	DISPLAY_CWATCH		2
#define	DISPLAY_MWATCH_SET	3
#define	DISPLAY_SWATCH_SET	4
#define	DISPLAY_CWATCH_SET	5

void display (byte);
void display_setwatch ();
void display_startset ();
Boolean display_button ();
byte display_content (byte);

//+++ "display.cc"

#endif
