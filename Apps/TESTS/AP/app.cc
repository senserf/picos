/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2012                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
//
// A simple "access point" for OSS communication over RF using XRS
//

#include "sysio.h"
#include "tcvphys.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "cc1100.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "ab.h"

// The maximum packet length for CC1100
#define	MAXPLEN		CC1100_MAXPLEN
// 4 bytes are needed for XRS header
#define	UBUFLEN		(MAXPLEN-4)

#ifndef	AB_LINK_MODE
// We are in charge of the link by default. Define this as AB_MODE_PASSIVE, if
// you are implementing a null wireless modem on the other side.
#define	AB_LINK_MODE	AB_MODE_ACTIVE
#endif

// ============================================================================

fsm receiver {

  char *ibuf;

  state RC_READ:

	ibuf = ab_in (RC_READ);

  state RC_ECHO:

	ser_out (RC_ECHO, ibuf);
	ufree (ibuf);

  state RC_EL:

	ser_out (RC_EL, "\r\n");
	proceed RC_READ;
}

// ============================================================================

fsm root {

  char ubuf [UBUFLEN];
  int sfd = -1;

  state RS_INIT:

	word w;

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}
	w = 0xffff;
	tcv_control (sfd, PHYSOPT_SETSID, &w);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	ab_init (sfd);
	// We are in charge of the link
	ab_mode (AB_LINK_MODE);
	runfsm receiver;

  state RS_GREATING:

	ser_out (RS_GREATING, "AP:\r\n");

  state RS_WINP:

	ser_in (RS_WINP, ubuf, UBUFLEN);
	if (ubuf [0] == '\0')
		proceed RS_WINP;

  state RS_ECHO:

	ser_outf (RS_ECHO, "Sending: %s\r\n", ubuf);

  state RS_SEND:

	ab_outf (RS_SEND, "%s", ubuf);
	proceed RS_GREATING;
}
