#include "node_peg.h"

void PegNode::setup (
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
	TNode::setup (85, 1, 1);	// network_id, local_host, master_host

	init ();
}

void PegNode::init () {

	ui_ibuf = ui_obuf = NULL;
	cmd_line = NULL;

	app_flags = 0;
	host_passwd = 0;
	master_delta = 0;
	host_pl = 9;
	tag_auditFreq = 10240;
	tag_eventGran = 10;

	msg4tag.buf = NULL;
	msg4tag.tstamp = 0;

	msg4ward.buf = NULL;
	msg4ward.tstamp = 0;

	app_count.rcv = app_count.snd = app_count.fwd = 0;

	appStart ();
}

void PegNode::reset () {

	TNode::reset ();

	init ();
}

// lib_app.c ==================================================================

int PegNode::find_tag (lword tag) {
	int i = 0;
	lword mask = 0xffffffff;
	
	if (!(tag & 0xff000000))  // rss irrelevant
		mask &= 0x00ffffff;

	if (!(tag & 0x00ff0000)) // power level 
		mask &= 0xff00ffff;

	while (i < tag_lim) {
		if ((tagArray[i].id & mask) == tag)
			return i;
		i++;
	}
	return -1;
}

char * PegNode::get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		app_diag (D_WARNING, "Waiting for memory");
		umwait (state);
	}
	return buf;
}

void PegNode::init_tag (word i) {
	tagArray[i].id = 0;
	tagArray[i].state = noTag;
	tagArray[i].count = 0;
	tagArray[i].evTime = 0;
	tagArray[i].lastTime = 0;
}

void PegNode::init_tags () {
	word i = tag_lim;
	while (i-- > 0)
		init_tag (i);
}

void PegNode::set_tagState (word i, tagStateType state, bool updEvTime) {
	tagArray[i].state = state;
	tagArray[i].count = 0; // always (?) reset the counter
	tagArray[i].lastTime = seconds();
	if (updEvTime)
		tagArray[i].evTime = tagArray[i].lastTime;
}

int PegNode::insert_tag (lword tag) {
	int i = 0;

	while (i < tag_lim) {
		if (tagArray[i].id == 0) {
			tagArray[i].id = tag;
			set_tagState (i, newTag, YES);
			app_diag (D_DEBUG, "Inserted tag %lx at %u", tag, i);
			return i;
		}
		i++;
	}
	app_diag (D_SERIOUS, "Failed tag (%lx) insert", tag);
	return -1;
}

void PegNode::check_tag (word state, word i, char** buf_out) {
	if (i >= tag_lim) {
		app_diag (D_FATAL, "tagAr bound %u", i);
		return;
	}
	
	if (tagArray[i].id == 0 ||
		seconds() - tagArray[i].lastTime < tag_eventGran)
		return;	

	switch (tagArray[i].state) {
		case newTag:
			app_diag (D_DEBUG, "Delete %lx", tagArray[i].id);
			init_tag (i);
			break;

		case goneTag:
			app_diag (D_DEBUG, "Report gone %lx", tagArray[i].id);
			msg_report_out (state, i, buf_out);
			break;

		case reportedTag:
			app_diag (D_DEBUG, "Set fadingReported %lx",
				tagArray[i].id);
			set_tagState (i, fadingReportedTag, NO);
			break;

		case confirmedTag:
			app_diag (D_DEBUG, "Set fadingConfirmed %lx",
				tagArray[i].id);
			set_tagState (i, fadingConfirmedTag, NO);
			break;

		case fadingReportedTag:
		case fadingConfirmedTag:
			app_diag (D_DEBUG, "Report going %lx",
				tagArray[i].id);
			set_tagState (i, goneTag, YES);
			msg_report_out (state, i, buf_out);
			if (local_host == master_host || master_host == 0)
				init_tag (i); // problematic kludge...
			break;

		default:
			app_diag (D_SERIOUS, "noTag? %lx in %u",
				tagArray[i].id, tagArray[i].state);
	}
}

void PegNode::copy_fwd_msg (word state, char** buf_out, char * buf, word size) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	memcpy (*buf_out, buf, size);
}

void PegNode::send_msg (char * buf, int size) {
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
//		app_diag (D_WARNING, "Sent %u to %u",
		app_diag (D_DEBUG, "Sent %u to %u",
			in_header(buf, msg_type),
			in_header(buf, rcv));
	} else
		app_diag (D_SERIOUS, "Tx %u failed",
			in_header(buf, msg_type));
 }

int PegNode::check_msg_size (char * buf, word size, word repLevel) {
	word expSize;
	
	// for some msgTypes, it'll be less trivial
	switch (in_header(buf, msg_type)) {
		case msg_pong:
			if ((expSize = sizeof(msgPongType)) == size)
				return 0;
			break;

		case msg_master:
			if ((expSize = sizeof(msgMasterType)) == size)
				return 0;
			break;

		case msg_report:
			if ((expSize = sizeof(msgReportType)) == size)
				return 0;
			break;

		case msg_reportAck:
			if ((expSize = sizeof(msgReportAckType)) == size)
				return 0;
			break;

		case msg_getTagAck:
			if ((expSize = sizeof(msgGetTagAckType)) == size)
				 return 0;
			break;

		case msg_setTagAck:
			if ((expSize = sizeof(msgSetTagAckType)) == size)
				return 0;
			break;

		case msg_findTag:
			if ((expSize = sizeof(msgFindTagType)) == size)
				return 0;
			break;

		// if it's needed, can be done... who cares now
		case msg_fwd:
		case msg_rpc:
			return 0;

		default:
			app_diag (repLevel, "Can't check size of %u (%d)",
				in_header(buf, msg_type), size);
			return 0;
	}
	
	// 4N padding might have happened
	if (size > expSize && size >> 2 == expSize >> 2)
		return 0;

	app_diag (repLevel, "Size error for %u: %d of %d",
			in_header(buf, msg_type), size, expSize);
	return (size - expSize);
}

/* if this is ever promoted to a regular functionality,
   we may have a process with a msg buffer waiting for
   trigger (TAG_LISTENS+id) from here, or the msg buffer
   hanging off tagDataType. For now, we keep it as simple as possible:
   check for a msg pending for this tag
*/

void PegNode::check_msg4tag (nid_t tag) {
//	diag ("check msg");
	if (msg4tag.buf && in_header(msg4tag.buf, rcv) == tag) {
//		diag ("check msg %d tag %d", in_header(msg4tag.buf, msg_type), tag);
		if (seconds() - msg4tag.tstamp <= 77) // do it
			send_msg (msg4tag.buf,
				in_header(msg4tag.buf, msg_type) == msg_getTag ?
				sizeof(msgGetTagType) : sizeof(msgSetTagType));
		ufree (msg4tag.buf);
		msg4tag.buf = NULL;
		msg4tag.tstamp = 0;
	}
}

// msg_io.c ==================================================================

word PegNode::countTags() {
	word i = tag_lim;
	word j = 0;

	while (i-- > 0) {
		if (tagArray[i].id)
			j++;
	}
	return j;
}

void PegNode::msg_report_out (word state, int tIndex, char** out_buf) {

	if (master_host == 0) { // nobody to report to
		app_diag (D_INFO, "I'm a Ronin");
		return;
	}

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportType));
	else
		memset (*out_buf, 0, sizeof(msgReportType));

	in_header(*out_buf, msg_type) = msg_report;
	in_header(*out_buf, rcv) = master_host;
	if (tIndex < 0) {
		in_report(*out_buf, tagId) = 0;
		in_report(*out_buf, tStamp) = master_delta;
		in_report(*out_buf, state) = sumTag;
		in_report(*out_buf, count) = countTags();
		return;
	}
	in_report(*out_buf, tagId) = tagArray[tIndex].id;
	in_report(*out_buf, tStamp) = tagArray[tIndex].evTime + master_delta;
	in_report(*out_buf, state) = tagArray[tIndex].state;
	in_report(*out_buf, count) = ++tagArray[tIndex].count;
}

void PegNode::msg_findTag_in (word state, char * buf) {

	char * out_buf = NULL;
	int tagIndex;

	if ( in_findTag(buf, target) == 0) { // summary
		msg_report_out (state, -1, &out_buf);
	} else {
		tagIndex = find_tag (in_findTag(buf, target));
		msg_report_out (state, tagIndex, &out_buf);
		// kludgy: we have an absent tag reported as sumary; change:
		if (tagIndex < 0) {
			in_report(out_buf, tagId) = in_findTag(buf, target);
			in_report(out_buf, state) = noTag;
		}
	}
	if (out_buf) {
		if (local_host == master_host) {
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf, oss_fmt);
		} else
			send_msg (out_buf, sizeof(msgReportType));
		ufree (out_buf);
	}
}

void PegNode::msg_findTag_out (word state, char** buf_out, lword tag,
								nid_t peg) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgFindTagType));
	else
		memset (*buf_out, 0, sizeof(msgFindTagType));

	in_header(*buf_out, msg_type) = msg_findTag;
	in_header(*buf_out, rcv) = peg;
	in_findTag(*buf_out, target) = tag;
}

void PegNode::msg_fwd_in (word state, char * buf, word size) {

	char * out_buf = NULL;
	int tagIndex;
	int w_len;

	if ( in_findTag(buf, target) != 0) {
		tagIndex = find_tag (in_fwd(buf, target));
		//diag ("FWD tag %d peg %d", (word)in_fwd(buf, target), in_header(buf, rcv));
		if (tagIndex >= 0) {
			w_len = size - sizeof(msgFwdType);
			copy_fwd_msg(state, &out_buf, buf + sizeof(msgFwdType), w_len);
		}
	} else {
		// kludge for remote set
		w_len = size - sizeof(msgFwdType);
		copy_fwd_msg(state, &out_buf, buf + sizeof(msgFwdType), w_len);
		if(in_header(out_buf, msg_type) == msg_setPeg) {
			if(in_setPeg(out_buf, new_id) != 0)
				local_host = in_setPeg(out_buf, new_id);
			if (in_setPeg(out_buf, level) != 0) {
				net_opt (PHYSOPT_SETPOWER, &in_setPeg(out_buf, level));
				host_pl = in_setPeg(out_buf, level);
			}
			diag ("PRINT MSG from peg %d %s", in_header(buf, snd), in_setPeg(out_buf, str));
			ufree (out_buf);
			return;
		}
	}
	if (out_buf) {
		//diag ("FWD in %d %d", in_header(out_buf, msg_type), in_header(out_buf, rcv));
		if(msg4tag.buf)
				ufree (msg4tag.buf); // discard old message
		msg4tag.buf = out_buf;
		msg4tag.tstamp = seconds();
	}
}

void PegNode::msg_fwd_out (word state, char** buf_out, word size, lword tag,
							nid_t peg, lword pass) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, size);
	else
		memset (*buf_out, 0, size);

	in_header(*buf_out, msg_type) = msg_fwd;
	in_header(*buf_out, rcv) = peg;
	in_fwd(*buf_out, target) = tag;
	in_fwd(*buf_out, passwd) = pass;
}

void PegNode::msg_getTagAck_in (word state, char * buf, word size) {

	oss_getTag_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgGetTagAckType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

void PegNode::msg_master_in (char * buf) {

	master_delta = in_master(buf, mtime) - seconds();
	master_host  = in_header(buf, snd); // blindly, for now
	app_diag (D_INFO, "Set master to %u at %ld", master_host,
			master_delta);
}

void PegNode::msg_master_out (word state, char** buf_out, nid_t peg) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = peg;
	in_master(*buf_out, mtime) = seconds();
}

void PegNode::msg_pong_in (word state, char * buf, word rssi) {
	char * out_buf = NULL; // is static needed / better here? (state)
	int tagIndex;

	lword key = ((lword)rssi << 8) | (lword)in_pong(buf, level);
	key = (key << 16) | in_header(buf, snd);
	// key = rssi(8b) pow_lev(8) tagId(16)
	app_diag (D_DEBUG, "Pong %lx", key);

	if ((tagIndex = find_tag (key)) < 0) { // not found
		insert_tag (key);
		return;
	}
	switch (tagArray[tagIndex].state) {
		case noTag:
			app_diag (D_SERIOUS, "NoTag error");
			return;

		case newTag:
			msg_report_out (state, tagIndex, &out_buf);
			if (master_host == local_host)
				set_tagState (tagIndex, confirmedTag, NO);
			else
				set_tagState (tagIndex, reportedTag, NO);
			break;

		case reportedTag:
			tagArray[tagIndex].lastTime = seconds();
			msg_report_out (state, tagIndex, &out_buf);
			// this shit may happen if the master is changed to
			// be local host:
			if (master_host == local_host)
				set_tagState (tagIndex, confirmedTag, NO);
			break;

		case confirmedTag:
			tagArray[tagIndex].lastTime = seconds();
			app_diag (D_DEBUG, "Ignoring Tag %lx",
				       	tagArray[tagIndex].id);
			break;

		case fadingReportedTag:
			set_tagState(tagIndex, reportedTag, NO);
			break;

		case fadingConfirmedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;

		case goneTag:
			set_tagState(tagIndex, newTag, YES);
			break;

		default:
			app_diag (D_SERIOUS, "Tag state?(%u) Suicide!",
				tagArray[tagIndex].state);
			reset();
	}

	if (out_buf) {
		if (local_host == master_host) {
			// master with tags - hack the sender
			in_header(out_buf, snd) = local_host;
			oss_report_out (out_buf, oss_fmt);
		} else
			send_msg (out_buf, sizeof(msgReportType));
		ufree (out_buf);
		out_buf = NULL;
	}
}

void PegNode::msg_report_in (word state, char * buf) {
	char * out_buf = NULL;

	msg_reportAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, sizeof(msgReportAckType));
		ufree (out_buf);
		out_buf = NULL;
	}
	oss_report_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgReportType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

void PegNode::msg_reportAck_in (char * buf) {
	int tagIndex;

	if ((tagIndex = find_tag (in_reportAck(buf, tagId))) < 0) { // not found
		app_diag (D_INFO, "Ack for a goner %lx",
			       in_reportAck(buf, tagId));
		return;
	}

	app_diag (D_DEBUG, "Ack (in %u) for %lx in %u",
		in_reportAck(buf, state),
		tagArray[tagIndex].id, tagArray[tagIndex].state);

	switch (tagArray[tagIndex].state) {
		case reportedTag:
			set_tagState(tagIndex, confirmedTag, NO);
			break;

		case fadingReportedTag:
			set_tagState(tagIndex, fadingConfirmedTag, NO);
			break;

		case goneTag:
			init_tag (tagIndex);
			break;

		default:
			app_diag (D_INFO, "Ignoring Ack for %lx on %u",
				tagArray[tagIndex].id,
				tagArray[tagIndex].state);
	}
}

void PegNode::msg_reportAck_out (word state, char * buf, char** out_buf) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportAckType));
	else
		memset (*out_buf, 0, sizeof(msgReportAckType));

	in_header(*out_buf, msg_type) = msg_reportAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_reportAck(*out_buf, tagId) = in_report(buf, tagId);
	in_reportAck(*out_buf, state) = in_report(buf, state);
	in_reportAck(*out_buf, count) = in_report(buf, count);
}

void PegNode::msg_setTagAck_in (word state, char * buf, word size) {

	oss_setTag_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgSetTagAckType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}

// oss_io.c ==================================================================

char * PegNode::stateName (unsigned state) {
	switch ((tagStateType)state) {
		case noTag:
			return "noTag\0";
		case newTag:
			return "newTag\0";
		case reportedTag:
			return "reportedTag\0";
		case confirmedTag:
			return "confirmedTag\0";
		case fadingReportedTag:
			return "fadingReportedTag\0";
		case fadingConfirmedTag:
			return "fadingConfirmedTag\0";
		case goneTag:
			return "goneTag\0";
		case sumTag:
			return "sumTag\0";
		default:
			return "unknown?\0";
	}
}

void PegNode::oss_report_out (char * buf, word fmt) {

	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Tag %u at Peg %u at %lu: %s(%u)"\
					" with rss:pl (%u:%u)\r\n",
				*((word*)&in_report(buf, tagId) +1), 
				in_header(buf, snd),
				in_report(buf, tStamp),
				stateName (in_report(buf, state)),
				in_report(buf, state),
				(*(word*)&in_report(buf, tagId)) >>8,
				(*(word*)&in_report(buf, tagId)) & 0x00ff);
			break;

		case OSS_TCL:
			lbuf = form (NULL, "Type%u Peg%u T%u %u %u %lu %u\n",
				in_header(buf, msg_type),
				in_header(buf, snd),
				*((word*)&in_report(buf, tagId) +1),
				(*(word*)&in_report(buf, tagId)) & 0x00ff,
				(*(word*)&in_report(buf, tagId)) >>8,
				in_report(buf, tStamp),
				in_report(buf, state) == goneTag ? 0 : 1);
			break;

		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (peg_oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void PegNode::oss_setTag_out (char * buf, word fmt) {

	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
		case OSS_TCL:
			lbuf = form (NULL,
				"Tag %u at Peg %u Set ack %u ack seq %u\r\n", 
					in_setTagAck(buf, tag),
						(local_host == master_host ?
						in_header(buf, snd) :
						in_header(buf, rcv)),
						in_setTagAck(buf, ack),
						in_setTagAck(buf, ackSeq));
			break;
			// do nothing
			//return;
		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (peg_oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void PegNode::oss_getTag_out (char * buf, word fmt) {

	char * lbuf = NULL;

	switch (fmt) {
		case OSS_HT:
		case OSS_TCL:
			lbuf = form (NULL,
				"Tag %u at Peg %u Get time %lu host id %x%x"
				" pow lev %x freq maj %u freq min %u span %u"
				" ack seq %u\r\n", 
				in_getTagAck(buf, tag),
				(in_getTagAck(buf, tag) != in_header(buf, snd) ?
				in_header(buf, snd) : in_header(buf, rcv)),
				in_getTagAck(buf, ltime),
				in_getTagAck(buf, host_id),
				in_getTagAck(buf, pow_levels),
				in_getTagAck(buf, freq_maj),
				in_getTagAck(buf, freq_min),
				in_getTagAck(buf, rx_span),
				in_getTagAck(buf, ackSeq));
			break;
			// do nothing
			//return;
		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;
	}
	if (fork (peg_oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
}

void PegNode::oss_findTag_in (word state, lword tag, nid_t peg) {

	char * out_buf = NULL;
	int tagIndex;

	if (peg == local_host || peg == 0) {
		if (tag == 0) { // summary
			msg_report_out (state, -1, &out_buf);
		} else {
			tagIndex = find_tag (tag);
			msg_report_out (state, tagIndex, &out_buf);
			// as in msg_findTag_in, kludge summary into
			// missing tag:
			if (tagIndex < 0) {
				in_report(out_buf, tagId) = tag;
				in_report(out_buf, state) = noTag;
			}
		}
		in_header(out_buf, snd) = local_host;
		oss_report_out (out_buf, oss_fmt);

	}

	if (peg != local_host) {
		msg_findTag_out (state, &out_buf, tag, peg);
		send_msg (out_buf, sizeof(msgFindTagType));
	}
	ufree (out_buf);
}

void PegNode::oss_getTag_in (word state, lword tag, nid_t peg, lword pass) {
	
	char * out_buf = NULL;
	char * get_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgGetTagType);
	
	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd 
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	get_buf = out_buf + sizeof(msgFwdType);
	in_header(get_buf, msg_type) = msg_getTag;
	in_header(get_buf, rcv) = (nid_t)tag;
	in_header(get_buf, snd) = local_host;
	in_getTag(get_buf, passwd) = pass;
	if (peg == local_host) {
		//	it is local - put it in the wroom	
		msg_fwd_in(state, out_buf, size);
	} else {	
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}

void PegNode::oss_master_in (word state, nid_t peg) {

	char * out_buf = NULL;

	// shortcut to set master_host
	if (local_host == peg) {
		master_host = peg;
		return;
	}

	msg_master_out (state, &out_buf, peg);
	send_msg (out_buf, sizeof(msgMasterType));
	ufree (out_buf);
}

void PegNode::oss_setTag_in (word state, lword tag, nid_t peg, lword pass,
	       nid_t nid, word maj, word min, word pl, word span, lword npass) {

	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetTagType);

	if(peg == 0 || tag == 0)
	   return;
	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, tag, peg, pass);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setTag;
	in_header(set_buf, rcv) = (nid_t)tag;
	in_header(set_buf, snd) = local_host;
	in_setTag(set_buf, passwd) = pass;
	in_setTag(set_buf, npasswd) = npass;
	in_setTag(set_buf, node_addr) = nid;
	in_setTag(set_buf, pow_levels) = pl;
	in_setTag(set_buf, freq_maj) = maj;
	in_setTag(set_buf, freq_min) = min;
	in_setTag(set_buf, rx_span) = span;
	if (peg == local_host) {
		//	it is local - put it in the wroom
		msg_fwd_in(state, out_buf, size);
	} else {
		send_msg (out_buf,  size);
	}
	ufree (out_buf);
}

void PegNode::oss_setPeg_in (word state, nid_t peg, nid_t nid, word pl,
								    char *str) {
	char * out_buf = NULL;
	char * set_buf = NULL;
	int size = sizeof(msgFwdType) + sizeof(msgSetPegType);

	// alloc and prepare msg fwd
	msg_fwd_out (state, &out_buf, size, 0, peg, 0);
	in_header(out_buf, snd) = local_host;
	// get offset for payload - getTag msg
	set_buf = out_buf + sizeof(msgFwdType);
	in_header(set_buf, msg_type) = msg_setPeg;
	in_header(set_buf, rcv) = 0;
	in_header(set_buf, snd) = local_host;
	in_setPeg(set_buf, level) = pl;
	in_setPeg(set_buf, new_id) = nid;
	memcpy(in_setPeg(set_buf, str), str, 16);
	if (peg != local_host) {
		send_msg (out_buf,  size);
	} else {
		msg_fwd_in(state, out_buf, size);
	}
	ufree (out_buf);
}

peg_oss_out::perform {

	state OO_RETRY:

		S->ser_out (OO_RETRY, data);
		ufree (data);
		terminate;
}

int PegNode::tr_offset (headerType * mb) {

	excptn ("tr_offset undefined, not supposed to be called");
}

void PegNode::appStart () {

	fork (peg_root, NULL);
}

void buildPegNode (
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
		create PegNode (
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
