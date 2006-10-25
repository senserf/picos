/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "node_peg.h"

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

#define UI_BUFLEN		UART_INPUT_BUFFER_LENGTH
#define DEF_LOCAL_HOST		1

static const char welcome_str[] =
				"\r\nPeg: s (lh hid mid pl) m f g t r h q\r\n";
static const char o_str[] =	"phys: %x, plug: %x, txrx: %x\r\n";
static const char ill_str[] =	"Illegal command (%s)\r\n";
static const char stats_str1[] = "Stats for hostId - localHost (%lx - %u):\r\n";
static const char stats_str2[] = " In (%u:%u), Out (%u:%u), Fwd (%u:%u),\r\n";
static const char stats_str3[] = " Freq audit (%u) events (%u),\r\n";
static const char stats_str4[] = 
		" PLev (%u), Time (%lu), Delta (%ld) to Master (%u),\r\n";
static const char stats_str5[] = " phys: %x, plug: %x, txrx: %x\r\n";
static const char stats_str6[] = " Mem free (%u, %u) faults (%u, %u)\r\n";

// Display node stats on UI
void PegNode::stats () {

	word faults0, faults1;
	word mem0 = memfree(0, &faults0);
	word mem1 = memfree(1, &faults1);

	net_diag (D_UI, stats_str1, host_id, local_host);
	net_diag (D_UI, stats_str2, app_count.rcv, tarp_ctrl.rcv,
			app_count.snd, tarp_ctrl.snd,
			app_count.fwd, tarp_ctrl.fwd);
	net_diag (D_UI, stats_str3, tag_auditFreq, tag_eventGran);
	net_diag (D_UI, stats_str4, host_pl, seconds(), 
			master_delta, master_host);
	net_diag (D_UI, stats_str5, net_opt (PHYSOPT_PHYSINFO, NULL),
			net_opt (PHYSOPT_PLUGINFO, NULL),
			net_opt (PHYSOPT_STATUS, NULL));
	net_diag (D_UI, stats_str6, mem0, mem1, faults0, faults1);
}

void PegNode::process_incoming (word state, char *buf, word size, word rssi) {

	int w_len;

  	if (check_msg_size (buf, size, D_SERIOUS) != 0)
	  	return;

  	switch (in_header(buf, msg_type)) {

	    case msg_pong:
		if (in_pong_rxon(buf)) 
			check_msg4tag (in_header(buf, snd));

		msg_pong_in (state, buf, rssi);
		return;

	    case msg_report:
		msg_report_in (state, buf);
		return;

	    case msg_reportAck:
		msg_reportAck_in (buf);
		return;

	    case msg_master:
		msg_master_in (buf);
		return;

	    case msg_findTag:
		msg_findTag_in (state, buf);
		return;

	    case msg_fwd:
		msg_fwd_in (state, buf, size);
		return;

	    case msg_getTagAck:
		msg_getTagAck_in (state, buf, size);
		return;

	    case msg_setTagAck:
		msg_setTagAck_in (state, buf, size);
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

	// more types, not implemented yet
	    default:
		app_diag (D_SERIOUS, "Got ? (%u)", in_header(buf, msg_type));

  	}
}

// [0, FF] -> [0, F], say:
static word map_rssi (word r) {
	return (r >> 4);
}

/*
   --------------------
   Receiver process
   RS_ <-> Receiver State
   --------------------
*/

// In this model, a single rcv is forked once, and runs / sleeps all the time
peg_rcv::perform {

	state RS_TRY:
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
			packet_size = 0;
		}
    		packet_size = S->net_rx (RS_TRY, &buf_ptr, &rssi, 0);
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

		// that's how we could check which plugin is on
		// if (S->net_opt (PHYSOPT_PLUGINFO, NULL) != INFO_PLUG_TARP)

	transient RS_MSG:
		S->process_incoming (RS_MSG, buf_ptr, packet_size,
			map_rssi(rssi));
		S->app_count.rcv++;
		proceed RS_TRY;
}

/*
  --------------------
  audit process
  AS_ <-> Audit State
  --------------------
*/

peg_audit::perform {

	state AS_START:
		if (buf_ptr != NULL) {
			ufree (buf_ptr);
			buf_ptr = NULL;
		}
		if (S->tag_auditFreq == 0) {
			app_diag (D_WARNING, "Audit stops");
			terminate;
		}
		ind = tag_lim;
		app_diag (D_DEBUG, "Audit starts");

	transient AS_TAGLOOP:
		if (ind-- == 0) {
			app_diag (D_DEBUG, "Audit ends");
			S->delay (S->tag_auditFreq, AS_START);
			sleep;
		}

	transient AS_TAGLOOP1:
		S->check_tag (AS_TAGLOOP+1, ind, &buf_ptr);

		if (buf_ptr) {
			if (S->local_host == S->master_host) {
				in_header(buf_ptr, snd) = S->local_host;
				S->oss_report_out (buf_ptr, oss_fmt);
			} else
				S->send_msg (buf_ptr, sizeof(msgReportType));
			ufree (buf_ptr);
			buf_ptr = NULL;
		}
		proceed AS_TAGLOOP;
}

/*
   --------------------
   cmd_in process
   CS_ <-> Command State
   --------------------
*/

peg_cmd_in::perform {

	state CS_INIT:
		if (S->ui_ibuf == NULL)
			S->ui_ibuf = S->get_mem (CS_INIT, UI_BUFLEN);

	transient CS_IN:
		S->ser_in (CS_IN, S->ui_ibuf, UI_BUFLEN);
		if (strlen(S->ui_ibuf) == 0) // CR on empty line would do it
			proceed CS_IN;

	transient CS_WAIT:
		if (S->cmd_line != NULL) {
			S->wait (CMD_WRITER, CS_WAIT);
			sleep;
		}

		S->cmd_line = S->get_mem (CS_WAIT, strlen(S->ui_ibuf) +1);
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

peg_root::perform {

	lword 		in_hid;
	nid_t 		in_lh, in_mh;
	word		in_hpl; // peg power level
	int		opt_sel;
	char 		str [PEG_STR_LEN];

	state RS_INIT:
//		local_host = DEF_LOCAL_HOST;
//		app_trace("init");

		// these survive for repeated commands;
		in_tag = in_pass = in_npass = 0;
		in_peg = in_rssi = in_pl = 0;

		S->init_tags ();
		S->ui_out (RS_INIT, welcome_str);
		S->ui_obuf = S->get_mem (RS_INIT, UI_BUFLEN);

		if (S->net_init (INFO_PHYS_DEV, INFO_PLUG_TARP) < 0) {
			app_diag (D_FATAL, "net_init failed");
			S->reset ();
		}

		S->net_opt (PHYSOPT_SETSID, &(S->net_id));
		S->net_opt (PHYSOPT_SETPOWER, &(S->host_pl));
		(void) fork (peg_rcv, NULL);
		(void) fork (peg_cmd_in, NULL);
		(void) fork (peg_audit, NULL);
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
//	     app_trace("rs_docmd");
		if (S->cmd_line[0] == ' ') // ignore if starts with blank
			proceed RS_FREE;

                if (S->cmd_line[0] == 'h') {
//                	app_trace("help");
			strcpy (S->ui_obuf, welcome_str);
//			app_trace("help_end");
			proceed RS_UIOUT;
		}

		if (S->cmd_line[0] == 'q')
			S->reset ();

		if (S->cmd_line[0] == 'm') {
			 S->scan (S->cmd_line+1, "%u", &in_peg);
			 S->oss_master_in (RS_DOCMD, in_peg);
			 proceed RS_FREE;
		}

		if (S->cmd_line[0] == 'f') {
//			app_trace("fwd");
			in_tag = 0;
		    	in_peg = in_rssi = in_pl = 0;
			S->scan (S->cmd_line+1, "%lu, %u, %u, %u",
				&in_tag, &in_peg, &in_rssi, &in_pl);
			*(word*)&in_tag = (in_rssi << 8 ) | in_pl;
			S->oss_findTag_in (RS_DOCMD, in_tag, in_peg);
//			app_trace("fwd-end");
			proceed RS_FREE;
		}

		if (S->cmd_line[0] == 'g') {
//			app_trace("get");
			S->scan (S->cmd_line+1, "%lu, %u, %lu",
				&in_tag, &in_peg, &in_pass);
			S->oss_getTag_in (RS_DOCMD, in_tag, in_peg, in_pass);
//			app_trace("get_end");
			proceed RS_FREE;
		}

        	if (S->cmd_line[0] == 't') {
			in_lh = in_pl = in_maj = in_min = in_span = 0;
			in_hid = 0;
			S->scan (S->cmd_line+1, "%lu %u %lu %u %u %u %x %u %lu",
				&in_tag, &in_peg, &in_pass, &in_lh, &in_maj,
				&in_min,  &in_pl, &in_span, &in_npass);
			S->oss_setTag_in (RS_DOCMD, in_tag, in_peg, in_pass,
				in_lh, in_maj, in_min, in_pl, in_span,
				in_npass);
			proceed RS_FREE;
		}
		
		if (S->cmd_line[0] == 'r') {
			in_lh = in_peg = in_hpl = 0;
			S->scan (S->cmd_line+1, "%u %u %u %s",
				&in_peg, &in_lh, &in_hpl, str);
//			app_diag(D_WARNING, "remote set %u new %u pwr %u %s", in_peg, in_lh, in_hpl, str);
			S->oss_setPeg_in (RS_DOCMD, in_peg, in_lh, in_hpl, str);
			proceed RS_FREE;
		}

		if (S->cmd_line[0] == 's') {
//			app_trace("stat");
			in_lh = in_hpl = in_mh = 0;
			in_hid = 0;
//			app_trace("stat_beg");
			S->scan (S->cmd_line+1, "%u, %lu, %u, %u",
				&in_lh, &in_hid, &in_mh, &in_hpl);
			if (in_lh)
				S->local_host = in_lh;
//			app_trace("stat_lh");
			if (in_hid)
				S->host_id = in_hid;
			if (in_mh)
				S->master_host = in_mh;
			if (in_hpl) {
				S->net_opt (PHYSOPT_SETPOWER, &in_hpl);
				S->host_pl = in_hpl;
			}

			S->stats();
			proceed RS_FREE;
		}

		if (S->cmd_line[0] == 'o') {

			if (strlen (S->cmd_line) > 2) {
				opt_sel = 0;
				if (S->cmd_line[1] == 'r')
					opt_sel++;
				else if (S->cmd_line[1] == 't')
					opt_sel += 2;
				else if (S->cmd_line[1] == ' ')
					opt_sel +=3;
				else {
					S->form (S->ui_obuf, ill_str,
						S->cmd_line);
					proceed RS_UIOUT;
				}

				if (S->cmd_line[2] == '+') {
					if (opt_sel & 1)
					S->net_opt (PHYSOPT_RXON, NULL);
					if (opt_sel & 2)
					S->net_opt (PHYSOPT_TXON, NULL);

				} else if (S->cmd_line[2] == '-') {
					if (opt_sel & 1)
						S->net_opt (PHYSOPT_RXOFF,
							NULL);
					if (opt_sel & 2)
						S->net_opt (PHYSOPT_TXOFF,
							NULL);
				}
			}
			S->form (S->ui_obuf, o_str,
				S->net_opt (PHYSOPT_PHYSINFO, NULL),
				    S->net_opt (PHYSOPT_PLUGINFO, NULL),
			       		S->net_opt (PHYSOPT_STATUS, NULL));
			proceed RS_UIOUT;
		}

		S->form (S->ui_obuf, ill_str, S->cmd_line);

	transient RS_UIOUT:
		S->ui_out (RS_UIOUT, S->ui_obuf);
		proceed RS_FREE;
}
