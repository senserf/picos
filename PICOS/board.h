#ifndef	__picos_board_h__
#define	__picos_board_h__

#include "picos.h"
#include "agent.h"
#include "nvram.h"
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

void	syserror (int, const char*);

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

	void		_da (phys_dm2200) (int, int);

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
			NFree;		// Minimum free so far - for stats

	/*
	 * RF interface
	 */
	PKT		_da (OBuffer);	// Output buffer
	Transceiver	*_da (RFInterface);

	/*
	 * RF interface component. We may want to modify it later, if it turns
	 * out to be dependent on RFModule.
	 */
	Boolean		_da (Receiving), _da (Xmitting),
			_da (TXOFF), _da (RXOFF);
	int		_da (tx_event);
	lword		_da (entropy);
	word		_da (statid);		// Station/network ID
	word		_da (min_backoff), _da (max_backoff), _da (backoff);
	word		_da (lbt_delay);
	double		_da (lbt_threshold);
	
	/*
	 * This is NULL if the node has no UART
	 */
	uart_t		*uart;

	/*
	 * This is EEPROM and FIM (IFLASH)
	 */
	NVRAM		*eeprom, *iflash;

	void _da (diag) (const char*, ...);
	void reset ();
	int _da (getpid) () { return (int) TheProcess; };
	lword _da (seconds) ();
	address	memAlloc (int, word);
	void memFree (address);
	word _da (actsize) (address);
	Boolean memBook (word);
	void memUnBook (word);
	word memfree (int pool, word *faults);
	inline void waitMem (int state) { TB.wait (N_MEMEVENT, state); };
	inline void _da (delay) (word msec, int state) {
		Timer->delay (msec * MILLISECOND, state);
	};
	inline void _da (when) (int ev, int state) { TB.wait (ev, state); };
	inline void _da (gbackoff) () {
		_da (backoff) = _da (min_backoff) + toss (_da (max_backoff));
	};

	inline int _da (io) (int state, int dev, int ope, char *buf, int len) {
		// Note: 'dev' is ignored: it exists for compatibility with
		// PicOS; io only works for the (single) UART.
		assert (uart != NULL, "PicOSNode->io: node %s has no UART",
			getSName ());
		return uart->U->ioop (state, ope, buf, len);
	};

	/*
	 * I/O formatting
	 */
	char * _da (vform) (char*, const char*, va_list);
	int _da (vscan) (const char*, const char*, va_list);
	char * _da (form) (char*, const char*, ...);
	int _da (scan) (const char*, const char*, ...);
	int _da (ser_out) (word, const char*);
	int _da (ser_in) (word, char*, int);
	int _da (ser_outf) (word, const char*, ...);
	int _da (ser_inf) (word, const char*, ...);

	/*
	 * EEPROM + FIM (IFLASH)
	 */
	void _da (ee_read)  (word, byte*, word);
	void _da (ee_erase) ();
	void _da (ee_write) (word, const byte*, word);
	int  _da (if_write) (word, word);
	word _da (if_read)  (word);
	void _da (if_erase) (int);

#include "encrypt.h"
	// Note: static TCV data is initialized in tcv_init.
#include "tcv_node_data.h"

	void setup (word, double, double, double, double, Long, Long, Long,
		double, RATE, Long, Long, Long, Long, Long, Long, Long, char*,
			char*);

};

process Inserial (PicOSNode) {

	uart_t *uart;
	char *tmp, *ptr;
	int len;

	states { IM_INIT, IM_READ, IM_BIN, IM_BIN1 };

	void setup ();
	void close ();

	perform;
};

process Outserial (PicOSNode) {

	uart_t	*uart;
	const char *data, *ptr;
	int len;

	states { OM_INIT, OM_WRITE, OM_RETRY };

	void setup (const char*);
	void close ();

	perform;
};

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
	// Application-level parameters for TARP
	virtual int _da (tr_offset) (headerType *mb) {
		excptn ("TNode->tr_offset undefined");
	};
	virtual bool _da (msg_isBind) (msg_t m) {
		excptn ("TNode->msg_isBind undefined");
	};
	virtual bool _da (msg_isTrace) (msg_t m) {
		excptn ("TNode->msg_isTrace undefined");
	};
	virtual bool _da (msg_isMaster) (msg_t m) {
		excptn ("TNode->msg_isMaster undefined");
	};
	virtual bool _da (msg_isNew) (msg_t m) {
		excptn ("TNode->msg_isNew undefined");
	};
	virtual bool _da (msg_isClear) (byte o) {
		excptn ("TNode->msg_isClear undefined");
	};
	virtual void _da (set_master_chg) () {
		excptn ("TNode->set_master_chg undefined");
	};

#include "net_node_data.h"
#include "plug_tarp_node_data.h"
#include "tarp_node_data.h"

	void setup ();
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
		Long	eesz,		// EEPROM size
		Long	ifsz,		// IFLASH size
		Long	ifps,		// IFLASH page size
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
