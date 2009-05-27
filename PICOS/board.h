#ifndef	__picos_board_h__
#define	__picos_board_h__

#include "picos.h"
#include "ndata.h"
#include "agent.h"
#include "nvram.h"
#include "tcv.h"
#include "tcvphys.h"
#include "wchan.h"
#include "tarp.h"
#include "plug_tarp.h"
#include "plug_null.h"

#define	N_MEMEVENT	0xFFFF0001
#define	PMON_CNTEVENT	0xFFFF0002
#define	PMON_NOTEVENT	0xFFFF0003

#define	PMON_STATE_NOT_RISING	0x01
#define	PMON_STATE_NOT_ON	0x02
#define	PMON_STATE_NOT_PENDING	0x04
#define	PMON_STATE_CNT_RISING	0x10
#define	PMON_STATE_CNT_ON	0x20
#define	PMON_STATE_CMP_ON	0x40
#define	PMON_STATE_CMP_PENDING	0x80

#define	UART_IMODE_D		0	// Direct UART mode
#define	UART_IMODE_P		1	// PHY (TCV) mode

#define	TheNode		((PicOSNode*)TheStation)
#define	ThePckt		((PKT*)ThePacket)

#define	MAX_LINE_LENGTH	63	// For d_uart_inp_p

int zz_running (void*), zz_crunning (void*);
int zz_killall (void*);
void zz_panel_signal (Long);
Boolean zz_validate_uart_rate (word);

extern	const char zz_hex_enc_table [];

void	syserror (int, const char*);

struct mem_chunk_struct	{

	struct	mem_chunk_struct	*Next;

	address	PTR;		// The address
	word	Size;		// The simulated size in full words
};

typedef	struct mem_chunk_struct	MemChunk;

class uart_dir_int_t {
//
// UART interface in direct mode
//
	public:

	char	*__inpline;
	Process	*pcsInserial, *pcsOutserial;

	// After reset
	void init () {
		__inpline = NULL;
		pcsInserial = pcsOutserial = NULL;
	};

	// Before halt; note: this is unused at present (no action is
	// needed to halt the UART, other than killing the driver processes,
	// but we may want to do something in the future
	void abort () { };

	uart_dir_int_t () { init (); };

};

class uart_tcv_int_t {
//
// UART interface for TCV PHY mode
//
	public:

	// === low level packet driver ========================================

	TIME	r_rstime;	// Time to wait until if resetting receiver
	int	x_qevent;	// Queue event id returned by TCV
	byte	rx_off, tx_off;	// On/off flags
	word	v_statid,	// station ID
		v_physid,	// PHY Id
		r_buffs,	// Number of bytes remaining to read
		r_buffl,	// Input buffer length
		x_buffl;	// Output buffer length
	address	r_buffer,	// Input buffer
		x_buffer;	// Output buffer
	byte	*r_buffp,	// Input buffer pointer
		*x_buffp;	// Output buffer pointer

	// === XRS ============================================================

	// This part accommodates a portion of AB (i.e., XRS), namely, the
	// bits that are not so local to the driver process. Tchnically,
	// this should be kept separate from the packet UART driver. It may
	// be OK, though, as XRS is a natural next layer to be used by the
	// praxis on top of packet UART. The dilemma is this: should be put
	// OEP into the same basket? Probably I will do it for now.

	Boolean	ab_new;
	byte	ab_md;
	char	*ab_cout, *ab_cin;

	void init () {
		// At reset
		bzero (this, sizeof (uart_tcv_int_t));
	};

	void abort ();

	uart_tcv_int_t () { init (); };
};

typedef	struct {
/*
 * The UART stuff. Instead of having all these as individual node attributes,
 * we create a structure encapsulating them. This is because most nodes will
 * have no UARTs, and the attributes would be mostly wasted.
 */

	byte	IMode;	// Interface mode

	UART	*U;	// Low-level (mode-independent) UART
	void	*Int;	// Interface
} uart_t;

#define	UART_INTF_D(u)	((uart_dir_int_t*)((u)->Int))
#define	UART_INTF_P(u)	((uart_tcv_int_t*)((u)->Int))

packet	PKT {

	word	*Payload;
	word	PaySize;

	void load (word *pay, int paysize) {
		// Note that paysize is in bytes and must be even. This is
		// called just before transmission.
		assert (paysize >= 2, "PKT: illegal payload size: %1d",
			paysize);
		Payload = new word [(PaySize = paysize)/2];
		memcpy (Payload, pay, paysize);
		// Assuming the checksum falls into the payload (but excluding
		// the preamble). This is in bits, according to the rules of
		// SMURPH. Perhaps some day we will take advantage of some of
		// its statistics collection tools?
		ILength = TLength = (paysize << 3);
	};
};

class rfm_intd_t {
//
// RF interface
//
	public: // ============================================================

	// Defaults needed for reset
	double		DefRPower;	// Receiver boost

	word		DefXPower,	// These are indexes
			DefRate,
			DefChannel;

	word		statid;

	Transceiver	*RFInterface;
	PKT		OBuffer;
	Boolean		Receiving, Xmitting, TXOFF, RXOFF;
	address		zzx_buffer, zzr_buffer;
	int		tx_event;

	double		lbt_threshold;

	word		min_backoff, max_backoff, backoff;
	word		lbt_delay;
	word		phys_id;

	rfm_intd_t (const data_no_t*);

	// Low-level setrate/setchannel
	void setrfpowr (word);
	void setrfrate (word);
	void setrfchan (word);

	// After reset
	void init ();

	// Before halt
	void abort ();

};

station PicOSNode abstract {

	void		_da (phys_dm2200) (int, int);
	void		_da (phys_cc1100) (int, int);
	void		phys_rfmodule_init (int, int);
	void		_da (phys_uart) (int, int, int);

	long		SecondOffset;

	Mailbox	TB;		// For trigger

	/*
	 * Memory allocator
	 */
	MemChunk	*MHead, *MTail;
	word		MTotal, MFree,
			NFree;		// Minimum free so far - for stats

	// Current number of processes + limit; note: a single (countdown) 
	// value could do theoretically, but then we would need a 'default"
	// for reset
	word		NPcss, NPcLim;

	/*
	 * RF interface
	 */
	rfm_intd_t	*RFInt;

	lword		_da (entropy);

	/*
	 * One more Boolean flag - to tell if the node is halted; we may want
	 * to reorganize this a bit later (like into a bunch of binary flags
	 * perhaps?
	 *
	 */
	Boolean		Halted;

	/*
	 * This is NULL if the node has no UART
	 */
	uart_t		*uart;

	/*
	 * Pins
	 */
	PINS		*pins;

	/*
	 * Leds
	 */
	LEDSM		*ledsm;

	/*
	 * Sensors/actuators
	 */
	SNSRS		*snsrs;

	/*
	 * This is EEPROM and FIM (IFLASH)
	 */
	NVRAM		*eeprom, *iflash;

	void _da (diag) (const char*, ...);

	//
	// Here comes the 'reset' mess:
	//

	// Internal, resets the UART
	void uart_reset (), uart_abort ();

	// Location
	inline void get_location (double &xx, double &yy) {
		if (RFInt != NULL)
			RFInt->RFInterface->getLocation (xx, yy);
		else
			xx = yy = 0.0;
	};

	inline void set_location (double xx, double yy) {
		if (RFInt != NULL)
			RFInt->RFInterface->setLocation (xx, yy);
	};

	// This one is called by the praxis to reset the node
	void _da (reset) ();
	// This one halts the node (from the praxis)
	void _da (halt) ();
	// This one stops the node (from outside the praxis)
	void stopall ();
	// This is type specific reset; also called by the agent to halt the
	// node
	virtual void reset ();
	// This is type specific init (usually defined in the bottom type
	// to start the root process); also called by the agent to switch
	// the node on (after switchOff)
	virtual void init () { };

	void initParams ();

	int _da (getpid) () { return __cpint (TheProcess); };
	lword _da (seconds) ();
	void _da (setseconds) (lword);
	word _da (sectomin) ();
	address	memAlloc (int, word);
	void memFree (address);
	word _da (actsize) (address);
	Boolean memBook (word);
	void memUnBook (word);
	inline void waitMem (int state) { TB.wait (N_MEMEVENT, state); };

	word _da (memfree) (int, word*);

	inline Boolean tally_in_pcs () {
		if (NPcLim == 0)
			return YES;
		if (NPcss >= NPcLim)
			return NO;
		NPcss++;
		return YES;
	}

	inline void tally_out_pcs () {
		if (NPcss != 0)
			NPcss--;
		TB.signal (N_MEMEVENT);
	};

	inline void _da (delay) (word msec, int state) {
		Timer->delay (msec * MILLISECOND, state);
	};

	inline void _da (when) (int ev, int state) { TB.wait (ev, state); };

	inline void _da (leds) (word led, word op) {
		if (ledsm != NULL)
			// Ignore otherwise
			ledsm->leds_op (led, op);
	};

	inline void _da (fastblink) (Boolean a) {
		if (ledsm != NULL)
			ledsm->fastblink (a);
	};

	inline int _da (io) (int state, int dev, int ope, char *buf, int len) {
		// Note: 'dev' is ignored: it exists for compatibility with
		// PicOS; io only works for the (single) UART (in direct mode)
		assert (uart != NULL, "PicOSNode->io: node %s has no UART",
			getSName ());
		return uart->U->ioop (state, ope, buf, len);
	};

	void _da (ldelay) (word, int);
	void _da (lhold) (int, lword*);

	/*
	 * I/O formatting
	 */
	char * _da (vform) (char*, const char*, va_list);
	int    _da (vscan) (const char*, const char*, va_list);
	char * _da (form) (char*, const char*, ...);
	int    _da (scan) (const char*, const char*, ...);
	int    _da (ser_out) (word, const char*);
	int    _da (ser_outb) (word, const char*);
	int    _da (ser_in) (word, char*, int);
	int    _da (ser_outf) (word, const char*, ...);
	int    _da (ser_inf) (word, const char*, ...);

	/*
	 * XRS operations
	 */
	void   _da (ab_init) (int);
	void   _da (ab_mode) (byte);
	void   _da (ab_outf) (word, const char*, ...);
	void   _da (ab_out) (word, char*);
	int    _da (ab_inf) (word, const char*, ...);
	char * _da (ab_in) (word);

	/*
	 * Operations on pins
	 */
	void no_pin_module (const char*);

	inline word  _da (pin_read) (word pn) {
		if (pins == NULL)
			no_pin_module ("pin_read");
		return pins->pin_read (pn);
	};

	inline int   _da (pin_write) (word pn, word val) {
		if (pins == NULL)
			no_pin_module ("pin_write");
		return pins->pin_write (pn, val);
	};

	inline int   _da (pin_read_adc) (word st, word pn, word ref, word smt) {
		if (pins == NULL)
			no_pin_module ("pin_read_adc");
		return pins->pin_read_adc (st, pn, ref, smt);
	};

	inline int   _da (pin_write_dac) (word pn, word val, word ref) {
		if (pins == NULL)
			no_pin_module ("pin_write_dac");
		return pins->pin_write_dac (pn, val, ref);
	};

	// The pulse monitor

	inline void  _da (pmon_start_cnt) (long cnt, Boolean edge) {
		if (pins == NULL)
			no_pin_module ("pmon_start_cnt");
		pins->pmon_start_cnt (cnt, edge);
	};

	inline void  _da (pmon_stop_cnt) () {
		if (pins == NULL)
			no_pin_module ("pmon_stop_cnt");
		pins->pmon_stop_cnt ();
	};

	inline void  _da (pmon_set_cmp) (long cnt) {
		if (pins == NULL)
			no_pin_module ("pmon_set_cmp");
		pins->pmon_set_cmp (cnt);
	};

	inline lword _da (pmon_get_cnt) () {
		if (pins == NULL)
			no_pin_module ("pmon_get_cnt");
		return pins->pmon_get_cnt ();
	};

	inline lword _da (pmon_get_cmp) () {
		if (pins == NULL)
			no_pin_module ("pmon_get_cmp");
		return pins->pmon_get_cmp ();
	};

	inline void  _da (pmon_start_not) (Boolean edge) {
		if (pins == NULL)
			no_pin_module ("pmon_start_not");
		pins->pmon_start_not (edge);
	};

	inline void  _da (pmon_stop_not) () {
		if (pins == NULL)
			no_pin_module ("pmon_stop_not");
		pins->pmon_stop_not ();
	};

	inline word  _da (pmon_get_state) () {
		if (pins == NULL)
			no_pin_module ("pmon_get_state");
		return pins->pmon_get_state ();
	};

	inline Boolean  _da (pmon_pending_not) () {
		if (pins == NULL)
			no_pin_module ("pmon_pending_not");
		return pins->pmon_pending_not ();
	};

	inline Boolean  _da (pmon_pending_cmp) () {
		if (pins == NULL)
			no_pin_module ("pmon_pending_cmp");
		return pins->pmon_pending_cmp ();
	};

	inline void  _da (pmon_dec_cnt) () {
		if (pins == NULL)
			no_pin_module ("pmon_dec_cnt");
		pins->pmon_dec_cnt ();
	};

	inline void  _da (pmon_sub_cnt) (long decr) {
		if (pins == NULL)
			no_pin_module ("pmon_sub_cnt");
		pins->pmon_sub_cnt (decr);
	};

	inline void  _da (pmon_add_cmp) (long incr) {
		if (pins == NULL)
			no_pin_module ("pmon_add_cmp");
		pins->pmon_add_cmp (incr);
	};

	/*
	 * Sensors/actuators
	 */
	void no_sensor_module (const char*);

	inline void _da (read_sensor) (int st, word sn, address val) {
		if (snsrs == NULL)
			no_sensor_module ("read_sensor");
		snsrs->read (st, sn, val);
	}
	inline void _da (write_actuator) (int st, word sn, address val) {
		if (snsrs == NULL)
			no_sensor_module ("write_actuator");
		snsrs->write (st, sn, val);
	}

	/*
	 * EEPROM + FIM (IFLASH)
	 */
	lword _da (ee_size) (Boolean*, lword*);
	word _da (ee_read)  (lword, byte*, word);
	word _da (ee_erase) (word, lword, lword);
	word _da (ee_write) (word, lword, const byte*, word);
	word _da (ee_sync) (word);
	int  _da (if_write) (word, word);
	word _da (if_read)  (word);
	void _da (if_erase) (int);

#include "encrypt.h"
	// Note: static TCV data is initialized in tcv_init.
#include "tcv_node_data.h"

	void setup (data_no_t*);

	IPointer preinit (const char*);
};

// === UART direct ============================================================

process d_uart_inp_p (PicOSNode) {

	uart_dir_int_t *f;
	char *tmp, *ptr;
	int len;

	states { IM_INIT, IM_READ, IM_BIN, IM_BIN1 };

	void setup ();
	void close ();

	perform;
};

process d_uart_out_p (PicOSNode) {

	uart_dir_int_t *f;
	const char *data, *ptr;
	int len;

	states { OM_INIT, OM_WRITE };

	void setup (const char*);
	void close ();

	perform;
};

// === UART packet ============================================================

process p_uart_rcv_p (PicOSNode) {

	uart_tcv_int_t *UA;

	states { RC_LOOP, RC_WLEN, RC_FILL, RC_OFFSTATE, RC_WOFF, RC_RESET,
		RC_WRST };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;

	byte getbyte (int, int);
	void rdbuff (int, int, int);
	void ignore (int, int);
	void rreset (int, int);
};

process p_uart_xmt_p (PicOSNode) {

	uart_tcv_int_t *UA;

	states { XM_LOOP, XM_PRE, XM_LEN, XM_SEND };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

// === XRS ====================================================================

process ab_driver_p (PicOSNode) {

	uart_tcv_int_t *UA;

	int	SID;			// Session ID

	address	packet;

	byte	ab_cur, ab_exp;

	states { AB_LOOP, AB_RCV };

	perform;

	void setup (uart_tcv_int_t*, int);

	Boolean ab_send (int);
	void ab_receive ();
};

// ============================================================================

station NNode abstract : PicOSNode {
/*
 * A node equipped with NULL plugin
 */
#include "plug_null_node_data.h"

	void setup ();
	virtual void reset ();
};

station TNode abstract : PicOSNode {
/*
 * A node equipped with TARP stuff
 */

// I have added the NULL plug, which costs nothing (one integer variable) in
// terms of attributes, such that we can use it together with TARP, if desired;
// this will be needed, say, for XRS; clumsy, clumsy, clumsy ... what about
// the OEP plugin? Wouldn't it be better to have a separate node type? Yes,
// I guess it would ... later. 
// Note: don't modify PicOS includes for multiple inclusion (this can only
// result in a mess); use #undefs, like the one below

#undef __plug_null_node_data_h
#include "plug_null_node_data.h"
#include "net_node_data.h"
#include "plug_tarp_node_data.h"
#include "tarp_node_data.h"

	void setup ();
	virtual void reset ();
};

process	BoardRoot {

	data_no_t *readNodeParams (sxml_t, int, const char*);
	data_ua_t *readUartParams (sxml_t, const char*);
	data_pn_t *readPinsParams (sxml_t, const char*);
	data_sa_t *readSensParams (sxml_t, const char*);
	data_le_t *readLedsParams (sxml_t, const char*);

	void initTiming (sxml_t);
	int initChannel (sxml_t, int, Boolean);
	void initNodes (sxml_t, int, int);
	void initPanels (sxml_t);
	void initRoamers (sxml_t);
	void initAll ();

	void readPreinits (sxml_t, int);
	
	virtual void buildNode (const char *tp, data_no_t *nddata) {
		excptn ("BoardRoot: buildNode undefined");
	};

	states { Start, Stop } ;

	perform;
};

process MoveHandler {

	TIME TimedRequestTime;

	union	{
		Dev	   *Agent;	// May be string
		const char *String;
	};

	int Left;
	char *BP;
	char *RBuf;
	word RBSize;
	FLAGS Flags;

	states { AckMove, Loop, ReadRq, Reply, Delay };

	void setup (Dev*, FLAGS);

	~MoveHandler ();

	perform;
};

process PanelHandler {

	TIME TimedRequestTime;

	union	{
		Dev	   *Agent;	// May be string
		const char *String;
	};

	int Left;
	char *BP;
	char *RBuf;
	word RBSize;
	FLAGS Flags;

	states { AckPanel, Loop, ReadRq, Reply, Delay };

	void setup (Dev*, FLAGS);

	~PanelHandler ();

	perform;
};

#endif
