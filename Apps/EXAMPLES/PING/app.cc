#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	MAX_PACKET_LENGTH	62
#define	IBUF_LENGTH		82

sint sfd = -1;			// RF session descriptor

char ibuf [IBUF_LENGTH];	// Buffer for UART input

// Sender parameters

word s_count, s_delay, s_length, s_content, s_counter;

fsm sender {

	state SND_LOOP:

		address packet;

		// s_length is "payload" length, we need these extras:
		//	2 bytes (one word) for NETID (0)
		//	2 bytes (one word) for Node Id
		//	2 bytes for the counter
		//	2 bytes (one word for CRC) [at the end]
		packet = tcv_wnp (SND_LOOP, sfd, s_length + 4*2);
		packet [0] = 0;
		packet [1] = host_id;
		packet [2] = s_counter++;

		for (sint i = 0; i < s_length/2; i++)
			// Note that packet is an array of words
			packet [3 + i] = s_content;

		// Done, send it out
		tcv_endp (packet);

		if (s_count == 0 || --s_count) {
			delay (s_delay, SND_LOOP);
			sleep;
		}

	state SND_BYE:

		ser_out (SND_BYE, "Sender stop\r\n");
		finish;
}

fsm receiver {

	address packet;

	state RCV_LOOP:

		packet = tcv_rnp (RCV_LOOP, sfd);
		if (tcv_left (packet) < 10) {
			// Ignore garbage
rcv_end:
			tcv_endp (packet);
			sameas RCV_LOOP;
		}

	state RCV_SHOW:

		ser_outf (RCV_SHOW, "PKT, len = %u, from = %u, num = %u, "
			"val = %x\r\n",
			tcv_left (packet),
			packet [1], packet [2], packet [3]);
		goto rcv_end;
}

Boolean do_s_command () {

	if (running (sender))
		// Already sending
		return NO;

	s_count = 1; s_delay = 1024; s_length = 2; s_content = 0;
	scan (ibuf + 1, "%u %u %u %x",	&s_count,
				      	&s_delay,
				      	&s_length,
					&s_content);

	// Make sure length is legit: between 2 and 58
	if (s_length < 2)
		s_length = 2;
	else if (s_length > 56)
		s_length = 58;
	else if (s_length & 1)
		s_length ++;

	runfsm sender;

	return YES;
}

Boolean do_r_command () {

	tcv_control (sfd, PHYSOPT_RXON, NULL);
	return YES;
}

Boolean do_k_command () {

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	killall (sender);
	return YES;
}

fsm root {

  state RS_INIT:

	// Initialize the radio interface

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);

	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	runfsm receiver;

  state RS_RCMD_M:

	ser_out (RS_RCMD_M,
		"\r\nRF PING\r\n"
		"Commands:\r\n"
		"s k d n w -> send k times at d an n-byte packet\r\n"
		"r         -> start receiver\r\n"
		"k         -> kill s+r\r\n"
	);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH);

	switch (ibuf [0]) {

		case 's' :
			if (do_s_command ())
				proceed RS_OK;
			break;
		case 'r' :
			if (do_r_command ())
				proceed RS_OK;
			break;
		case 'k' :
			if (do_k_command ())
				proceed RS_OK;
			break;
	}

  state RS_RCMD_E:

	ser_out (RS_RCMD_E, "Illegal command or parameter\r\n");
	proceed RS_RCMD_M;

  entry RS_OK:

	ser_out (RS_OK, "OK\r\n");
	proceed RS_RCMD;
}
