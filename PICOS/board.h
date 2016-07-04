#ifndef	__picos_board_h__
#define	__picos_board_h__

#define	VUEE_VERSION	1.2

#ifndef	ZZ_R3D
// A fallback to compile legacy praxes
#define	ZZ_R3D	0
#endif

#include "picos.h"
#include "ndata.h"
#include "agent.h"
#include "nvram.h"
#include "tcv.h"
#include "tcvphys.h"
#include "wchan.h"
#include "encrypt.h"
#include "lcdg_n6100p_driver.h"

#include "lib_params.h"

#ifndef	VUEE_RESYNC_INTERVAL
// The default RT sync granularity in msecs
#define	VUEE_RESYNC_INTERVAL	500
#endif

#ifndef	VUEE_SLOMO_FACTOR
// The slow motion factor > 1.0 -> slow motion, i.e., RT is longer than virtual
#define	VUEE_SLOMO_FACTOR	1.0
#endif

#define	N_MEMEVENT	((IPointer)(-65537))
#define	PMON_CNTEVENT	((IPointer)(-65536))
#define	PMON_CMPEVENT	PMON_CNTEVENT
#define	PMON_NOTEVENT	((IPointer)(-65535))

#define	PMON_STATE_NOT_RISING	0x01
#define	PMON_STATE_NOT_ON	0x02
#define	PMON_STATE_NOT_PENDING	0x04
#define	PMON_STATE_CNT_RISING	0x10
#define	PMON_STATE_CNT_ON	0x20
#define	PMON_STATE_CMP_ON	0x40
#define	PMON_STATE_CMP_PENDING	0x80

#define	UART_IMODE_D		0	// Direct UART mode
#define	UART_IMODE_N		1	// PHY (TCV) N-mode (packet)
#define	UART_IMODE_P		2	// PHY (TCV) P-mode (packet, persistent)
#define	UART_IMODE_L		3	// PHY (TCV) mode (line)
#define	UART_IMODE_E		4	// PHY (TCV) mode (escaped, parity)
#define	UART_IMODE_F		5	// PHY (TCV) mode (escaped, checksum)

// Channel types
#define	CTYPE_SHADOWING		0
#define	CTYPE_SAMPLED		1
#define	CTYPE_NEUTRINO		2

#define	TheNode		((PicOSNode*)TheStation)
#define	ThePckt		((PKT*)ThePacket)
#define	ThePPcs		((_PP_*)TheProcess)

#define	MAX_LINE_LENGTH		63	// For d_uart_inp_p

typedef	void *code_t;

// ============================================================================

void __pi_panel_signal (Long);
Boolean __pi_validate_uart_rate (word);

extern	const char __pi_hex_enc_table [];
extern	int __pi_channel_type;

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

// ============================================================================

class uart_tcv_int_t {
//
// UART interface for TCV PHY packet and line modes
//
	public:

	TIME	r_rstime;	// Time to wait until if resetting receiver
	int	x_qevent;	// Queue event id returned by TCV
	word	v_flags,
		v_statid,	// station ID (used as spare length in P mode)
		v_physid,	// PHY Id
		r_buffs,	// Number of bytes remaining to read
		r_buffl,	// Input buffer length
		x_buffl;	// Output buffer length
	address	r_buffer,	// Input buffer
		x_buffer;	// Output buffer
	byte	*r_buffp,	// Input buffer pointer
		*x_buffp;	// Output buffer pointer

	void init () {
		// At reset
		memset (this, 0, sizeof (uart_tcv_int_t));
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

	UARTDV	*U;	// Low-level (mode-independent) UART
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

typedef struct {
//
// This is the constant part of the RF interface, so it can be shared by
// multiple nodes
//
	double	DefRPower,	// Receiver boost
		*lbt_threshold;

	word	DefXPower,	// These are indices
		DefRate,
		DefChannel,
		min_backoff,
		max_backoff,
		lbt_delay,
		lbt_tries;
} rfm_const_t;

class rfm_intd_t {
//
// RF interface
//
	public: // ============================================================

	rfm_const_t	*cpars;		// Constant parameters

	word		statid;

	int		MaxPL;

	Transceiver	*RFInterface;
	PKT		OBuffer;
	Boolean		Receiving, Xmitting, RXOFF;
	byte		LastPower;
	address		__pi_x_buffer, __pi_r_buffer;
	int		tx_event;

	word		backoff;
	word		phys_id;

#if (RADIO_OPTIONS & RADIO_OPTION_STATS)

#define	RERR_RCPA	0
#define	RERR_RCPS	1
#define	RERR_XMEX	2
#define	RERR_XMDR	3
#define	RERR_CONG	4
#define	RERR_MAXB	5
#define	RERR_CURB	6
#define	RERR_SIZE	(sizeof (word) * RERR_CURB)

	word		rerror [RERR_CURB+1];
#endif
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

// ============================================================================

typedef	struct {
//
// This structure is shared by all PicOS's RTC interfaces
//
	byte	year, month, day, dow, hour, minute, second;

} rtc_time_t;

class rtc_module_t {
//
// Generic Real Time Clock interface
//
	time_t	SecOffset;

	public:

	rtc_module_t () { SecOffset = 0; };

	void set (const rtc_time_t*);
	void get (rtc_time_t*);
};

// ============================================================================

process	_PP_ {
//
// This is a standard prefix for a PicOS process
//
	TIME		WaitingUntil;	// Target time (if waiting)
	_PP_		*HNext, *HPrev;	// For hash collisions
	sint		ID;		// The identifier (required by PicOS)
	word		Flags;		// GP flags mostly for grabs
	IPointer	__pi_data;	// The data pointer

// Note ... in preparation for a compiler: we will put there a data pointer
// but for now it would create more mess than good; this is because while a
// simple macro pre-processing would go a long way towards simplicity and
// uniformity, cpp is a bit insufficient

#define	_PP_flag_wtimer 0	// The process is waiting on a timer

	sint	_pp_apid_ ();	// Allocates process ID

	void 	_pp_hashin_ (), _pp_unhash_ ();

	void	inline _pp_enter_ () {

		// This method is transparently called whenever the
		// thread/strand wakes up, no matter in which state;
		// we needed it to implement the proper semantics
		// of operations like snooze (which have been removed
		// from PicOS and VUEE - so much for creativity); we
		// may still need it for something later, so let it
		// linger for a while

		clearFlag (Flags, _PP_flag_wtimer);
	};

	virtual ~_PP_ () {
		_pp_unhash_ ();
	};
};
	
station PicOSNode abstract {

	void		_da (phys_dm2200) (int, int);
	void		_da (phys_cc1100) (int, int);
	void		phys_rfmodule_init (int, int);
	void		_da (phys_uart) (int, int, int);

	// Used to implement second clock (as starting from zero)
	lint		SecondOffset;

	Mailbox	TB;		// For trigger

	pwr_tracker_t	*pwr_tracker;

	void pwrt_change (word m, word l) {
		if (pwr_tracker)
			pwr_tracker->pwrt_change (m, l);
	};

	void pwrt_add (word m, word l, double tm) {
		if (pwr_tracker)
			pwr_tracker->pwrt_add (m, l, tm);
	};

	void pwrt_clear () {
		if (pwr_tracker)
			pwr_tracker->pwrt_clear ();
	};

	void pwrt_zero () {
		// Zero out when the node is being switched off
		if (pwr_tracker)
			pwr_tracker->pwrt_zero ();
	};

	/*
	 * The watchdog process
	 */
	Process		*Watchdog;

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

	lword		_da (entropy),
			__host_id__;

	inline void _da (add_entropy) (lword u) {
		_da (entropy) = (_da (entropy) << 4) ^ u;
	};

	inline lword __host_id () { return __host_id__; };

	/*
	 * To tell if the node is halted
	 */
	Boolean		Halted,

	/*
	 * Can be moved from its initial location
	 */
			Movable;
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
	 * Emulator output
	 */
	EMULM		*emulm;

	/*
	 * Sensors/actuators
	 */
	SNSRS		*snsrs;

	/*
	 * This is EEPROM and FIM (IFLASH)
	 */
	NVRAM		*eeprom, *iflash;

	// ====================================================================

	/*
	 * Graphic LCD display
	 */
	LCDG		*lcdg;

	// ====================================================================

	/*
	 * RTC: this one is built-in and not optional, as it is extremely
	 * simple, so we need not specify it in the data set
	 */
	rtc_module_t	rtc_module;

	// ====================================================================

	/*
	 * If not NULL, contains information how the node should be temporarily
	 * repainted, if presented by ROAMER
	 */
	highlight_supplement_t *highlight;

	void _da (diag) (const char*, ...);
	void _da (emul) (sint, const char*, ...);

	//
	// Here comes the 'reset' mess:
	//

	// Internal, resets the UART
	void uart_reset (), uart_abort ();

	// Location
#if ZZ_R3D
	inline void get_location (double &xx, double &yy, double &zz) {
		if (RFInt != NULL)
			RFInt->RFInterface->getLocation (xx, yy, zz);
		else
			xx = yy = zz = 0.0;
	};

	inline void set_location (double xx, double yy, double zz) {
		if (RFInt != NULL)
			RFInt->RFInterface->setLocation (xx, yy, zz);
	};
#else
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
#endif

	// This one is called by the praxis to reset the node
	void _da (reset) ();
	// This one halts the node (from the praxis)
	void _da (halt) ();
	// This one stops the node (from outside the praxis)
	void stopall ();
	// Clear the highlight supplement
	Process *cleanhlt ();
	// This is type specific reset; also called by the agent to halt the
	// node
	virtual void reset ();
	// This is type specific init (usually defined in the bottom type
	// to start the root process); also called by the agent to switch
	// the node on (after switchOff)
	virtual void init () { };

	void initParams ();

	address	memAlloc (int, word);
	void memFree (address);
	word _da (actsize) (address);
	Boolean memBook (word);
	void memUnBook (word);
	inline void waitMem (int state) { TB.wait (N_MEMEVENT, state); };

	word _da (memfree) (int, word*);
	word _da (maxfree) (int, word*);

	inline Boolean tally_in_pcs () {
		if (NPcLim == 0)
			return YES;
		if (NPcss >= NPcLim)
			return NO;
		NPcss++;
		return YES;
	};

	inline void tally_out_pcs () {
		if (NPcss != 0)
			NPcss--;
		TB.signal (N_MEMEVENT);
	};

	inline int tally_left () {
		return (NPcLim == 0) ? 999 : (NPcLim - NPcss);
	};

	inline int _da (io) (int state, int dev, int ope, char *buf, int len) {
		// Note: 'dev' is ignored: it exists for compatibility with
		// PicOS; io only works for the (single) UART (in direct mode)
		assert (uart != NULL, "PicOSNode->io: node %s has no UART",
			getSName ());
		return uart->U->ioop (state, ope, buf, len);
	};

	/*
	 * I/O formatting
	 */
	char * _da (vform) (char*, const char*, va_list);
	word _da (vfsize) (const char*, va_list);
	word _da (fsize) (const char*, ...);
	int    _da (vscan) (const char*, const char*, va_list);
	char * _da (form) (char*, const char*, ...);
	int    _da (scan) (const char*, const char*, ...);
	int    _da (ser_out) (word, const char*);
	int    _da (ser_outb) (word, const char*);
	int    _da (ser_in) (word, char*, int);
	int    _da (ser_outf) (word, const char*, ...);
	int    _da (ser_inf) (word, const char*, ...);

	/*
	 * Operations on pins
	 */
	void no_pin_module (const char*);


	inline void _da (buttons_action) (void (*act)(word)) {
		if (pins == NULL)
			no_pin_module ("buttons_action");
		pins->buttons_action (act);
	};


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

	inline void  _da (pmon_start_cnt) (lint cnt, Boolean edge) {
		if (pins == NULL)
			no_pin_module ("pmon_start_cnt");
		pins->pmon_start_cnt (cnt, edge);
	};

	inline void  _da (pmon_stop_cnt) () {
		if (pins == NULL)
			no_pin_module ("pmon_stop_cnt");
		pins->pmon_stop_cnt ();
	};

	inline void  _da (pmon_set_cmp) (lint cnt) {
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

	inline void  _da (pmon_sub_cnt) (lint decr) {
		if (pins == NULL)
			no_pin_module ("pmon_sub_cnt");
		pins->pmon_sub_cnt (decr);
	};

	inline void  _da (pmon_add_cmp) (lint incr) {
		if (pins == NULL)
			no_pin_module ("pmon_add_cmp");
		pins->pmon_add_cmp (incr);
	};

	/*
	 * Sensors/actuators
	 */
	void no_sensor_module (const char*);

	inline void _da (read_sensor) (int st, sint sn, address val) {
		if (snsrs == NULL)
			no_sensor_module ("read_sensor");
		snsrs->read (st, sn, val);
	}
	inline void _da (write_actuator) (int st, sint sn, address val) {
		if (snsrs == NULL)
			no_sensor_module ("write_actuator");
		snsrs->write (st, sn, val);
	}
	inline void _da (wait_sensor) (sint sn, int st) {
		if (snsrs == NULL)
			no_sensor_module ("wait_sensor");
		snsrs->wevent (st, sn);
	}

	/*
	 * EEPROM + FIM (IFLASH)
	 */
	lword _da (ee_size) (Boolean*, lword*);
	word _da (ee_open) ();
	void _da (ee_close) ();
	word _da (ee_read)  (lword, byte*, word);
	word _da (ee_erase) (word, lword, lword);
	word _da (ee_write) (word, lword, const byte*, word);
	word _da (ee_sync) (word);
	int  _da (if_write) (word, word);
	word _da (if_read)  (word);
	void _da (if_erase) (int);

	// Note: static TCV data is initialized in tcv_init.
#include "tcv_node_data.h"

#include "lib_attributes.h"

	// Praxis requested attributes (intended for libraries)

	void setup (data_no_t*);

	IPointer preinit (const char*);
};

// === UART direct ============================================================

process d_uart_inp_p : _PP_ (PicOSNode) {

	uart_dir_int_t *f;
	char *tmp, *ptr;
	int len;

	states { IM_INIT, IM_READ, IM_BIN, IM_BIN1 };

	void setup ();
	void close ();

	perform;
};

process d_uart_out_p : _PP_ (PicOSNode) {

	uart_dir_int_t *f;
	const char *data, *ptr;
	int len;

	states { OM_INIT, OM_WRITE };

	void setup (const char*);
	void close ();

	perform;
};

// === UART N-mode ============================================================

process p_uart_rcv_n : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	states { RC_LOOP, RC_PREAMBLE, RC_WLEN, RC_FILL, RC_OFFSTATE, RC_WOFF,
		RC_RESET, RC_WRST };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

process p_uart_xmt_n : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	states { XM_LOOP, XM_PRE, XM_LEN, XM_SEND };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

// === UART P-mode ============================================================

process p_uart_rcv_p : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	states { RC_LOOP, RC_PREAMBLE, RC_WLEN, RC_FILL, RC_OFFSTATE, RC_WOFF,
		RC_RESET, RC_WRST };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

process p_uart_xmt_p : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	byte Hdr [2], Chk [2];

	states { XM_LOOP, XM_HDR, XM_LEN, XM_SEND, XM_CHK, XM_CHL, XM_END,
		XM_NEXT, XM_SACK, XM_SACL, XM_SACM, XM_SACN };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

// === UART L-mode ============================================================

process p_uart_rcv_l : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	states { RC_LOOP, RC_FIRST, RC_MORE, RC_OFFSTATE, RC_WOFF };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;

	void rdbuff (int, int);
};

process p_uart_xmt_l : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	states { XM_LOOP, XM_SEND, XM_EOL1, XM_EOL2 };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

// === UART E-mode ============================================================

process p_uart_rcv_e : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	byte Parity;

	states { RC_LOOP, RC_WAITSTX, RC_MORE, RC_MOREESC, RC_WAITESC,
			RC_OFFSTATE, RC_WOFF };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

process p_uart_xmt_e : _PP_ (PicOSNode) {

	uart_tcv_int_t *UA;

	byte Parity;

	states { XM_LOOP, XM_SSTX, XM_SEND, XM_OUTBYTE, XM_ESCAPE, XM_CODA,
			XM_SPARITY, XM_SETX, XM_PESCAPE };

	void setup () { UA = UART_INTF_P (TheNode->uart); };

	perform;
};

// RF Module ==================================================================

// Uncomment this to make LBT threshold an average over the interval (as it
// used to be) rather than the maximum
// #define LBT_THRESHOLD_IS_AVERAGE

process RM_Receiver : _PP_ (PicOSNode) {

	rfm_intd_t *rf;

	states { RCV_GETIT, RCV_START, RCV_RECEIVE, RCV_GOTIT };

	byte get_rssi (byte&);

	perform;

	void setup ();
};

process RM_ADC (PicOSNode) {
//
// This one is not _PP_
//
	rfm_intd_t 	*rf;

#ifdef LBT_THRESHOLD_IS_AVERAGE
	double		ATime,		// Accumulated sampling time
			Average,	// Average signal so far
			CLevel;		// Current (last) signal level
	TIME		Last;		// Last sample time
#else
	double		Maximum;
#endif
	double sigLevel ();

	states { ADC_WAIT, ADC_RESUME, ADC_UPDATE, ADC_STOP };

	perform;

	void setup ();
};

process RM_Xmitter : _PP_ (PicOSNode) {

	int		buflen;
	word		ntry;
	RM_ADC		*RSSI;
	rfm_intd_t	*rf;

	states { XM_LOOP, XM_TXDONE, XM_LBS };

	perform;

	void setup ();
	void gbackoff ();
	void pwr_on ();
	void pwr_off ();
	void set_congestion_indicator (word);
};

// Neutrino channel

process RN_Receiver : _PP_ (PicOSNode) {

	rfm_intd_t	*rf;

	states { RCV_GETIT, RCV_GO };

	void setup ();

	perform;
};

process RN_Xmitter : _PP_ (PicOSNode) {

	rfm_intd_t	*rf;

	void setup ();
	void pwr_on (), pwr_off ();

	states { XM_LOOP, XM_TXDONE };

	perform;
};

// ============================================================================

process	BoardRoot {

	data_no_t *readNodeParams (sxml_t, int, const char*, const char*);
	data_ua_t *readUartParams (sxml_t, const char*);
	data_pn_t *readPinsParams (sxml_t, const char*);
	data_sa_t *readSensParams (sxml_t, const char*);
	data_le_t *readLedsParams (sxml_t, const char*);
	data_em_t *readEmulParams (sxml_t, const char*);
	data_pt_t *readPwtrParams (sxml_t, const char*);

	void initTiming (sxml_t);
	int initChannel (sxml_t, int, Boolean);
	void initNodes (sxml_t, int, int, const char*[], const char*[], int);
	void initPanels (sxml_t);
	void initRoamers (sxml_t);
	void initAgent (sxml_t);
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

	states { InitMove, AckMove, BgrMove, AckDone, Loop, ReadRq, Reply,
			Delay };

	void setup (Dev*, FLAGS);

	~MoveHandler ();

	void fill_buffer (Long, char);
	char *read_image_files (int&);

	perform;
};

void __mup_update (Long);		// Function to request move update

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

// ============================================================================

void hold (int, lword);
void delay (word, int);
word dleft (sint);

inline void __pi_when (int ev, int state) { TheNode->TB.wait (ev, state); }

// ============================================================================

sint __pi_join (sint, word);
void __pi_joinall (code_t, word);
void __pi_kill (sint);
sint __pi_running (code_t);
int __pi_crunning (code_t);
void __pi_killall (code_t);
sint __pi_getcpid ();

// ============================================================================

void powerdown (), powerup ();

// ============================================================================

void watchdog_start (), watchdog_stop ();

#define	watchdog_clear()	watchdog_start ()

// ============================================================================

void rtc_set (const rtc_time_t*);
void rtc_get (rtc_time_t*);

// ============================================================================

void highlight_set (lword, double , const char*, ...);
void highlight_clear ();

// ============================================================================

int _no_module_ (const char*, const char*);

// ============================================================================

#define	VCTRL_PTRCK_CLEAR	0

void vuee_control (int, ...);

#endif
