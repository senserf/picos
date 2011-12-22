/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

int 	sfd = -1;		// Session descriptor

// ============================================================================
// This is a simple illustration of radio communication via NULL the plugin
// ============================================================================

void show (word st, address pkt) {

	static word Count = 0;

	ser_outf (st, "RCV: %d [%s] pow = %d qua = %d\r\n",
		Count++,
		(char*)(pkt + 1),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
}

fsm receiver {

  address rpkt;

  state RC_TRY:

	rpkt = tcv_rnp (RC_TRY, sfd);

  state RC_SHOW:

	show (RC_SHOW, rpkt);
	tcv_endp (rpkt);
	proceed RC_TRY;
}

// ============================================================================

word plen (char *str) {

	word k;

	// The total length of the packet is string length + 2 bytes for the
	// header (Network ID) + 2 bytes for CRC, with the whole thing rounded
	// up to an even length

	// This is how many bytes we need for the string
	k = strlen (str) + 1;

	// Round up to even
	if (k & 1)
		k++;

	if (k > MAX_PACKET_LENGTH - 4) {
		// Too long, must truncate the string
		str [MAX_PACKET_LENGTH - 4] = '\0';
		return MAX_PACKET_LENGTH;
	}

	return k + 4;
}

fsm sender (char *arg) {

  state SN_SEND:

	address spkt;

	spkt = tcv_wnp (SN_SEND, sfd, plen (arg));
	spkt [0] = 0;
	strcpy ((char*)(spkt + 1), arg);
	tcv_endp (spkt);
	finish;
}

// ============================================================================

fsm root {

  char *ibuf;

  state RS_INIT:

	ibuf = (char*) umalloc (IBUF_LENGTH);
	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	runfsm receiver;

  state RS_RCMD_M:

	ser_out (RS_RCMD_M,
		"\r\nRF S-R example\r\n"
		"Command:\r\n"
		"s string  -> send the string in a packet\r\n"
	);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH);

	if (ibuf [0] == 's')
		proceed RS_XMIT;

  state RS_RCMD_E:

	ser_out (RS_RCMD_E, "Illegal command\r\n");
	proceed RS_RCMD_M;

  entry RS_XMIT:

	runfsm sender (ibuf+1);
	proceed RS_RCMD;
}
