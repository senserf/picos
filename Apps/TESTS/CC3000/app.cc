/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "plug_null.h"
#include "phys_cc1100.h"
#include "phys_cc3000.h"
#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82

cc3000_wlan_params_t WP = {
	CC3000_POLICY_DONT,
	CC3000_WLAN_SEC_WPA,
	5,
	"rybkababadeadba"
};

cc3000_server_params_t SP = { { 192, 168, 1, 16 }, 2234 };

word maxplen = 128;

char ibuf [IBUFLEN];

Boolean inited = NO;

int SFD;

static word fstring (address from) {

	word until;

	while (isspace (ibuf [*from]))
		(*from)++;

	for (until = *from; ibuf [until] != '\0'; until++);
	while (until && isspace (ibuf [until-1]))
		until--;

	if (until <= *from)
		return 0;

	return until - *from;
}

// ============================================================================

char lbuf [3*16+2];
address rpkt;

static Boolean dumpline () {

	char *lpt;
	byte b;
	word i;

	lpt = lbuf;

	for (i = 0; i < 16; i++) {
		if (tcv_read (rpkt, &b, 1) == 0) {
			if (i == 0)
				return YES;
			break;
		}
		if (i)
			*lpt++ = ' ';
		*lpt++ = HEX_TO_ASCII (b >> 4);
		*lpt++ = HEX_TO_ASCII (b);
	}

	*lpt++ = '\r';
	*lpt++ = '\n';
	*lpt++ = '\0';

	return NO;
}

fsm reader {

	state RD_WAIT:

		word len;

		rpkt = tcv_rnp (RD_WAIT, SFD);

	state RD_SHOW:

		ser_outf (RD_SHOW, "RCV: %u\r\n", tcv_left (rpkt));

	state RD_DUMP:

		if (dumpline ()) {
			tcv_endp (rpkt);
			proceed RD_WAIT;
		}

	state RD_DUMP_OUT:

		ser_out (RD_DUMP_OUT, lbuf);
		sameas RD_DUMP;
}
		
fsm root {

	word w [5];

	state RS_MEM:

		w [0] = stackfree ();
		w [1] = memfree (0, w + 2);
		w [3] = maxfree (0, w + 4);

	state RS_MEM_SHOW:

		ser_outf (RS_MEM_SHOW, "MEM: %u %u %u %u %u\r\n",
			w [0], w [1], w [2], w [3], w [4]);

	state RS_LOOP:

		ibuf [0] = '\0';
		ser_in (RS_LOOP, ibuf, IBUFLEN-1);
		switch (ibuf [0]) {

			// Init the driver
			case 'i' : proceed RS_INIT;
			// Change connection parameter
			case 'p' : proceed RS_PARA;
			case 'r' : reset ();
			case 'm' : proceed RS_MEM;
			// Send a packet
			case 's' : proceed RS_SEND;
			// Report driver status
			case 'd' : proceed RS_STATUS;
			case 'o' : proceed RS_ONOFF;
			case 'l' : proceed RS_SENDLOOP;
			case 'c' : proceed RS_COMMAND;

		}

	state RS_ILL:

		ser_out (RS_ILL, "Illegal\r\n");
		proceed RS_LOOP;

	state RS_STATUS:

		if (!inited)
			proceed RS_ILL;

		w [0] = tcv_control (SFD, PHYSOPT_STATUS, w + 1);

	state RS_STATUS_SHOW:

		ser_outf (RS_STATUS_SHOW, "ST: %u %u %u %u %u\r\n",
			w [1] & 0xFF, w [1] >> 8, w [2], w [3], w [0]);
		proceed RS_LOOP;

	state RS_PARA:

		switch (ibuf [1]) {

			case 's' : proceed RS_PARA_SHOW;
			case 'n' : proceed RS_PARA_NETID;
			case 'm' : proceed RS_PARA_POLICY;
			case 'q' : proceed RS_PARA_SECURITY;
			case 'l' : proceed RS_PARA_SIDLEN;
			case 'a' : proceed RS_PARA_IP;
			case 'p' : proceed RS_PARA_PORT;
			case 'b' : proceed RS_PARA_BUFF;
		}

		proceed RS_ILL;

	state RS_PARA_SHOW:

		ser_outf (RS_PARA_SHOW, "NE: %u %u %u %s %u\r\n",
			WP.policy, WP.security_mode, WP.ssid_length,
				WP.ssid_n_key, maxplen);

	state RS_PARA_SHOW_MORE:

		ser_outf (RS_PARA_SHOW_MORE, "SE: %u.%u.%u.%u %u\r\n",
			SP.ip [0], SP.ip [1], SP.ip [2], SP.ip [3], SP.port);

		proceed RS_LOOP;

	state RS_PARA_NETID:

		w [1] = 2;
		w [0] = fstring (w + 1);

		if (w [0] == 0 || w [0] > 31)
			proceed RS_ILL;

		strncpy (WP.ssid_n_key, ibuf + w [1], w [0]);
		WP.ssid_n_key [w [0]] = '\0';
		proceed RS_PARA_SHOW;

	state RS_PARA_POLICY:

		w [0] = WNONE;
		scan (ibuf + 2, "%u", w + 0);
		if (w [0] > 5)
			proceed RS_ILL;
		WP.policy = w [0];
		proceed RS_PARA_SHOW;

	state RS_PARA_SECURITY:

		w [0] = WNONE;
		scan (ibuf + 2, "%u", w + 0);
		if (w [0] > 3)
			proceed RS_ILL;
		WP.security_mode = w [0];
		proceed RS_PARA_SHOW;
		
	state RS_PARA_SIDLEN:

		w [0] = WNONE;
		scan (ibuf + 2, "%u", w + 0);
		if (w [0] > strlen (WP.ssid_n_key))
			proceed RS_ILL;
		WP.ssid_length = w [0];
		proceed RS_PARA_SHOW;

	state RS_PARA_PORT:

		w [0] = WNONE;
		scan (ibuf + 2, "%u", w + 0);
		SP.port = w [0];
		proceed RS_PARA_SHOW;

	state RS_PARA_BUFF:

		w [0] = WNONE;
		scan (ibuf + 2, "%u", w + 0);
		if (w [0] > 1024)
			proceed RS_ILL;
		maxplen = w [0];
		proceed RS_PARA_SHOW;

	state RS_PARA_IP:

		int i;

		if (scan (ibuf + 2, "%u %u %u %u", w + 0, w + 1, w + 2, w + 3)
		    < 4)
			proceed RS_ILL;
		for (i = 0; i < 4; i++) {
			if (w [i] > 255)
				proceed RS_ILL;
			SP.ip [i] = (byte) (w [i]);
		}
		proceed RS_PARA_SHOW;

	state RS_INIT:

		if (inited)
			proceed RS_ILL;

		phys_cc3000 (0, maxplen, &SP, &WP);
		tcv_plug (0, &plug_null);
		SFD = tcv_open (NONE, 0, 0);
		if (SFD < 0)
			proceed RS_INIT_F;

		inited = YES;

		if (runfsm reader == 0)
			proceed RS_INIT_G;

		proceed RS_STATUS;

	state RS_INIT_F:

		ser_out (RS_INIT_F, "Open failed\r\n");
		proceed RS_LOOP;

	state RS_INIT_G:

		ser_out (RS_INIT_G, "Fork failed\r\n");
		proceed RS_LOOP;

	state RS_ONOFF:

		if (!inited)
			proceed RS_ILL;

		if (ibuf [1] == 'n')
			tcv_control (SFD, PHYSOPT_RXON, NULL);
		else if (ibuf [1] == 'f')
			tcv_control (SFD, PHYSOPT_RXOFF, NULL);
		else
			proceed RS_ILL;

		proceed RS_LOOP;

	state RS_SEND:

		address pkt;

		if (!inited)
			proceed RS_ILL;

		w [1] = 1;
		w [0] = fstring (w + 1);

		if ((pkt = tcv_wnp (WNONE, SFD, w [0] + 1)) == NULL)
			proceed RS_SEND_NM;

		strncpy ((char*)pkt, ibuf + w [1], w [0]);
		((char*)pkt) [w [0]] = '\0';

		tcv_endp (pkt);

	state RS_SEND_D:
Done:
		ser_out (RS_SEND_D, "Done\r\n");
		proceed RS_LOOP;

	state RS_SEND_NM:

		ser_out (RS_SEND_NM, "No mem!\r\n");
		proceed RS_LOOP;

	state RS_COMMAND:

		// A placeholder
		proceed RS_ILL;

	state RS_SENDLOOP:

		if (!inited)
			proceed RS_ILL;

		w [1] = 1;
		w [0] = fstring (w + 1);
		w [2] = 1;

	state RS_SENDLOOS:

		form (ibuf + w [0] + w [1], "%u", w [2]);
		w [3] = strlen (ibuf + w [1]);

	state RS_SENDLOOQ:

		ser_outf (RS_SENDLOOQ, "SND: %s\r\n", ibuf + w [1]);

	state RS_SENDLOOR:

		address pkt;

		pkt = tcv_wnp (RS_SENDLOOR, SFD, w [3] + 1);
		strcpy ((char*)pkt, ibuf + w [1]);
		tcv_endp (pkt);

		w [2]++;
		delay (5192, RS_SENDLOOS);
}
