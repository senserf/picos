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

// This is a SIMPLE illustration of a multiprogram praxis. We have two node
// types: sender and receiver, i.e., the combined functionality of SIMPLE is
// split. This is the sender.

// ============================================================================
// This is a simple illustration of radio communication via NULL the plugin
// ============================================================================

void show (word st, address pkt) {

	static word Count = 0;

	ser_outf (st, "RCV: %d [%s] pow = %d qua = %d\r\n",
		Count,
		(char*)(pkt + 1),
		((byte*)pkt) [tcv_left (pkt) - 1],
		((byte*)pkt) [tcv_left (pkt) - 2]
	);
	Count++;
}

// ============================================================================

fsm root {

  // This is also the receiver

  address rpkt;
  int sfd = -1;

  state RS_INIT:

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	// No need to enable transmitter
	// tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

  state RC_TRY:

	rpkt = tcv_rnp (RC_TRY, sfd);

  state RC_SHOW:

	show (RC_SHOW, rpkt);
	tcv_endp (rpkt);
	proceed RC_TRY;
}
