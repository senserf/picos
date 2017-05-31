#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "max30102.h"

#define	SAMPFREQ	100

// Parameters
typedef	struct {

	char name [5];
	word min, max;
	word *pp;

} param_t;

word	par_irsd = 2,	// Sample damping "k" [EMA shift]
	par_irod = 1,	// Old value damping "k"
	par_deld = 12,	// Delta for "down" turn detection
	par_delu = 12,	// Delta for "up" turn detection
	par_plsd = 3;	// Pulse damping "k"
	
param_t	Params [] = {
		{ "irsd", 1,  16, &par_irsd },
		{ "irod", 1,  16, &par_irod },
		{ "deld", 0, 256, &par_deld },
		{ "delu", 0, 256, &par_delu },
		{ "plsd", 0,  16, &par_plsd },
	};


lword	re, ir;

lword	se_irs,		// Damped sample
	se_iro,		// Damped last (old) sample
	se_plr;

lword	sslp;		// Samples since last peak

byte	dir;		// 0-down, 1-up

// ============================================================================

static inline void sema_update (lword *s, word k, lword v) {
	*s = *s + v - (*s >> k);
}

static inline lword sema_get (lword s, word k) {
	return s >> k;
}

// ============================================================================

fsm show {

	state WAIT_DATA_READY:

		when (show, DISPLAY_DATA);
		release;

	state DISPLAY_DATA:

		lword ss;

		if ((ss = sema_get (se_plr, par_plsd)) != 0)
			ser_outf (DISPLAY_DATA,
				"T: %lu, R: %u\r\n",
					seconds (),
					(word)((60 * SAMPFREQ) / ss));
		sameas WAIT_DATA_READY;
}

// ============================================================================

static void change_direction () {

	if (dir == 0) {
		// Peak
		if (sslp < 5 * SAMPFREQ) {
			sema_update (&se_plr, par_plsd, sslp);
			trigger (show);
		}
		sslp = 0;
	}
}

static void update () {

	lword sirs, siro;

	sslp++;

	sema_update (&se_irs, par_irsd, ir);

	sirs = sema_get (se_irs, par_irsd);
	siro = sema_get (se_iro, par_irod);

	if (dir) {
		// Current direction == up
		if (sirs < siro - par_deld) {
			dir = 0;
			change_direction ();
		}
	} else {
		// Current direction == down
		if (sirs > siro + par_delu) {
			dir = 1;
			change_direction ();
		}
	}

	// Update old value
	sema_update (&se_iro, par_irod, sirs);
}

// ============================================================================

fsm collect {

	state START_COLLECTION:

		sslp = 0;
		se_plr = SAMPFREQ << par_plsd;

	state NEXT_SAMPLE:

		max30102_read_sample (NEXT_SAMPLE, &re, &ir);
		update ();
		sameas NEXT_SAMPLE;
}

// ============================================================================

fsm root {

	word par, val;

	state INIT:

		runfsm show;

	state MENU:

		ser_out (MENU,
			"Commands:\r\n"
			"  o [0|1]\r\n"
			"  r nx\r\n"
			"  w nx vx\r\n"
			"  c [0|1]\r\n"
			"  p [pname [val]]\r\n"
		);

	state INPUT:

		char cmd [16];

		ser_in (INPUT, cmd, 16);

		if (cmd [0] == 'o' || cmd [0] == 'c') {
			par = 1;
			scan (cmd + 1, "%u", &par);
			if (cmd [0] == 'o') {
				if (par)
					max30102_start (0);
				else
					max30102_stop ();
			} else {
				if (par) {
					if (!running (collect))
						runfsm collect;
				} else {
					killall (collect);
				}
			}

			proceed SAYOK;
		} else if (cmd [0] == 'r') {
			// Register read
			par = WNONE;
			scan (cmd + 1, "%x", &par);
			if (par <= 0xff) {
				val = max30102_rreg ((byte)par);
				proceed OUTREG;
			}
		} else if (cmd [0] == 'w') {
			// Register write
			par = WNONE;
			val = 0;
			scan (cmd + 1, "%x %x", &par, &val);
			if (par <= 0xfd) {
				max30102_wreg ((byte)par, (byte)val);
				proceed OUTREG;
			}
		} else if (cmd [0] == 'p') {
			if (cmd [1] == '\0')
				proceed SHOWPARS;
			for (par = 0; par < sizeof (Params) / sizeof (param_t);
			     par++) {
				if (strncmp (Params [par] . name, cmd + 2, 4)
				    == 0)
					break;
			}
			if (par == sizeof (Params) / sizeof (param_t))
				proceed BAD_INPUT;
			if (cmd [6] == '\0')
				proceed SHOWPAR;
			// Set parameter
			val = WNONE;
			scan (cmd + 6, "%u", &val);
			if (val < Params [par] . min ||
			    val > Params [par] . max)
				proceed BAD_INPUT;
			*(Params [par] . pp) = val;
			proceed SAYOK;
		}

	state BAD_INPUT:

		ser_out (BAD_INPUT, "Illegal input\r\n");
		proceed MENU;

	state SAYOK:

		ser_out (SAYOK, "OK\r\n");
		proceed INPUT;

	state OUTREG:

		ser_outf (OUTREG, "Reg [%x] = %x\r\n", par, val);
		proceed INPUT;

	state SHOWPARS:

		par = 0;

	state SHOW_NEXT_PAR:

		ser_outf (SHOW_NEXT_PAR, "%s = %u\r\n",
			Params [par] . name,
			*(Params [par] . pp));

		if (++par == sizeof (Params) / sizeof (param_t))
			proceed INPUT;
		else
			proceed SHOW_NEXT_PAR;

	state SHOWPAR:

		ser_outf (SHOWPAR, "%s = %u\r\n",
			Params [par] . name,
			*(Params [par] . pp));

		proceed INPUT;
}
