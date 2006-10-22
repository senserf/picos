#ifndef	__picos_board_h__
#define	__picos_board_h__

#include "picos.h"
#include "uart.h"
#include "tcv.h"
#include "tcvphys.h"
#include "chan_radio.h"
#include "tarp.h"
#include "plug_tarp.h"
#include "plug_null.h"

#define	N_MEMEVENT	0xFFFF0001

#define	TheNode		((PicOSNode*)TheStation)
#define	ThePckt		((PKT*)ThePacket)

#define	MAX_LINE_LENGTH	63	// For Inserial


extern	const char zz_hex_enc_table [];

struct mem_chunk_struct	{

	struct	mem_chunk_struct	*Next;

	address	PTR;		// The address
	word	Size;		// The simulated size in full words
};

typedef	struct mem_chunk_struct	MemChunk;

typedef	struct {
/*
 * The UART stuff. Instead of having all these as individual node attributes,
 * we create a structure encapsulating them. This is because most nodes will
 * have no UARTs, and the attributes would be mostly wasted.
 */
	UART	*U;
	char	*__inpline;
	Process	*pcsInserial, *pcsOutserial;
} uart_t;

packet	PKT {

	word	*Payload;
	word	PaySize;

	void load (word *pay, int paysize) {
		// Note that paysize is in bytes and must be even. This is
		// called just before transmission.
		Payload = new word [(PaySize = paysize)/2];
		memcpy (Payload, pay, paysize);
		// Assuming the checksum falls into the payload (but excluding
		// the preamble)
		ILength = TLength = paysize * Ether->BitsPerByte + 
			Ether->PacketFrameLength;
	};
};

station PicOSNode {

	void		phys_dm2200 (int, int);

	Mailbox	TB;		// For trigger

	/*
	 * Defaults needed for reset
	 */
	double		DefXPower, DefRPower;

	/*
	 * Memory allocator
	 */
	MemChunk	*MHead, *MTail;
	word		MTotal, MFree,
			NFree;	// Minimum free so far - for stats

	/*
	 * RF interface component. We may want to modify it later, if it turns
	 * out to be dependent on RFModule.
	 */
	PKT		OBuffer;	// Output buffer
	Transceiver	*RFInterface;
	Boolean		Receiving, Xmitting, TXOFF, RXOFF;
	int		tx_event;
	lword		entropy;
	word		statid;		// Station/network ID
	word		min_backoff, max_backoff, backoff;
	word		lbt_delay;
	double		lbt_threshold;
	
	/*
	 * This is NULL if the node has no UART
	 */
	uart_t		*uart;

	void reset ();
	int getpid () { return (int) TheProcess; };
	lword seconds ();
	address	memAlloc (int, word);
	void memFree (address);
	word actsize (address);
	Boolean memBook (word);
	void memUnBook (word);
	word memfree (int pool, word *faults);
	inline void waitMem (int state) { TB.wait (N_MEMEVENT, state); };
	inline void delay (word msec, int state) {
		Timer->delay (msec * MILLISECOND, state);
	};
	inline void wait (int ev, int state) { TB.wait (ev, state); };
	inline void gbackoff () {
		backoff = min_backoff + toss (max_backoff);
	};

	inline int io (int state, int dev, int ope, char *buf, int len) {
		// Note: 'dev' is ignored: it exists for compatibility with
		// PicOS
		assert (uart != NULL, "PicOSNode->io: node %s has no UART",
			getSName ());
		return uart->U->io (state, ope, buf, len);
	};

	/*
	 * I/O formatting
	 */
	char *vform (char*, const char*, va_list);
	int vscan (const char*, const char*, va_list);
	char *form (char*, const char*, ...);
	int scan (const char*, const char*, ...);
	int ser_out (word, const char*);
	int ser_in (word, char*, int);
	int ser_outf (word, const char*, ...);
	int ser_inf (word, const char*, ...);

#include "encrypt.h"
	// Note: static TCV data is initialized in tcv_init.
#include "tcv_node_data.h"

	void setup (word, double, double, double, double, Long, Long, Long,
		double, RATE, long, Long, Long, Long, char*, char*);

};

process Inserial (PicOSNode) {

	uart_t *uart;
	char *tmp, *ptr;
	int len;

	states { IM_INIT, IM_READ, IM_BIN, IM_BIN1 };

	void setup ();
	void finish ();

	perform;
};

process Outserial (PicOSNode) {

	uart_t	*uart;
	const char *data, *ptr;
	int len;

	states { OM_INIT, OM_WRITE, OM_RETRY };

	void setup (const char*);
	void finish ();

	perform;
};

#if TCV_TIMERS
// We need a TCV process to handle timers
process timeserv (PicOSNode) {

	states { Run };

	perform {

		state Run:

			S->delay (runrq (), Run);
			sleep;
	};
};
#endif /* TCV_TIMERS */

station NNode : PicOSNode {
/*
 * A node equipped with NULL plugin
 */
#include "plug_null_node_data.h"

	void setup ();
	void reset ();
};

station TNode : PicOSNode {
/*
 * A node equipped with TARP stuff
 */

// These ones are supposed to be provided/set by the application. Why arent they
// simply TARP data settable by the application (via some 'init' call), which
// would be easier to translate to SMURPH? Another little problem is the bunch
// of macros normally stored in app_tarp_if.h (on the application's side), which
// I have moved temporarily to options.sys (yyyeeechhhh).

	nid_t	net_id 		/* = 85 */; // 0x55 set network id to any value
	nid_t	local_host 	/* = 97 */;
	nid_t   master_host 	/* = 1 */;

	// Defaults needed for reset

	nid_t	Def_net_id;
	nid_t	Def_local_host;
	nid_t	Def_master_host;

	// Application-level parameters for TARP
	virtual int tr_offset (headerType *mb) {
		excptn ("TNode->tr_offset undefined");
	};
	virtual bool msg_isBind (msg_t m) {
		excptn ("TNode->msg_isBind undefined");
	};
	virtual bool msg_isTrace (msg_t m) {
		excptn ("TNode->msg_isTrace undefined");
	};
	virtual bool msg_isMaster (msg_t m) {
		excptn ("TNode->msg_isMaster undefined");
	};
	virtual bool msg_isNew (msg_t m) {
		excptn ("TNode->msg_isNew undefined");
	};
	virtual bool msg_isClear (byte o) {
		excptn ("TNode->msg_isClear undefined");
	};
	virtual void set_master_chg () {
		excptn ("TNode->set_master_chg undefined");
	};

#include "net_node_data.h"
#include "plug_tarp_node_data.h"
#include "tarp_node_data.h"

	void setup (nid_t ni, nid_t lh, nid_t mh);
	void reset ();
};

process	BoardRoot {

	void readNodeParams (sxml_t, int,
		Long&,
		double&,
		double&,
		Long&,
		Long&,
		Long&,
		double&,
		Long&,
		Long&,
		Long&,
		Long&,
		Long&,
		Long&,
		Long&,
		Long&,
		Long&,
		char*&,
		char*&
	);

	void initTiming (sxml_t);
	void initChannel (sxml_t, int);
	void initNodes (sxml_t, int);
	void initAll ();

	virtual void buildNode (
		const char *tp,		// Type
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
		excptn ("BoardRoot: buildNode undefined");
	};

	states { Start, Stop } ;

	perform;
};

#endif
