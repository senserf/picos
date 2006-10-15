#ifndef	__node_tag_h__
#define	__node_tag_h__

#include "app_tag.h"
#include "board.h"
#include "chan_shadow.h"
#include "diag.h"

station TagNode : TNode {


	char * ui_ibuf /* = NULL */;
	char * ui_obuf /* = NULL */;

	// command line and its semaphores
	char * cmd_line	/* = NULL */;

#define CMD_READER	((int)(((char*)TheNode)+1))
#define CMD_WRITER	((int)(((char*)TheNode)+2))
#define RX_SW_ON 	((int)(((char*)TheNode)+3))

	pongParamsType pong_params /* = {	5120, 	// freq_maj
						1024, 	// freq_min
						0x0987, // 7-8-9 (3 levels)
						1024,	// rx_span
						7	// rx_lev
					} */ ;

	word	app_flags 	/* = 0 */;
	lword	host_passwd 	/* = 0 */;
	lword	host_id 	/* = 0xBACA0061 */; // ESN burned (virtually?)

	appCountType app_count 	/* = {0, 0, 0} */;

	void stats ();
	void process_incoming (word, char*, word);

	char *get_mem (word state, int len);
	void send_msg (char * buf, int size);
	word max_pwr (word p_levs);
	void set_tag (char * buf);
	word check_passwd (lword p1, lword p2);

	void msg_getTag_in (word state, char *buf);
	void msg_getTagAck_out (word, char**, nid_t, seq_t, word);
	void msg_setTag_in (word state, char * buf);
	void msg_setTagAck_out (word, char**, nid_t, seq_t, word);

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

#define	TheTag	((TagNode*)TheNode)

process tag_rcv (TagNode) {

	int packet_size;
	char * buf_ptr;

	states { RS_TRY, RS_MSG };

	void setup (void *dummy) {
		packet_size = 0;
		buf_ptr = NULL;
	};

	perform;
};

process tag_rxsw (TagNode) {

	states { RS_OFF, RS_ON };

	void setup (void *dummy) { };

	perform;
};

process tag_pong (TagNode) {

	char	frame [sizeof (msgPongType)];
	word	shift;

	states { PS_INIT, PS_NEXT, PS_SEND, PS_SENDP2 };

	void setup (void *dummy) { };

	perform;
};

process tag_cmd_in (TagNode) {

	states { CS_INIT, CS_IN, CS_WAIT };

	void setup (void *dummy) { };

	perform;
};

process tag_root (TagNode) {

	states { RS_INIT, RS_FREE, RS_RCMD, RS_DOCMD, RS_UIOUT };

	void setup (void *dummy) { };

	perform;
};

#endif
