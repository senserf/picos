/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "pins.h"
#include "netid.h"

#ifndef	__SMURPH__
#include "cc1100.h"
#endif

// Packet format: NetID(w) ser(b) len(b) chars (CRC); +1 is for the sentinel
// byte
#define	FRAME_LENGTH	(2 + 2 + 2)
#define	MAX_LLENGTH	(CC1100_MAXPLEN - FRAME_LENGTH + 1)

// ============================================================================

sint RFD;
byte ISQ, ISQ_N = 1, OSQ, WAKE = 0;

// ============================================================================

static void send_rf (const char *buf, sint len) {

	sint pln, i;
	address pkt;

	if (len <= 0)
		len = strlen (buf);

	if (len > MAX_LLENGTH - 1)
		len = MAX_LLENGTH - 1;

	if ((pln = len + FRAME_LENGTH) & 1)
		pln++;

	for (i = 0; i < NCOPIES; i++) {
		if ((pkt = tcv_wnps (WNONE, RFD, pln, i < WAKE)) == NULL)
			break;
		pkt [1] = OSQ | (len << 8);
		memcpy (pkt + 2, buf, len);
		tcv_endp (pkt);
	}

	if (i)
		OSQ++;
}

static void skspace (char **s) {

	while (isspace (**s))
		(*s)++;
}

fsm uart_reader {

	char UIBuf [82];

	state UR_WAIT:

		char *s;

		ser_in (UR_WAIT, UIBuf, MAX_LLENGTH);
		s = UIBuf;

		skspace (&s);

		for (WAKE = 0; *s == '!'; s++, WAKE++);

		skspace (&s);

		if (*s == '\0')
			sameas UR_WAIT;

		send_rf (s, 0);
		proceed UR_WAIT;
}

fsm root {

	address pkt;

	state RO_INIT:

		word netid;

		tcv_plug (0, &plug_null);
		phys_cc1100 (0, CC1100_MAXPLEN);
		if ((RFD = tcv_open (WNONE, 0, 0)) < 0)
			syserror (EHARDWARE, "RF");
		netid = NETID;
		tcv_control (RFD, PHYSOPT_SETSID, &netid);
		tcv_control (RFD, PHYSOPT_RXON, NULL);
		runfsm uart_reader;

	state RO_WAITRF:

		sint len;
		byte sq, ln;

		pkt = tcv_rnp (RO_WAITRF, RFD);
		len = tcv_left (pkt);

		if (len < 6) {
Drop:
			tcv_endp (pkt);
			sameas RO_WAITRF;
		}

		sq = pkt [1] & 0xFF;
		if (!ISQ_N && sq == ISQ)
			// Duplicate
			goto Drop;

		ln = (pkt [1] >> 8) & 0xFF;
		if (ln > len - 6)
			// Length error
			goto Drop;

		ISQ_N = 0;
		ISQ = sq;

		((char*)(pkt + 2)) [ln] = '\0';

	state RO_UART:

		ser_outf (RO_UART, "%s\r\n", (char*)(pkt + 2));
		tcv_endp (pkt);
		sameas RO_WAITRF;
}
