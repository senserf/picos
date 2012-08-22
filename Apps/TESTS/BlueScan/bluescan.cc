#include "sysio.h"
#include "form.h"
#include "bluescan.h"
#include "board_pins.h"

static const char bscan_init_commands [] =
#ifdef	BT_MODULE_LINKMATIK
	"SET CONTROL ECHO 7\0"
	"SET CONTROL ECHO 7\0"
	"SET CONTROL ECHO 7\0"
	"SET CONTROL AUTOCALL\0"
#endif
#ifdef	BT_MODULE_BTM182
	"ATC0\0"
	"ATX0\0"
	"ATQ0\0"
	"ATR0\0"
	"ATO1\0"
#endif
	    "\0";

static const char bscan_scan_commands [] = 
#ifdef	BT_MODULE_LINKMATIK
	"INQUIRY 12 NAME\0"
#endif
#ifdef	BT_MODULE_BTM182
	"ATF?\0"
	"ATF?\0"
#endif
	    "\0";

bscan_item_t bscan_cache [BSCAN_CACHESIZE];

// ============================================================================

static char ibuf [BSCAN_ILINELENGTH+1];
static word nibuf;
static int heartbeat;
static Boolean (*qual) (const bscan_item_t*);
static const char eol [] = "\r\n";

// ============================================================================

static void rst () {

	blue_reset_set;
	mdelay (10);
	blue_reset_clear;
	mdelay (10);
}

static int scmp (const char *s1, const char *s2) {

	while (*s2 != '\0') {
		if (*s1 != *s2)
			return 0;
		s1++;
		s2++;
	}

	return 1;
}

static void update_cache () {

	bscan_item_t *p;
	byte st;

	for_all_bscan_entries (p) {
		st = p->status;
		if (st && (st & 0x80) == 0) {
			// Neither empty nor pending, decrement
			st--;
			if (st == 0) {
				// expired
				st = 0x80;
				trigger (bscan_cache);
			}
			p->status = st;
		}
	}
}

static void add_cache () {

    bscan_item_t pp;

#ifdef BT_MODULE_LINKMATIK

    {
	const char *s;
	word w, nc;

	if (!scmp (ibuf, "NAME ")) {
		if (scmp (ibuf, "INQUIRY"))
			heartbeat = BSCAN_HBCREDITS;
		return;
	}

	heartbeat = BSCAN_HBCREDITS;

	for (s = ibuf + 5, nc = 0; nc < 6; nc++, s += 3) {
		if (scan (s, "%x", &w) == 0) {
			// Something wrong
			return;
		}
		pp.mac [nc] = (byte) w;
	}

	// Name

	if (*s != '"')
		// Something wrong
		return;

	for (s++, nc = 0; nc < BSCAN_NAMELENGTH; nc++, s++) {
		if (*s == '"')
			break;
		pp.name [nc] = *s;
	}
	pp.name [nc] = '\0';
    }

#endif

#ifdef	BT_MODULE_BTM182

    {
	const char *s;
	word w, v, nc;
	lword wl;

	s = ibuf;
	if (!isdigit (*s)) {
		if (scmp (s, "Inquiry") || scmp (s, " already"))
			heartbeat = BSCAN_HBCREDITS;
		return;
	}

	while (isdigit (*s))
		s++;

	// Find the first character of name
	while (*s == ' ')
		s++;

	// This is the blank immediately preceding MAC
	for (nc = nibuf - 1; ibuf [nc] != ' '; nc--);

	if (scan (ibuf + nc, "%x %x %lx", &w, &v, &wl) != 3)
		// Something wrong
		return;

	heartbeat = BSCAN_HBCREDITS;

	// Last character of name
	while (ibuf [nc] == ' ')
		nc--;

	if ((nc = (ibuf + nc + 1) - s) > BSCAN_NAMELENGTH)
		nc = BSCAN_NAMELENGTH;

	pp.mac [0] = (byte)(w  >>  8);
	pp.mac [1] = (byte)(w       );
	pp.mac [2] = (byte)(v       );
	pp.mac [3] = (byte)(wl >> 16);
	pp.mac [4] = (byte)(wl >>  8);
	pp.mac [5] = (byte)(wl      );

	pp.name [nc] = '\0';
	while (nc--)
		pp.name [nc] = s [nc];
    }
#endif

    {
	bscan_item_t *p, *q;
	word w;

	pp.status = 0x80 + BSCAN_RETENTION;

	// Check if qualified
	if (qual != NULL && !qual (&pp))
		// Not qualified
		return;

	// Search for the MAC in the table
	q = NULL;
	for_all_bscan_entries (p) {
		if (p->status == 0) {
			q = p;
			continue;
		}
		for (w = 0; w < 6; w++)
			if (pp.mac [w] != p->mac [w])
				goto Next;
		// Same mac
		p->status = BSCAN_RETENTION;
		return;
Next:		CNOP;
	}

	if (q == NULL)
		// No room
		return;

	*q = pp;
	trigger (bscan_cache);
    }
}

// ============================================================================

fsm bscan_read {

	state START:

		nibuf = 0;

	state CONTINUE:

		char c;

		io (CONTINUE, UART_B, READ, &c, 1);

		if (c < 32) {
			// Assume anything less than blank ends the line;
			// empty and weird lines will be ignored anyway
			ibuf [nibuf] = '\0';
			add_cache ();
			sameas START;
		}

		if (nibuf < BSCAN_ILINELENGTH)
			ibuf [nibuf++] = c;

		sameas CONTINUE;
}

fsm bscan_out (const char *sp) {

	state LINE_OUT:

		if (*sp == '\0')
			sameas CODA;

		io (LINE_OUT, UART_B, WRITE, (char*)sp, 1);
		sp++;
		savedata (sp);
		sameas LINE_OUT;

	state CODA:

		io (CODA, UART_B, WRITE, (char*)(eol+0), 1);

#ifdef BT_MODULE_LINKMATIK
	state FINE:

		io (FINE, UART_B, WRITE, (char*)(eol+1), 1);
#endif
		finish;
}

// ============================================================================

fsm bscan_scan {

	const char *s;

	state SCAN_START:

		s = bscan_init_commands;

	state SCAN_INIT:

		join (runfsm bscan_out (s), SCAN_NI);
		release;

	state SCAN_NI:

		// Skip the current command
		while (*s++ != '\0');
		if (*s != '\0') {
			delay (128, SCAN_INIT);
			release;
		}

		heartbeat = BSCAN_HBCREDITS;

	state SCAN_LOOP:

		if (heartbeat <= 0) {
			rst ();
			proceed SCAN_START;
		}
		update_cache ();

	state SCAN_CMD:

		s = bscan_scan_commands;

	state SCAN_ENTER:

		join (runfsm bscan_out (s), SCAN_NC);
		release;

	state SCAN_NC:

		// Skip
		while (*s++ != '\0');
		if (*s != '\0') {
			delay (128, SCAN_ENTER);
			release;
		}

		heartbeat--;
		delay (BSCAN_INTERVAL, SCAN_LOOP);
}

// ============================================================================

void bscan_stop () {

	killall (bscan_out);
	killall (bscan_scan);
	killall (bscan_read);

	rst ();
}

void bscan_start (Boolean (*q)(const bscan_item_t*)) {

	bscan_item_t *p;

	bscan_stop ();

	// Zero out cache

	for_all_bscan_entries (p)
		p->status = 0;

	qual = q;

	runfsm bscan_read;
	runfsm bscan_scan;
}
