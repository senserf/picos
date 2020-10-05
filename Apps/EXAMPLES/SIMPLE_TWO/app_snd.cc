/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	60
#define	IBUF_LENGTH		82

// This is a SIMPLE illustration of a multiprogram praxis. We have two node
// types: sender and receiver, i.e., the combined functionality of SIMPLE is
// split. This is the sender.

int 	sfd = -1;		// Session descriptor

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
	// No need to enable receiver
	// tcv_control (sfd, PHYSOPT_RXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

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
