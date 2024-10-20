#include "rf.h"
#include "peg.h"
#include "common.h"
#include "form.h"

sint	RFC,			// RF descriptor
	uart_desc;		// UART descriptor

static void init () {

	word sid;

	phys_cc1350 (0, MAX_PACKET_LENGTH);
	phys_uart (1, MAX_UART_LINE_LENGTH, 0);
	tcv_plug (0, &plug_null);
	RFC = tcv_open (NONE, 0, 0);
	uart_desc = tcv_open (WNONE, 1, 0);
	sid = ((word)(host_id >> 16));
	tcv_control (RFC, PHYSOPT_SETSID, &sid);
	tcv_control (RFC, PHYSOPT_RXON, NULL);
}

fsm rf_rx {

	word sid, data [3];
	lword scn;

	state RF_RX:

		address pkt;

		pkt = tcv_rnp (RF_RX, RFC);

		if (!pkt_issane (pkt) || pkt_did (pkt) != NODE_ID ||
		    pkt_cmd (pkt) != PKT_REPORT || pkt_ple (pkt) < 10) {
			// We only care about sane report packets addressed to
			// us; the legit payload length is 4 + 3 x 2 bytes
			tcv_endp (pkt);
			sameas RF_RX;
		}

		sid = pkt_sid (pkt);
		scn = ((lword*)pkt_pay (pkt)) [0];
		memcpy (data, pkt_pay (pkt) + 4, 6);
		tcv_endp (pkt);

	state RF_SHOW:

		address pkt;

		pkt = tcv_wnp (RF_SHOW, uart_desc, REPORT_LINE_LENGTH);
		form ((char*)pkt, "Node %d <%lu>: %x %x %x", sid, scn,
			data [0], data [1], data [2]);
		tcv_endp (pkt);

		sameas RF_RX;
}

fsm root {

	word did, del;

	state RS_START:

		powerup ();
		blink (0, 1, 512, 512, 200);
		init ();
		// Start the receiver process ...
		runfsm rf_rx;

		// ... and become the command listener

	state RS_UREAD:

		address buf;
		char *bp;
		sint k;

		buf = tcv_rnp (RS_UREAD, uart_desc);
		bp = (char*) buf;
		// Skip to first non blank
		while (*bp == ' ' || *bp == '\t')
			bp++;

		if (*bp == 'r') {
			// Run, expect two arguments: node Id + interval
			k = scan (bp + 1, "%u %u", &did, &del);
			tcv_endp (buf);
			if (k != 2)
				sameas RS_BADCMD;
			else
				sameas RS_SEND_RUN;
		}

		if (*bp == 's') {
			// Stop, expect a single argument
			k = scan (bp + 1, "%u", &did);
			tcv_endp (buf);
			if (k != 1)
				sameas RS_BADCMD;
			else
				sameas RS_SEND_STOP;
		}

		tcv_endp (buf);

	state RS_BADCMD:

		address pkt;
		const char *msg = "bad command";

		pkt = tcv_wnp (RS_BADCMD, uart_desc, strlen (msg) + 1);
		strcpy ((char*)pkt, msg);
		tcv_endp (pkt);

		sameas RS_UREAD;

	state RS_SEND_RUN:

		address pkt;

		pkt = tcv_wnp (RS_SEND_RUN, RFC, FRAME_LENGTH + 2);

		pkt_did (pkt) = did;
		pkt_sid (pkt) = NODE_ID;
		pkt_cmd (pkt) = PKT_RUN;
		pkt_ple (pkt) = 2;
		((word*)(pkt_pay (pkt))) [0] = del;
		tcv_endp (pkt);

		sameas RS_UREAD;

	state RS_SEND_STOP:

		address pkt;

		pkt = tcv_wnp (RS_SEND_RUN, RFC, FRAME_LENGTH + 0);

		pkt_did (pkt) = did;
		pkt_sid (pkt) = NODE_ID;
		pkt_cmd (pkt) = PKT_STOP;
		pkt_ple (pkt) = 0;
		tcv_endp (pkt);

		sameas RS_UREAD;

}
