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

static int 	sfd = -1;

// ============================================================================

static word Count;

void show (word st, address pkt) {

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
	if ((k = strlen (str)) > MAX_PACKET_LENGTH - 5) {
		str [MAX_PACKET_LENGTH - 4] = '\0';
		k = MAX_PACKET_LENGTH - 5;
	}

	return (k + 6) & 0xfe;
}

fsm sender (char *msg) {

  address spkt;

  state SN_SEND:

	spkt = tcv_wnp (SN_SEND, sfd, plen (msg));
	spkt [0] = 0;
	strcpy ((char*)(spkt + 1), msg);
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
		"\r\nRF cmds:\r\n"
		"s string  -> send the string in a packet\r\n"
		"m x r     -> xmt [-1|0=off|1=hold|2=on] rcv [-1|0|1]\r\n"
	);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH-1);

	if (ibuf [0] == 's')
		proceed RS_XMIT;
	if (ibuf [0] == 'm')
		proceed RS_RFSTAT;

  state RS_RCMD_E:

	ser_out (RS_RCMD_E, "Illegal command\r\n");
	proceed RS_RCMD_M;

  state RS_XMIT:

	runfsm sender (ibuf + 1);
	proceed RS_RCMD;

  state RS_RFSTAT:

	int x, r;

	x = r = 3;

	scan (ibuf + 1, "%u %d", &x, &r);
	if (r > 2 || x > 2 || r < -1 || x < -1)
		proceed RS_RCMD_E;

	switch (x) {
		case 0 : 
			tcv_control (sfd, PHYSOPT_TXOFF, NULL);
			break;
		case 1 :
			tcv_control (sfd, PHYSOPT_TXHOLD, NULL);
			break;
		case 2 :
			tcv_control (sfd, PHYSOPT_TXON, NULL);
			break;
	}

	switch (r) {
		case 0 : 
			tcv_control (sfd, PHYSOPT_RXOFF, NULL);
			break;
		case 1 :
		case 2 :
			tcv_control (sfd, PHYSOPT_RXON, NULL);
			break;
	}

	proceed RS_RCMD;
}
