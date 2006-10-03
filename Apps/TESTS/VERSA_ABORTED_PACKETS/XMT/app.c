/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "plug_null.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_dm2200.h"

#define	MAX_PACKET_LENGTH	64
#define	IBUFLEN			64

static	word	CntSent,
		SPA = 0xAAAA,		// Pattern
		PLen = 32;

static int sfd;
static word SendInterval = 2048;

/* ======================================================================= */

#define	SN_SEND		00

process (sender, void)

	address packet;
	int i;
	word w;

	nodata;

  entry (SN_SEND)

	packet = tcv_wnp (SN_SEND, sfd, PLen);

	packet [0] = 0;	// Net ID
	packet [1] = CntSent;

	for (i = 2; i < PLen/2; i++)
			packet [i] = SPA;

	diag ("XMT: [%d] %x %x", PLen, packet [1], packet [2]);

	tcv_endp (packet);
	CntSent++;
	delay (SendInterval, SN_SEND);

endprocess (1)

void do_start () {

	if (!running (sender))
		fork (sender, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
}

void do_quit () {

	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	killall (sender);
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SCI		20
#define	RS_STA		30
#define	RS_SPL		40
#define	RS_SPA		50
#define	RS_DON		60

process (root, int)

	static char *ibuf;
	int n;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_dm2200 (0, MAX_PACKET_LENGTH + 4);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

  entry (RS_RCMD-2)

  	ser_outf (RS_RCMD-2,
	"\r\nXMITTER:\r\n"
	"Commands [d = int, x = hex]:\r\n"
	"i d      -> set xmit interval (msec) [%u]\r\n"
	"a d      -> action: 0-stop, 1-start\r\n"
	"l d      -> packet length in bytes [6-64]: %d\r\n"
	"x x      -> hex pattern to send [%x]\r\n"
	,
		SendInterval, PLen, SPA
		
	);

  entry (RS_RCMD)
  
	ibuf [0] = ' ';
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 'i')
		proceed (RS_SCI);
	if (ibuf [0] == 'a')
		proceed (RS_STA);
	if (ibuf [0] == 'x')
		proceed (RS_SPA);
	if (ibuf [0] == 'l')
		proceed (RS_SPL);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SCI)

	n = 0;
	scan (ibuf + 1, "%d", &n);
	if (n <= 0)
		proceed (RS_RCMD+1);
	SendInterval = n;
	proceed (RS_DON);

  entry (RS_STA)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	if (n)
		do_start ();
	else
		do_quit ();

	proceed (RS_DON);

  entry (RS_SPL)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 6 || n > MAX_PACKET_LENGTH)
		proceed (RS_RCMD+1);
	// Make it even
	PLen = (n + 1) & 0xFFFE;
	proceed (RS_DON);

  entry (RS_DON)

	diag ("DONE");
	proceed (RS_RCMD);

  entry (RS_SPA)

	scan (ibuf + 1, "%x", &SPA);
	proceed (RS_DON);

endprocess (1)
