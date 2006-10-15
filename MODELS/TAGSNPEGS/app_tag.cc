/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "node_tag.h"

#if DM2200
#define INFO_PHYS_DEV INFO_PHYS_DM2200
#endif

#ifndef INFO_PHYS_DEV
#error "UNDEFINED RADIO"
#endif

// UI is uart_a, including simulated uart_a
#define ui_out	ser_out
#define ui_outf ser_outf
#define ui_in	ser_in
#define ui_inf	ser_inf

// arbitrary
#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH

static const char welcome_str[] = "\r\nTag: s (lh M m pl span hid) h q\r\n";
static const char ill_str[] =	"Illegal command (%s)\r\n";
static const char stats_str1[] = "Stats for hostId: localHost (%lx: %u):\r\n";
static const char stats_str2[] = " In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n";
static const char stats_str3[] = " Time (%lu), Freqs (%u, %u), PLev: %x\r\n";
static const char stats_str4[] = "phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str5[] = " Mem free (%u, %u) faults (%u, %u)\r\n";

// Display node stats on UI
void TagNode::stats () {

	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);

	app_diag (D_UI, stats_str1, host_id, local_host);
	app_diag (D_UI, stats_str2, app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd);
	app_diag (D_UI, stats_str3, seconds(), pong_params.freq_maj,
			pong_params.freq_min, pong_params.pow_levels);
	app_diag (D_UI, stats_str4, net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL));
	app_diag (D_UI, stats_str5, mem0, mem1, faults0, faults1);
}

void TagNode::process_incoming (word state, char * buf, word size) {

  int w_len;
  
  switch (in_header(buf, msg_type)) {

	case msg_getTag:
		msg_getTag_in (state, buf);
		return;

	case msg_setTag:
		msg_setTag_in (state, buf);
		stats();
		return;

	case msg_rpc:
		if (cmd_line != NULL) { // busy with another input
			wait (CMD_WRITER, state);
			sleep;
		}
		w_len = size - sizeof(msgRpcType);
		cmd_line = get_mem (state, w_len);
		memcpy (cmd_line, buf + sizeof(msgRpcType), w_len);
		trigger (CMD_READER);
		return;

	case msg_getTagAck:
	case msg_setTagAck:
	case msg_pong:

		return;

	default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  }
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/
tag_rcv::perform {

	state RS_TRY:

		if (buf_ptr != NULL) {
			// This is a complete crapola: some S-> methods are
			// in fact macros that need no S-> in front (like
			// ufree). Some others are functions that need it.
			// FIXME: do everything one way or the other
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
		packet_size = S->net_rx (RS_TRY, &buf_ptr, NULL, 0);
		if (packet_size <= 0) {
			app_diag (D_SERIOUS, "net_rx failed (%d)",
				packet_size);
			proceed RS_TRY;
		}

		app_diag (D_DEBUG, "RCV (%d): %x-%u-%u-%u-%u-%u\r\n",
			  packet_size, in_header(buf_ptr, msg_type),
			  in_header(buf_ptr, seq_no) & 0xffff,
			  in_header(buf_ptr, snd),
			  in_header(buf_ptr, rcv),
			  in_header(buf_ptr, hoc) & 0xffff,
			  in_header(buf_ptr, hco) & 0xffff);

	transient RS_MSG:

		S->process_incoming (RS_MSG, buf_ptr, packet_size);
		S->app_count.rcv++;

		proceed RS_TRY;
}

/*
  -------------------
  Rxsw process
  ------------------
*/

tag_rxsw::perform {

	state RS_OFF:

		S->net_opt (PHYSOPT_RXOFF, NULL);
		net_diag (D_DEBUG, "Rx off %x",
			S->net_opt (PHYSOPT_STATUS, NULL));
		S->wait (RX_SW_ON, RS_ON);

	entry (RS_ON)
		S->net_opt (PHYSOPT_RXON, NULL);
		net_diag (D_DEBUG, "Rx on %x", 
			S->net_opt (PHYSOPT_STATUS, NULL));
		S->delay (S->pong_params.rx_span, RS_OFF);
}

// [0, F] -> [0, FF] (high power levels may not make sense anyway)
static word map_level (word l) {
	switch (l) {
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
			return 4;
		case 5:
			return 5;
		case 6:
			return 6;
		case 7:
			return 0x0A; // -4 dBm
		case 8:
			return 0x0F; // 0 dBm
		case 9:
			return 0x60; // 4 dBm
		case 10:
			return 0x70;
		case 11:
			return 0x80;
		case 12:
			return 0x90;
		case 13:
			return 0xC0;
		case 14:
			return 0xE0;
		case 15:
			return 0xFF;
		default:
			return 1; // -20 dBm
		}
}

/*
  --------------------
  Pong process: spontaneous ping a rebours
  PS_ <-> Pong State 
  --------------------
*/

tag_pong::perform {

	word level;

	state PS_INIT:

		in_header(frame, msg_type) = msg_pong;
		in_header(frame, rcv) = 0;
		in_header(frame, hco) = 1; // we do not want pong msg be forwarded by 
	transient PS_NEXT:

		// let's say 1ms is bad -- helps with input, and
		// doesn't make any sense anyway
		if (S->local_host == 0 || S->pong_params.freq_maj < 2) {
			app_diag (D_WARNING, "Pong's suicide");
			terminate;
		}
		shift = 0;

	transient PS_SEND:

		S->net_opt (PHYSOPT_TXON, NULL);
		net_diag (D_DEBUG, "Tx on %x",
			S->net_opt (PHYSOPT_STATUS, NULL));

	// State removed, appeared redundant (PG)

		level = ((S->pong_params.pow_levels >> shift) & 0x000f);
		if (level > 0 ) { // pong with this power
			in_pong (frame, level) = level;

			if (level == S->pong_params.rx_lev) {
				in_pong (frame, flags) |= PONG_RXON;
				trigger (RX_SW_ON);
			} else
				in_pong (frame, flags) = 0;

			level = map_level (level);
			S->net_opt (PHYSOPT_SETPOWER, &level);
			S->send_msg (frame, sizeof(msgPongType));
			app_diag (D_DEBUG, "It was pong on level %u",
				in_pong (frame, level));
		}
		S->net_opt (PHYSOPT_TXOFF, NULL);
		net_diag (D_DEBUG, "Tx off %x", 
			S->net_opt (PHYSOPT_STATUS, NULL));
		S->delay (S->pong_params.freq_min, PS_SENDP2);

	state PS_SENDP2:

		if ((shift += 4) < 16)
			proceed PS_SEND;

		if (S->pong_params.freq_maj > S->pong_params.freq_min << 2) {
		// << 2 is for 4 levels
			S->delay (S->pong_params.freq_maj - 
					(S->pong_params.freq_min << 2),
				PS_NEXT);
			sleep;
		}
		proceed PS_NEXT;
}

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/

tag_cmd_in::perform {

	state CS_INIT:
		if (S->ui_ibuf == NULL)
			S->ui_ibuf = S->get_mem (CS_INIT, UI_BUFLEN);

	transient CS_IN:
		// hangs on the uart_a interrupt or polling
		S->ser_in (CS_IN, S->ui_ibuf, UI_BUFLEN);
		if (strlen (S->ui_ibuf) == 0) // CR on empty line does it
			proceed CS_IN;

	transient CS_WAIT:
		if (S->cmd_line != NULL) {
			S->wait (CMD_WRITER, CS_WAIT);
			sleep;
		}

		S->cmd_line = S->get_mem (CS_WAIT, strlen (S->ui_ibuf) +1);
		strcpy (S->cmd_line, S->ui_ibuf);
		trigger (CMD_READER);
		proceed CS_IN;
}

/*
   --------------------
   Root process
   RS_ <-> Root State
   --------------------
*/

tag_root::perform {

	// input (s command)
	word in_lh, in_pl, in_maj, in_min, in_span;
	lword in_hid;
	
	state RS_INIT:
		S->local_host = (nid_t)(S->host_id);
		S->tarp_ctrl.param &= 0xFE; // routing off
		S->ui_out (RS_INIT, welcome_str);
		S->ui_obuf = S->get_mem (RS_INIT, UI_BUFLEN);
		if (S->net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			// reset ();
			excptn ("tag_root: reset called");
		}

		S->net_opt (PHYSOPT_SETSID, &(S->net_id));
		S->net_opt (PHYSOPT_RXOFF, NULL);
		S->net_opt (PHYSOPT_TXOFF, NULL);

		fork (tag_rcv, NULL);
		fork (tag_cmd_in, NULL);
		fork (tag_rxsw, NULL);

		if (S->local_host)
			fork (tag_pong, NULL);

		proceed RS_RCMD;

	state RS_FREE:
		ufree (S->cmd_line);
		S->cmd_line = NULL;
		trigger (CMD_WRITER);

	transient RS_RCMD:
		if (S->cmd_line == NULL) {
			S->wait (CMD_READER, RS_RCMD);
			sleep;
		}

	transient RS_DOCMD:
		if (S->cmd_line[0] == ' ') // ignore if starts with blank
			proceed RS_FREE;

                if (S->cmd_line[0] == 'h') {
			strcpy (S->ui_obuf, welcome_str);
			proceed RS_UIOUT;
		}

		if (S->cmd_line[0] == 'q')
			// reset();
			excptn ("tag_root: reset called (q)");
			
		if (S->cmd_line[0] == 's') {
			in_lh = in_pl = in_maj = in_min = in_span = 0;
			in_hid = 0;
			S->scan (S->cmd_line+1, "%u %u %u %x %u %lx",
				&in_lh, &in_maj, &in_min,  &in_pl, &in_span,
				&in_hid);
			if (in_lh) {
				if (S->local_host == 0) {
					S->local_host = in_lh;
					fork (tag_pong, NULL);
				} else {
					S->local_host = in_lh;
				}
			}
			if (in_pl) {
				S->pong_params.rx_lev = S->max_pwr(in_pl);
				S->pong_params.pow_levels = in_pl;
			}
			if (in_maj) {
				if (S->pong_params.freq_maj < 2) {
					S->pong_params.freq_maj = in_maj;
					fork (tag_pong, NULL);
				} else {
					S->pong_params.freq_maj = in_maj;
				}
			}
			if (in_min)
				S->pong_params.freq_min = in_min;
			if (in_span)
				S->pong_params.rx_span = in_span;
			if (in_hid)
				S->host_id = in_hid;
			S->stats();
			proceed RS_FREE;
		}

		form (S->ui_obuf, ill_str, S->cmd_line);

	transient RS_UIOUT:
		S->ui_out (RS_UIOUT, S->ui_obuf);
		proceed RS_FREE;

}
