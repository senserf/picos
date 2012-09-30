/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "uart_def.h"

#ifdef	UART_B
#define	LINE_LENGTH	84
#define	UART_BT		UART_B
#else
// Single UART, OSS through RF
#include "tcvphys.h"
#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "ab.h"
// We will try to live with this
#define	LINE_LENGTH	(CC1100_MAXPLEN - 6)
#define	UART_BT		UART_A
#endif

#define	DELAY_CMDI	512		// Command - first verification try
#define	DELAY_CMDV	512		// Ver command - first check
#define	DELAY_VERI	256		// Between verification checks
#define	DELAY_EXIT	1024		// To clean up
#define	DELAY_SAFE	512		// Delay "just in case"
#define	DELAY_POKE	256		// Between poke tries
#define DELAY_RRESET	(1*1024)	// Rate reset
#define	DELAY_RESET	10		// Module reset

#define	TRIES_VERI	6		// Verification checks 256
#define	TRIES_CMDV	3		// Verification commands
#define	TRIES_CMDI	3		// Commands
#define	TRIES_POKE	3		// Number of poke commands

static char bibuf [LINE_LENGTH];	// BT input buffer
static char uibuf [LINE_LENGTH];	// UART input buffer
static char cmbuf [LINE_LENGTH];	// Command buffer (BT output)
static char scbuf [LINE_LENGTH];	// Secondary command buffer
static void (*bt_readf) (const char*);	// Function to interpete BT input
static char *cmdl;			// List of commands to run

// The secondary buffer is used, e.g., for command verification, whereby a
// different command may have to be interjected to check for the effects of
// the original command, which must not be discarded until confirmed

static word nbibuf;			// bibuf fill count
static word rate;			// Current rate index
static word OK;
static sint min_rate_index;

typedef struct o_line_s {
	struct o_line_s *next;
	char line [0];
} o_line_t;

#define	O_LINE_HDRLEN	sizeof (o_line_t*)	// Offset to the string

static o_line_t	*u_o_lines,	// Output lines destined to UART
		*b_o_lines;	// Output lines destined to BT

// Parameters for preset
static word new_rate_index;
static char new_name [17];
static char new_pin [17];

static char *iparse;		// Used by root for parsing input

// ============================================================================

#ifndef	UART_RATE_SETTABLE
#error "System must be compiled with UART_RATE_SETTABLE = 1"
static const word rate_list [] = { 0 };
#else
static const word rate_list [] = UART_RATES_AVAILABLE;
#endif
#define	NRATES (sizeof (rate_list) / sizeof (word))

static const char eol [] = "\r\n";

// ============================================================================

void write_line (const char*, o_line_t**);

#define	wl_u(l)	write_line (l, &u_o_lines);
#define	wl_b(l)	write_line (l, &b_o_lines);

// ============================================================================

static sint rate_index (word r) {

	sint i;

	for (i = 0; i < sizeof (rate_list) / sizeof (word); i++)
		if (rate_list [i] == r)
			return i;
	return -1;
}

// ============================================================================

static Boolean kcmp (const char *s1, const char *s2) {
//
// Checks if keyword s2 occurs in string s1, case ignored
//
	const char *a, *b;

	while (*s1 != '\0') {
		for (a = s1, b = s2; !isspace (*b) && *b != '\0'; a++, b++)
			if (tolower (*a) != tolower (*b))
				goto Next;
		return 1;
Next:
		s1++;
	}
	return 0;
}

// ============================================================================

#ifdef	BT_MODULE_LINKMATIK

// ============================================================================
// Linkmatik ==================================================================
// ============================================================================

#define	bt_rate_available(a) 1

static const char *b_cmd_uart =
	"SET BT CLASS 001f00\0"
	"SET BT LAP 9e8b33\0"
	"SET BT PAIR *\0"
	"SET BT PAGEMODE 4 2000 1\0"
	"SET BT ROLE 0 f 7d00\0"
	"SET BT SNIFF 0 20 1 8\0"
	"SET CONTROL CD 04 0\0"
	"SET CONTROL ECHO 4\0"
	"SET CONTROL ESCAPE 0 00 1\0"
	"SET CONTROL AUTOCALL\0"
	"\0";

static const char *b_cmd_sniff =
	"SET BT CLASS 001f00\0"
	"SET BT LAP 9e8b33\0"
	"SET BT PAIR *\0"
	"SET BT PAGEMODE 4 2000 1\0"
	"SET BT ROLE 0 f 7d00\0"
	"SET BT SNIFF 0 20 1 8\0"
	"SET CONTROL CD 04 0\0"
	"SET CONTROL ECHO 7\0"
	"SET CONTROL ESCAPE 0 00 1\0"
	"SET CONTROL AUTOCALL\0"
	"\0";

static void bt_rate_test (const char *li) {
//
// Determines if there's a valid response from the module for a rate check
//
	// Look for "SYNTAX ERROR"
	if (kcmp (li, "syntax"))
		OK = 1;
}

static void bt_command_test (const char *li) {
//
// Verifies whether one of the "list" command has been accepted
//
	const char *s;

	s = cmbuf;

	if (kcmp (s, "autocall") || kcmp (s, "pair")) {
		// Ignore autocall and pair, which don't echo
		OK = 1;
		return;
	}

	while (1) {
		// Find the beginning of a word
		while (*s != '\0' && !isalnum (*s))
			s++;
		if (*s == '\0') {
			// All matched
			OK = 1;
			return;
		}
		if (!kcmp (li, s))
			// No match
			return;
		// Advance
		while (*s != '\0' && !isspace (*s))
			s++;
	}
}

static void bt_name_command_test (const char *li) {
//
// Verifies the acceptance of the name setting command
//
	if (kcmp (li, "set") && kcmp (li, "name") && kcmp (li, new_name))
		OK = 1;
}

static void bt_pin_command_test (const char *li) {

	if (kcmp (li, "set") && kcmp (li, "auth") && kcmp (li, new_pin))
		OK = 1;
}

static void bt_poke_cmd () {
//
// Create a command to poke the module (used to check if rate is correct)
//
	strcpy (cmbuf, "TE");
}

static void bt_verification_cmd () {
//
// Given the command string (in cmbuf), fills scbuf with a command to poll
// for the effects of that command
//
	// We get all settings fishing for the one we want to verify
	strcpy (scbuf, "SET");
}

static void bt_name_verification_cmd () {
	bt_verification_cmd ();
}

static void bt_pin_verification_cmd () {
	bt_verification_cmd ();
}

static void bt_setrate_cmd () {
	form (cmbuf, "SET CONTROL BAUD %u00,8n1", rate_list [new_rate_index]);
}

static void bt_setname_cmd () {
	form (cmbuf, "SET BT NAME %s", new_name);
}

static void bt_setpin_cmd () {
	form (cmbuf, "SET BT AUTH * %s", new_pin);
}

#endif

#ifdef	BT_MODULE_BTM182

// ============================================================================
// BTM 182 ====================================================================
// ============================================================================

// Only these rates are accessible
const word lrates [] = { 48, 96, 192, 384, 576, 1152, 2304 };

static sint bt_rate_available (word a) {

	sint i;

	for (i = 0; i < sizeof (lrates) / sizeof (word); i++)
		if (lrates [i] == rate_list [a])
			return i + '0';
	return 0;
}

static const char *b_cmd_uart =
	"ATC1\0"
	"ATX0\0"
	"ATE1\0"
	"ATG1\0"
	"ATH1\0"
	"ATK0\0"
	"ATO0\0"
	"ATM0\0"
	"ATO0\0"
	"ATR1\0"
	"ATO0\0"	// Repeat this, as it isn't acked
	"ATS1\0"
	"ATO0\0"
	"ATQ1\0"
	"ATO0\0"
	"\0";

static const char *b_cmd_sniff =
	"ATC1\0"
	"ATX0\0"
	"ATE1\0"
	"ATG1\0"
	"ATH1\0"
	"ATK0\0"
	"ATO1\0"
	"ATM0\0"
	"ATO1\0"
	"ATR0\0"
	"ATO1\0"
	"ATS1\0"
	"ATO1\0"
	"ATQ0\0"
	"ATO1\0"
	"\0";

static void bt_rate_test (const char *li) {
//
// Determines if there's a valid response from the module for a rate check
//
	if (li [0] == '0' || li [0] == '1')
		// I don't know how to make this any more reliable, if we
		// don't know the echo status
		OK = 1;
}

static void bt_command_test (const char *li) {
//
// Verifies whether one of the "list" command has been accepted
//
	const char *s;

	// ATO is not acked, I don't know why
	if (kcmp (cmbuf, "ATO") || li [0] == cmbuf [3])
		OK = 1;
}

static void bt_name_command_test (const char *li) {
//
// Verifies the acceptance of the name setting command
//
	if (kcmp (li, new_name))
		OK = 1;
}

static void bt_pin_command_test (const char *li) {

	if (kcmp (li, new_pin))
		OK = 1;
}

static void bt_poke_cmd () {
//
// Create a command to poke the module (used to check if rate is correct)
//
	strcpy (cmbuf, "ATQ?");
}

static void bt_verification_cmd () {
//
// Given the command string (in cmbuf), fills scbuf with a command to poll
// for the effects of that command
//
	// We get all settings fishing for the one we want to verify
	strcpy (scbuf, cmbuf);
	scbuf [3] = '?';
}

static void bt_name_verification_cmd () {
	strcpy (scbuf, "ATN?");
}

static void bt_pin_verification_cmd () {
	strcpy (scbuf, "ATP?");
}

static void bt_setrate_cmd () {
	form (cmbuf, "ATL%c", bt_rate_available (new_rate_index));
}

static void bt_setname_cmd () {
	form (cmbuf, "ATN=%s", new_name);
}

static void bt_setpin_cmd () {
	form (cmbuf, "ATP=%s", new_pin);
}

#endif

#ifdef	BT_MODULE_BOLUTEK

// ============================================================================
// BC4 BOLUTEK version ========================================================
// ============================================================================

// Accessible rates
const word lrates [] = { 48, 96, 192, 384, 576, 1152, 2304 };

static sint bt_rate_available (word a) {

	sint i;

	for (i = 0; i < sizeof (lrates) / sizeof (word); i++)
		if (lrates [i] == rate_list [a])
			return i + 3 + '0';
	return 0;
}

static const char *b_cmd_uart =
	"AT+ROLE0\0"
	"AT+RESET\0"
	"AT+INQ\0"
	"\0";

static const char *b_cmd_sniff = 
	"AT+ROLE1\0"
	"AT+RESET\0"
	"AT+INQM0,9,30\0"
	"AT+AUTOINQ0\0"
	"AT+INQC\0"
	"\0";

static void bt_rate_test (const char *li) {
//
// Determines if there's a valid response from the module for a rate check
//
	if (kcmp (li, "OK"))
		OK = 1;
}

static void bt_command_test (const char *li) {
//
// Verifies whether one of the "list" command has been accepted
//
	const char *s;

	if (kcmp (cmbuf, "ROLE0")) {
		if (kcmp (li, "ROLE=0"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "ROLE1")) {
		if (kcmp (li, "ROLE=1"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "RESET")) {
		if (kcmp (li, "+READY"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "INQM")) {
		if (kcmp (li, "+INQM=0,9,30"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "AUTOINQ")) {
		if (kcmp (li, "Q0") || kcmp (li, "OK"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "INQC")) {
		if (kcmp (li, "+INQE") || kcmp (li, "304"))
			OK = 1;
		return;
	}

	if (kcmp (cmbuf, "INQ")) {
		if (kcmp (li, "+PAIR") || kcmp (li, "303"))
			OK = 1;
		return;
	}

	bt_rate_test (li);
}

static void bt_name_command_test (const char *li) {
//
// Verifies the acceptance of the name setting command
//
	if (kcmp (li, new_name))
		OK = 1;
}

static void bt_pin_command_test (const char *li) {

	if (kcmp (li, new_pin))
		OK = 1;
}

static void bt_poke_cmd () {
//
// Create a command to poke the module (used to check if rate is correct)
//
	strcpy (cmbuf, "AT");
}

static void bt_verification_cmd () {
//
// Given the command string (in cmbuf), fills scbuf with a command to poll
// for the effects of that command
//
	// The direct response is fine
	scbuf [0] = '\0';
}

static void bt_name_verification_cmd () {
	bt_verification_cmd ();
}

static void bt_pin_verification_cmd () {
	bt_verification_cmd ();
}

static void bt_setrate_cmd () {
	form (cmbuf, "AT+BAUD%c", bt_rate_available (new_rate_index));
}

static void bt_setname_cmd () {
	form (cmbuf, "AT+NAME%s", new_name);
}

static void bt_setpin_cmd () {
	form (cmbuf, "AT+PIN%s", new_pin);
}

static void bt_factory_cmd () {
	strcpy (cmbuf, "AT+DEFAULT");
}

#endif

// ============================================================================

static void bt_echo (const char *il) { wl_u (il); }

// ============================================================================

fsm bt_read {

	Boolean el;

	state START:

		nbibuf = 0;

	state CONTINUE:

		char c;

		io (CONTINUE, UART_BT, READ, &c, 1);

		if (c < 32) {
			if (nbibuf == 0)
				sameas CONTINUE;
			// Assume anything less than blank ends the line;
			// empty and weird lines should be ignored
			if (nbibuf != 0) {
				bibuf [nbibuf] = '\0';
				if (bt_readf != NULL)
					bt_readf (bibuf);
			}
			sameas START;
		}

		if (c <= 0x7E) {
			if (nbibuf < LINE_LENGTH-1)
				bibuf [nbibuf++] = c;
		}

		sameas CONTINUE;
}

// ============================================================================

fsm bt_write {

	char *s;

	state INIT:

		if (b_o_lines == NULL) {
			when (&b_o_lines, INIT);
			release;
		}

		s = b_o_lines->line;

	state WRITE_LINE:

		if (*s == '\0') {
			// Release the line
			o_line_t *o = b_o_lines->next;
			ufree (b_o_lines);
			b_o_lines = o;
			sameas CODA;
		}

		io (WRITE_LINE, UART_BT, WRITE, s, 1);
		s++;
		sameas WRITE_LINE;

	state CODA:

		io (CODA, UART_BT, WRITE, (char*)(eol+0), 1);

#ifdef BT_MODULE_BTM182
		// Don't send \n for BTM-182 if not connected
		if (!blue_ready)
			// Not connected, bypass sending NL
			proceed INIT;
#endif

	state FINE:

		io (FINE, UART_BT, WRITE, (char*)(eol+1), 1);
		proceed INIT;
}

// ============================================================================

#ifdef UART_B

fsm oss_write {
//
// This FSM writes to UART_B
//
	char *s;

	state INIT:

		if (u_o_lines == NULL) {
			when (&u_o_lines, INIT);
			release;
		}

		s = u_o_lines->line;

	state WRITE_LINE:

		if (*s == '\0') {
			// Release the line
			o_line_t *o = u_o_lines->next;
			ufree (u_o_lines);
			u_o_lines = o;
			sameas CODA;
		}

		io (WRITE_LINE, UART_A, WRITE, s, 1);
		s++;
		sameas WRITE_LINE;

	state CODA:

		io (CODA, UART_A, WRITE, (char*)(eol+0), 1);

	state FINE:

		io (FINE, UART_A, WRITE, (char*)(eol+1), 1);

		proceed INIT;
}

#define	oss_read(st,buf)	ser_in (st, buf, LINE_LENGTH)

#else

fsm oss_write {
//
// This FSM writes to an AP over the wireless link
//
	char *s;

	state INIT:

		char *q;

		if (u_o_lines == NULL) {
			when (&u_o_lines, INIT);
			release;
		}

		// Release the line
		s = (char*) u_o_lines;
		u_o_lines = u_o_lines -> next;
		// Shift the line down, so the string begins at the first byte
		// of the structure, so we can pass it directly to ab_out, so
		// it will deallocate it properly
		for (q = s; (*q = *(q + O_LINE_HDRLEN)) != '\0'; q++);

	state SEND:

		ab_out (SEND, s);
		sameas INIT;
}

void oss_read (word st, char *buf) {
//
// Read from an AP
//
	char *s;

	s = ab_in (st);
	strcpy (buf, s);
	ufree (s);
}
	
#endif

// ============================================================================

void write_line (const char *line, o_line_t **q) {

	sint n, m;
	o_line_t *newl, *p;

	m = (n = strlen (line)) + 1 + sizeof (o_line_t);

	if ((newl = (o_line_t*) umalloc (m)) == NULL) {
		diag ("OOM!");
		return;
	}

	strcpy (newl->line, line);
	newl->next = NULL;

	if (*q == NULL) {
		*q = newl;
		trigger (q);
		return;
	}

	for (p = *q; p->next != NULL; p = p->next);
	p->next = newl;
}

// ============================================================================
// ============================================================================

fsm bt_command {
//
// Issue a command persistently until its effects are confirmed
//
	word cmdtries, vertries, chktries;

	state INIT:

		wl_u (cmbuf);
		cmdtries = 0;
		OK = 0;

	state ISSUE:

		// Issue the command
		wl_b (cmbuf);
		cmdtries++;
		// Delay a bit
		delay (DELAY_CMDI, ISSUED);
		release;

	state ISSUED:

		// Verification counter
		vertries = 0;

	state VERIFY:

		// Verification command
		if (*scbuf != '\0') {
			wl_b (scbuf);
			vertries++;
		} else {
			vertries = TRIES_CMDV;
		}
		// Delay another bit
		delay (DELAY_CMDV, VERCHK);
		release;

	state VERCHK:

		chktries = 0;

	state CHKLOOP:

		chktries++;

		if (OK) {
			wl_u ("-> OK");
			delay (DELAY_EXIT, DONE);
			release;
		}

		if (chktries >= TRIES_VERI) {
			// Failed, reissue verification command
			if (vertries >= TRIES_CMDV) {
				// Failed, reissue command
				if (cmdtries >= TRIES_CMDI) {
					// Failure
					wl_u ("-> Failed");
					delay (DELAY_EXIT, DONE);
					release;
				}
				proceed ISSUE;
			}
			proceed VERIFY;
		}
		delay (DELAY_VERI, CHKLOOP);
		release;

	state DONE:

		finish;
}

// ============================================================================

fsm bt_check_response {
//
// Checks if the module responds at all (used to verify that the rate is OK)

	word waitcount;

	state INIT:

		OK = 0;
		bt_readf = bt_rate_test;
		waitcount = TRIES_POKE * 4;
		bt_poke_cmd ();

	state ISSUE_LOOP:

		// Issue the poll every fourth turn
		if ((waitcount & 3) == 0)
			wl_b (cmbuf);

		delay (DELAY_POKE, ISSUE_CHECK);
		release;

	state ISSUE_CHECK:

		if (OK || waitcount == 1) {
			// Done, the caller will see the OK flag
			delay (DELAY_SAFE, DONE);
			release;
		}

		waitcount--;
		proceed ISSUE_LOOP;

	state DONE:

		finish;
}

// ============================================================================

fsm bt_findrate {

	word waitcount;

	state INIT:

		// Reset the module
		blue_reset_set;
		mdelay (10);
		blue_reset_clear;
		mdelay (40);

		OK = 0;

		for (rate = min_rate_index;
			!bt_rate_available (rate) && rate < NRATES; rate++);

		if (rate == NRATES) {
			wl_u ("No rates available for module");
			finish;
		}

	state NEXT_RATE:

		delay (DELAY_SAFE, NEXT_TRY);
		release;

	state NEXT_TRY:

		form (uibuf, "TRYING %d00 bps ...", rate_list [rate]);
		wl_u (uibuf);

		ion (UART_BT, CONTROL, (char*)(rate_list + rate),
			UART_CNTRL_SETRATE);

		delay (DELAY_SAFE, CHECK_RESPONSE);
		release;

	state CHECK_RESPONSE:

		join (runfsm bt_check_response, VERIFY);
		release;

	state VERIFY:

		if (OK) {
			form (uibuf, "RATE IS %d00", rate_list [rate]);
			wl_u (uibuf);
		} else {
			while (++rate < NRATES) {
				if (bt_rate_available (rate))
					proceed NEXT_RATE;
			}
			wl_u ("NO RESPONSE");
		}

		// To get rid of any residual noise
		delay (DELAY_SAFE, DONE);
		release;

	state DONE:

		finish;
}

// ============================================================================

fsm bt_preset {

	Boolean fdone;

	state INIT:

		fdone = NO;
		wl_u ("Resetting the module ...");
		blue_reset_set;
		delay (DELAY_RESET, RESET_DONE);
		release;

	state RESET_DONE:

		blue_reset_clear;
CRate:
		wl_u ("Checking the rate at which module responds ...");
		join (runfsm bt_findrate, B_RATE_FOUND);
		release;

	state B_RATE_FOUND:

		if (OK == 0) {
NoResp:
			wl_u ("Preset failed, no response");
			finish;
		}

#ifdef BT_MODULE_BOLUTEK

		if (fdone)
			goto FactoryDone;

		// Need to reset to factory defaults (which may change the
		// rate)

		wl_u ("Resetting to factory defaults ...");
		bt_factory_cmd ();
		bt_readf = bt_command_test;
		OK = 0;
		wl_b (cmbuf);
		delay (DELAY_SAFE, B_CHECK_FACTORY);
		release;

	state B_CHECK_FACTORY:

		if (!OK)
			// Failed, try again from scratch
			proceed INIT;

		fdone = YES;
		goto CRate;

FactoryDone:

#endif
		if (new_rate_index != rate) {
			// Need to reset rate
			form (uibuf, "Changing rate to %d00",
				rate_list [new_rate_index]);
			wl_u (uibuf);
			bt_setrate_cmd ();
			wl_b (cmbuf);
			delay (DELAY_SAFE, B_NEWRATE);
			release;
		}

	state B_PRESET:

		// The "list" commands
		OK = 0;
		bt_readf = bt_name_command_test;
		bt_setname_cmd ();
		bt_name_verification_cmd ();
		join (runfsm bt_command, B_PINCMD);
		release;

	state B_PINCMD:

		if (!OK) {
			wl_u ("Cannot set name");
			finish;
		}
		bt_readf = bt_pin_command_test;
		bt_setpin_cmd ();
		bt_pin_verification_cmd ();
		join (runfsm bt_command, B_LIST);
		release;

	state B_LIST:

		// Running commands from the list
		bt_readf = bt_command_test;

	state B_MORE:

		if (*cmdl == '\0') {
			wl_u ("Preset OK");
			finish;
		}
		strcpy (cmbuf, cmdl);
		bt_verification_cmd ();
		join (runfsm bt_command, B_MORELOOP);
		release;

	state B_MORELOOP:

		cmdl += strlen (cmdl) + 1;
		proceed B_MORE;

	state B_NEWRATE:

		// Do it one more time
		wl_b (cmbuf);
		delay (DELAY_SAFE, B_NEWRATE_AGAIN);
		release;

	state B_NEWRATE_AGAIN:

		rate = new_rate_index;

		ion (UART_BT, CONTROL, (char*)(rate_list + rate),
			UART_CNTRL_SETRATE);

		join (runfsm bt_check_response, B_NEWRATE_CHECK);
		release;

	state B_NEWRATE_CHECK:

		if (OK) {
			wl_u ("Rate change OK");
			proceed B_PRESET;
		}

		wl_u ("Preset failed, can't change rate");
		finish;
}

// ============================================================================

static sint skipb () {
	while (isspace (*iparse)) iparse++;
	return *iparse;
}

static sint skipnb () {
	while (!isspace (*iparse) && *iparse != '\0') iparse++;
	return *iparse;
}

sint scanstr (char *t, sint len) {

	if (skipb () == '\0')
		return 0;

	while (len && !isspace (*iparse)) {
		*t++ = *iparse++;
		len--;
	}
	*t = '\0';

	skipnb ();
	return 1;
}

// ============================================================================

fsm root {

	sint n, k, p;

#ifndef	UART_B
	sint sfd = -1;
#endif

  state RS_INI:

#ifndef	UART_B

	phys_cc1100 (0, CC1100_MAXPLEN);

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	n = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, (address)(&n));
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	ab_init (sfd);
	ab_mode (AB_MODE_PASSIVE);
#endif
	ion (UART_BT, CONTROL, (char*)(&n), UART_CNTRL_GETRATE);
	// Do not use rates below 9600
	if ((min_rate_index = rate_index (96)) < 0) {
		wl_u ("Rate 9600 is not available!");
		// Wait forever
		when (&min_rate_index, RS_INI);
		release;
	}

	rate = rate_index (n);

	runfsm bt_read;
	runfsm bt_write;
	runfsm oss_write;

	bt_readf = bt_echo;

  state RS_MEN:

	wl_u (" ");
#ifdef BT_MODULE_LINKMATIK
	wl_u ("BT LinkMatik: test and maintanance");
#endif
#ifdef BT_MODULE_BTM182
	wl_u ("BT BTM-182: test and maintanance");
#endif
#ifdef BT_MODULE_BOLUTEK
	wl_u ("BT BC4 (BOLUTEK): test and maintanance");
#endif
	wl_u ("Commands:");
	wl_u ("w string  > write line to module");
	wl_u ("r rate    > set rate for module");
#ifdef	UART_B
	wl_u ("t rate    > set rate for UART");
#endif
	wl_u ("e [|0|1]  > escape");
	wl_u ("a         > reset");
	wl_u ("s         > view status flag");
	wl_u ("D         > auto discover rate");
	wl_u ("S r n p m > preset rate name pin mode=uart|sniffer");
	wl_u ("p [0|1]   > power (down|up)");

  state RS_RCM:

	oss_read (RS_RCM, uibuf);

	switch (uibuf [0]) {

	    case 'w': proceed RS_WRI;
	    case 'r': proceed RS_RAT;
#ifdef	UART_B
	    case 't': proceed RS_RAU;
#endif
	    case 'e': proceed RS_ESC;
	    case 'a': proceed RS_RES;
	    case 's': proceed RS_ATT;
	    case 'p': proceed RS_POW;
	    case 'D': proceed RS_DIS;
	    case 'S': proceed RS_PRE;

	}

  state RS_ERR:

	wl_u ("Illegal command or parameter");
	proceed RS_MEN;

  state RS_WRI:

	for (k = 1; uibuf [k] == ' '; k++);
	wl_b (uibuf + k);
	proceed RS_RCM;

  state RS_RAT:

	n = 96;
	scan (uibuf + 1, "%d", &n);
	if ((k = rate_index (n)) < min_rate_index || !bt_rate_available (k))
		proceed RS_ERR;
	ion (UART_BT, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	rate = k;
	proceed RS_RCM;

#ifdef	UART_B
  state RS_RAU:

	n = 96;
	scan (uibuf + 1, "%d", &n);
	if ((k = rate_index (n)) < min_rate_index)
		proceed RS_ERR;
	ion (UART_A, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed RS_RCM;
#endif

  state RS_ESC:

	n = -1;
	scan (uibuf + 1, "%d", &n);
	if (n == 0)
		goto EClear;

	blue_escape_set;
	if (n == -1)
		// Hold
		proceed RS_ATT;

	if (n == 1) {
		// Standard pulse
		mdelay (10);
EClear:
		blue_escape_clear;
		proceed RS_RCM;
	}
	// Factory reset
	while (n--)
		mdelay (1000);

	blue_escape_clear;
	// Say something
	proceed RS_ATT;

  state RS_RES:

	blue_reset_set;
	mdelay (10);
	blue_reset_clear;
	proceed RS_RCM;

  state RS_POW:

	n = 1;
	scan (uibuf + 1, "%d", &n);
	if (n)
		blue_power_up;
	else
		blue_power_down;
	proceed RS_RCM;

  state RS_ATT:

	form (uibuf, "STATUS = %d", blue_ready);
	wl_u (uibuf);
	proceed RS_RCM;

  state RS_DIS:

	wl_u ("Scanning for bit rate");
	join (runfsm bt_findrate, RS_DID_DONE);
	release;

  state RS_DID_DONE:

	bt_readf = bt_echo;
	proceed RS_RCM;

  state RS_PRE:

	n = 0;
	scan (uibuf + 1, "%d", &n);
	if ((k = rate_index (n)) < min_rate_index || !bt_rate_available (k))
		proceed RS_ERR;

	new_rate_index = k;

	iparse = uibuf + 1;
	skipb ();
	skipnb ();

	if (scanstr (new_name, 16) == 0)
		proceed RS_ERR;

	if (scanstr (new_pin, 16) == 0)
		proceed RS_ERR;

	p = skipb ();

	if (p == 'u')
		cmdl = (char*) b_cmd_uart;
	else if (p == 's')
		cmdl = (char*) b_cmd_sniff;
	else
		proceed RS_ERR;

	// Validate PIN
	for (k = 0; isdigit (new_pin [k]); k++);
	if (new_pin [k] != '\0')
		proceed RS_ERR;

	form (uibuf, "RATE: %d00 [%d], name: %s, pin: %s, mode: %c",
		n, new_rate_index, new_name, new_pin, p);

	wl_u (uibuf);

#ifdef BT_MODULE_BTM182

	wl_u ("Resetting to factory rate ...");
	blue_escape_set;
	delay (DELAY_RRESET, RS_PRERS);
	release;

  state RS_PRERS:

	blue_escape_clear;
	wl_u ("OK");
#endif
	join (runfsm bt_preset, RS_DID_DONE);
	release;
}
