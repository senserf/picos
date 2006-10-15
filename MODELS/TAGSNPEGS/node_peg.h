#ifndef	__node_tag_h__
#define	__node_tag_h__

#include "app_peg.h"
#include "board.h"
#include "chan_shadow.h"
#include "diag.h"

station PegNode : TNode {


	char * ui_ibuf /* = NULL */;
	char * ui_obuf /* = NULL */;

	// command line and its semaphores
	char * cmd_line	/* = NULL */;

#define CMD_READER	((int)(((char*)TheNode)+1))
#define CMD_WRITER	((int)(((char*)TheNode)+2))

	word	app_flags 	/* = 0 */;
	lword	host_passwd 	/* = 0 */;
	lword	host_id 	/* = 0xBACA0001 */; // ESN
	long	master_delta 	/* = 0 */;
	word	host_pl 	/* = 9 */;
	word	tag_auditFreq 	/* = 10240 */; // in bin msec
	word	tag_eventGran 	/* = 10 */;  // in seconds

	// if we can get away with it, it's better to have it in IRAM (?)
	tagDataType tagArray [tag_lim];

	wroomType msg4tag 	/* = {NULL, 0} */;
	wroomType msg4ward 	/* = {NULL, 0} */;

	appCountType app_count 	/* = {0, 0, 0} */;

	int find_tag (lword tag);
	char *get_mem (word state, int len);
	void init_tag (word i);
	void init_tags ();
	void set_tagState (word i, tagStateType state, bool updEvTime);
	int insert_tag (lword tag);
	void check_tag (word state, word i, char** buf_out);
	void copy_fwd_msg (word state, char** buf_out, char * buf, word size);
	void send_msg (char * buf, int size);
	int check_msg_size (char * buf, word size, word repLevel);
	void check_msg4tag (nid_t tag);
	
	void stats ();
	void process_incoming (word state, char * buf, word size, word rssi);
	
	word countTags();
	void msg_report_out (word state, int tIndex, char** out_buf);
	void msg_findTag_in (word state, char * buf);
	void msg_findTag_out (word state, char** buf_out, lword tag, nid_t peg);
	void msg_fwd_in (word state, char * buf, word size);
	void msg_fwd_out (word, char**, word, lword, nid_t, lword);
	void msg_getTagAck_in (word state, char * buf, word size);
	void msg_master_in (char * buf);
	void msg_master_out (word state, char** buf_out, nid_t peg);
	void msg_pong_in (word state, char * buf, word rssi);
	void msg_report_in (word state, char * buf);
	void msg_reportAck_in (char * buf);
	void msg_reportAck_out (word state, char * buf, char** out_buf);
	void msg_setTagAck_in (word state, char * buf, word size);
	
	char * stateName (unsigned state);
	void oss_report_out (char * buf, word fmt);
	void oss_setTag_out (char * buf, word fmt);
	void oss_getTag_out (char * buf, word fmt);
	void oss_findTag_in (word state, lword tag, nid_t peg);
	void oss_getTag_in (word state, lword tag, nid_t peg, lword pass);
	void oss_master_in (word state, nid_t peg);
	void oss_setTag_in (word, lword, nid_t, lword, nid_t, word, word, word,
		word, lword);
	void oss_setPeg_in (word, nid_t, nid_t, word, char*);
	

	int tr_offset (headerType *mb);

	virtual bool msg_isBind (msg_t m) { return NO; };
	virtual bool msg_isTrace (msg_t m) { return NO; };
	virtual bool msg_isMaster (msg_t m) { m == msg_master; };
	virtual bool msg_isNew (msg_t m) { return NO; }
	virtual bool msg_isClear (byte o) { return YES; };
	virtual void set_master_chg () { app_flags |= 2; };

	/*
	 * Application starter
	 */
	void appStart ();

	void setup (
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
	);
};

#define	ThePeg	((PegNode*)TheNode)

process peg_oss_out (PegNode) {

	char *data;

	states { OO_RETRY };

	void setup (char *oc) {
		data = oc;
	};

	perform;
};

process peg_rcv (PegNode) {

	int packet_size;
	char * buf_ptr;
	word rssi;

	states { RS_TRY, RS_MSG };

	void setup (void *dummy) {
		packet_size = 0;
		buf_ptr = NULL;
		rssi = 0;
	};

	perform;
};

process peg_audit (PegNode) {

	char *buf_ptr;
	word ind;

	states { AS_START, AS_TAGLOOP, AS_TAGLOOP1 };

	void setup (void *dummy) {
		buf_ptr = NULL;
	};

	perform;
};

process peg_cmd_in (PegNode) {

	states { CS_INIT, CS_IN, CS_WAIT };

	void setup (void *dummy) { };

	perform;
};

process peg_root (PegNode) {

	lword	in_tag, in_pass, in_npass;
	nid_t	in_peg;
	word	in_rssi, in_pl, in_maj, in_min, in_span;

	states { RS_INIT, RS_FREE, RS_RCMD, RS_DOCMD, RS_MEM, RS_UIOUT };

	void setup (void *dummy) { };

	perform;
};

#endif
