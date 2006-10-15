#include "node_tag.h"

void TagNode::setup (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,		// Transmission rate
		Long	PRE,		// Preamble
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
    ) {

	PicOSNode::setup (mem, X, Y, XP, RP, BCmin, BCmax, LBTDel, LBTThs,
		rate, PRE, UMODE, UBS, USP, UIDV, UODV);
	// TARP
	TNode::setup (85, 97, 1);	// net_id, local_host, master_host

	ui_ibuf = ui_obuf = NULL;

	cmd_line = NULL;

	pong_params.freq_maj = 5120;
	pong_params.freq_min = 1024;
	pong_params.pow_levels = 0x0987;
	pong_params.rx_span = 1024;
	pong_params.rx_lev = 7;

	app_flags 	= 0;
	host_passwd 	= 0;
	host_id 	= 0xBACA0061;
	app_count.rcv   = 0;
	app_count.snd   = 0;
	app_count.fwd   = 0;

	appStart ();
}

// lib_app.c ==================================================================

char * TagNode::get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "Waiting for memory");
		umwait (state);
	}
	return buf;
}


void TagNode::send_msg (char * buf, int size) {
	// it doesn't seem like a good place to filter out
	// local host, but it's convenient, for now...

	// this shouldn't be... WARNING to see why it is needed...
	if (in_header(buf, rcv) == local_host) {
		app_diag (D_WARNING, "Dropped msg(%u) to lh",
			in_header(buf, msg_type));
		return;
	}

	if (net_tx (WNONE, buf, size, 0) == 0) {
		app_count.snd++;
//		app_diag (D_WARNING, "Sent msg %u to %u",
        	app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
}
 
word TagNode::max_pwr (word p_levs) {
 	word shift = 0;
 	word level = p_levs & 0x000f;
 	while ((shift += 4) < 16) {
	 	if (level < (p_levs >> shift) & 0x000f) 
			level = (p_levs >> shift) & 0x000f;
 	}
 	return level;
}
 
void TagNode::set_tag (char * buf) {
	// we may need more scrutiny...
	if (in_setTag(buf, node_addr) != 0)
		local_host = in_setTag(buf, node_addr);
	if (in_setTag(buf, pow_levels) != 0)
	    pong_params.rx_lev = max_pwr(in_setTag(buf, pow_levels));
		pong_params.pow_levels = in_setTag(buf, pow_levels);
	if (in_setTag(buf, freq_maj) != 0)
		pong_params.freq_maj = in_setTag(buf, freq_maj);
	if (in_setTag(buf, freq_min) != 0)
		pong_params.freq_min = in_setTag(buf, freq_min);
	if (in_setTag(buf, rx_span) != 0)
		pong_params.rx_span = in_setTag(buf, rx_span);
	if (in_setTag(buf, npasswd) != 0)
		host_passwd = in_setTag(buf, npasswd);
}

word TagNode::check_passwd (lword p1, lword p2) {
	if (host_passwd == p1)
		return 1;
	if (host_passwd == p2)
		return 2;
	app_diag (D_WARNING, "Failed passwd");
	return 0;
}

// msg_io.c ==================================================================

void TagNode::msg_getTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass;
	pass = check_passwd (in_getTag(buf, passwd), 0);

	msg_getTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
	//diag("get from %d seq %d", in_header(buf, snd), in_header(buf, seq_no));

	net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgGetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);
}

void TagNode::msg_getTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq, word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgGetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgGetTagAckType));

	in_header(*out_buf, msg_type) = msg_getTagAck;
	in_header(*out_buf, rcv) = rcv;

	in_getTagAck(*out_buf, ackSeq) = seq;
	in_getTagAck(*out_buf, ltime) = seconds();
	in_getTagAck(*out_buf, host_id) = host_id;
	in_getTagAck(*out_buf, tag) = local_host;
//	diag("get ack in %x %x pas %x %x", in_getTagAck(*out_buf, ltime), in_getTagAck(*out_buf, host_id));
	if(pass) {
		in_getTagAck(*out_buf, pow_levels) = pong_params.pow_levels;
		in_getTagAck(*out_buf, freq_maj) = pong_params.freq_maj;
		in_getTagAck(*out_buf, freq_min) = pong_params.freq_min;
		in_getTagAck(*out_buf, rx_span) = pong_params.rx_span;
		in_getTagAck(*out_buf, spare) = 0xff;
	}
}

void TagNode::msg_setTag_in (word state, char * buf) {
	char * out_buf = NULL;
	word pass = check_passwd (in_setTag(buf, passwd),
			in_setTag(buf, npasswd));
	//diag("set from %d seq %d", in_header(buf, snd), in_header(buf, seq_no));

	msg_setTagAck_out (state, &out_buf, in_header(buf, snd),
			in_header(buf, seq_no), pass);
    net_opt (PHYSOPT_TXON, NULL);
	send_msg (out_buf, sizeof(msgSetTagAckType));
	net_opt (PHYSOPT_TXOFF, NULL);
	ufree (out_buf);

	if (pass)
		set_tag (buf);
}

void TagNode::msg_setTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq, word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgSetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgSetTagAckType));

	in_header(*out_buf, msg_type) = msg_setTagAck;
	in_header(*out_buf, rcv) = rcv;
	in_setTagAck(*out_buf, tag) = local_host;
	in_setTagAck(*out_buf, ackSeq) = seq;
	in_setTagAck(*out_buf, ack) = (byte)pass;
}

int TagNode::tr_offset (headerType * mb) {

	excptn ("tr_offset unimplemented, not supposed to be called");
}

void TagNode::appStart () {

	fork (tag_root, NULL);
}

void buildTagNode (
		word mem,
		double	X,		// Coordinates
		double  Y,
		double	XP,		// Power
		double	RP,
		Long	BCmin,		// Backoff
		Long	BCmax,
		Long	LBTDel, 	// LBT delay (ms) and threshold (dBm)
		double	LBTThs,
		RATE	rate,
		Long	PRE,		// Preamble
		Long	UMODE,		// UART mode
		Long	UBS,		// UART buffer size
		Long	USP,		// UART rate
		char	*UIDV,		// Input device for UART
		char	*UODV		// Output device for UART
	) {
/*
 * The purpose of this is to isolate the two applications, such that their
 * specific "type" files don't have to be used together in any module.
 */
		create TagNode (
			mem,
			X,
			Y,
			XP,
			RP,
			BCmin,
			BCmax,
			LBTDel,
			LBTThs,
			rate,
			PRE,
			UMODE,
			UBS,
			USP,
			UIDV,
			UODV
		);
}
