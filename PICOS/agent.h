#ifndef	__picos_agent_h__
#define	__picos_agent_h__

#define	AGENT_SOCKET		4443

#define	PMON_DEBOUNCE_CNT_ON	(3*16)		// 48 msec on
#define	PMON_DEBOUNCE_CNT_OFF	(3*16)		// 48 msec off
#define	PMON_DEBOUNCE_NOT_ON	(4*16)		// 64 msec on
#define	PMON_DEBOUNCE_NOT_OFF	(100*16)	// 2 sec off

// Pin status
#define	PINSTAT_INPUT		0
#define	PINSTAT_OUTPUT		1
#define	PINSTAT_ADC		2
#define	PINSTAT_DAC0		3
#define	PINSTAT_DAC1		4
#define	PINSTAT_PULSE		5
#define	PINSTAT_NOTIFIER	6
#define	PINSTAT_ABSENT		7

// Voltage triggers for ON/OFF pins
#define	SCHMITT_DOWNL	((word)(0.9 * 0x7fff))	// Going down low
#define	SCHMITT_DOWNH	((word)(1.3 * 0x7fff))	// Going down high

#define	SCHMITT_UPL	((word)(1.5 * 0x7fff))	// Going up low
#define	SCHMITT_UPH	((word)(1.9 * 0x7fff))	// Going up high


#define	XTRN_MBX_BUFLEN		64		// Mailbox buffer size
#define	PRQS_INPUT_BUFLEN	82		// PIN request buffer size
#define	PUPD_OUTPUT_BUFLEN	32		// PIN update buffer size
#define	MRQS_INPUT_BUFLEN	112		// MOVE request buffer size

#define	MOVER_MAX_STEPS		10000		// Max number of steps per leg
#define	MOVER_TARGET_STEP	0.05		// 5 centimeters
#define	MOVER_MIN_STEPS		8		// Even a tiny move is no jump

#define	XTRN_IMODE_NONE		(0<<29)
#define	XTRN_IMODE_DEVICE	(1<<29)
#define	XTRN_IMODE_SOCKET	(2<<29)
#define	XTRN_IMODE_STRING	(3<<29)
#define	XTRN_IMODE_MASK		(3<<29)

#define	XTRN_OMODE_NONE		(0<<25)
#define	XTRN_OMODE_DEVICE	(1<<25)
#define	XTRN_OMODE_SOCKET	(2<<25)
#define	XTRN_OMODE_MASK		(3<<25)

#define	XTRN_IMODE_TIMED	(1<<28)
#define	XTRN_IMODE_UNTIMED	(0<<28)
#define	XTRN_IMODE_HEX		(1<<27)
#define	XTRN_IMODE_ASCII	(0<<27)

#define	XTRN_OMODE_HEX		(1<<24)
#define	XTRN_OMODE_ASCII	(0<<24)
#define	XTRN_OMODE_HOLD		(1<<23)		/* hold output (socket) */
#define	XTRN_OMODE_NOHOLD	(0<<23)

#define	XTRN_IMODE_STRLEN	0x00FFFFFF

#define	CONNECTION_TIMEOUT	(1024*30)	/* Thirty seconds */
#define	SHORT_TIMEOUT		(1024*10)	/* Ten seconds */
#define	AGENT_MAGIC		htons (0xBAB4)

#define	AGENT_RQ_UART		1		/* Remote UART */
#define	AGENT_RQ_PINS		2		/* Pin control */
#define	AGENT_RQ_LEDS		3
#define	AGENT_RQ_MOVE		4		/* Mobility */
#define	AGENT_RQ_CLOCK		5

#define	ECONN_MAGIC		0		/* Illegal magic */
#define	ECONN_STATION		1		/* Illegal station number */
#define	ECONN_UNIMPL		2		/* Function unimplemented */
#define	ECONN_NOUART		3		/* Station has no UART */
#define	ECONN_ALREADY		4		/* Already connected to this */
#define	ECONN_NOPINS		5		/* No pin module */
#define	ECONN_TIMEOUT		6
#define	ECONN_ITYPE		7		/* Non-socket interface */
#define	ECONN_NOLEDS		8
#define	ECONN_DISCONN		9		/* This is in fact a dummy */
#define	ECONN_LONG		11
#define	ECONN_INVALID		12		/* Invalid request */
#define	ECONN_OK		129		/* Positive ack */

#define	ThePicOSNode	((PicOSNode*)TheStation)

typedef	unsigned char	byte;

#define	tohex(d)	((char) (((d) > 9) ? (d) + 'a' - 10 : (d) + '0'))
#define unhex(c) 	((byte)(((byte)(c) <= '9') ? (c) - '0' : ( \
			  ((byte)(c) <= 'F') ? (c) - 'A' + 10 : (c) - 'a' + 10 \
			)))
typedef	struct {
/*
 * Incoming request header
 */
	unsigned int	magic:16,	// Magic for a quick sanity check
			rqcod:16,	// Request code
			stnum:32;	// Station ID
} rqhdr_t;

mailbox Dev (int) {

	inline int wl (int st, char *&buf, int &left) {
		// This is for writing stuff potentially longer than the
		// mailbox buffer
		int nc;

		while (left > 0) {
			nc = (left > XTRN_MBX_BUFLEN) ? XTRN_MBX_BUFLEN : left;
			if (this->write (buf, nc) != ACCEPTED) {
				if (!isActive ())
					return ERROR;
				this->wait (OUTPUT, st);
				sleep;
			}
			left -= nc;
			buf += nc;
		}
	}

	inline int wi (int st, char *buf, int nc) {
		// This one is for writing short stuff (certainly not longer
		// than the mailbox buffer)
		assert (nc <= XTRN_MBX_BUFLEN, "Dev->w at %s: attempt to write"
			" %1d bytes, which is more than %1d", nc,
				XTRN_MBX_BUFLEN);
		if (this->write (buf, nc) != ACCEPTED) {
			if (!isActive ())
				return ERROR;
			this->wait (OUTPUT, st);
			sleep;
		}
		return OK;
	};

	inline int ri (int st, char *buf, int nc) {
		if (this->read (buf, nc) != ACCEPTED) {
			if (!isActive ())
				return ERROR;
			this->wait (NEWITEM, st);
			sleep;
		}
		return OK;
	};

	inline int rs (int st, char *&buf, int &left) {

		int nk;

		while (1) {
			if ((nk = readToSentinel (buf, left)) == ERROR)
				return ERROR;

			if (nk == 0) {
				this->wait (SENTINEL, st);
				sleep;
			}

			left -= nk;
			buf += nk;

			// Check if the sentinel has been read
			if (TheEvent)
				// An alias for Info01, means that the
				// sentinel was found
				return OK;

			if (left == 0)
				return REJECTED;
		} 
	};
};

process	UART_in;
process	UART_out;

class	UART {

	friend  class UART_in;
	friend  class UART_out;
	friend	class UartHandler;

	union	{
		Dev 	    *I;	// Input mailbox (shared with string)
		const char  *S;
	};

	Dev	*O;		// Output mailbox

	FLAGS	Flags;
	word	B_ilen, B_olen;
	TIME	ByteTime;

	byte	*IBuf;
	word	IB_in, IB_out;

	byte	*OBuf;
	word	OB_in, OB_out;

	char	*TI_aux;
	int	TCS, TI_ptr;
	TIME	TimedChunkTime;

	UART_in	 	*PI;
	UART_out 	*PO;

	public:

	inline Boolean ibuf_full () {
		return ((IB_in + 1) % B_ilen) == IB_out;
	};

	inline void ibuf_put (int b) {
		IBuf [IB_in++] = (byte) b;
		if (IB_in == B_ilen)
			IB_in = 0;
	};

	inline int ibuf_get () {
		int k;
		if (IB_in == IB_out)
			return -1;
		k = IBuf [IB_out++];
		if (IB_out == B_ilen)
			IB_out = 0;
		return k;
	};

	inline Boolean obuf_empty () {
		return (OB_in == OB_out);
	};

	inline byte obuf_peek () {
		return OBuf [OB_out];
	};

	inline void obuf_get () {
		if (++OB_out == B_olen)
			OB_out = 0;
	};

	inline int obuf_put (byte b) {
		if (((OB_in + 1) % B_olen) == OB_out)
			return -1;
		OBuf [OB_in++] = b;
		if (OB_in == B_olen)
			OB_in = 0;
		return 0;
	};

	char getOneByte (int);
	void sendStuff (int, char*, int n);

	void getTimed (int, char*);

	int ioop (int st, int op, char*, int len);

	void rst ();

	UART (data_ua_t*);
	~UART ();
};

process	UART_in {

	UART	*U;
	TIME	TimeLastRead;

	states { Get, GetH1 };
	void setup (UART*);
	perform;
};

process	UART_out {

	UART	*U;
	TIME	TimeLastWritten;

	states { Put };
	void setup (UART*);
	perform;
};

class LEDSM {
/*
 * The LEDs module
 */
	friend	class	LedsHandler;

	word 	NLeds;			// Number of leds (<= 64)
	byte	OUpdSize;		// Update buffer size
	Boolean	Device,			// Device flag (as opposed to socket)
		Changed,		// Change flag
		Fast;			// Fast blink rate

	byte	*LStat;			// Led status, one nibble per LED

	Dev	*O;

	char	*UBuf;			// Buffer for updates

	Process	*OutputThread;

	inline word getstat (word led) {
		word t = LStat [led >> 1];
		if ((led & 1) == 0)
			t >>= 4;
		return t & 0xf;
	};

	inline void setstat (word led, word stat) {
		word t = led >> 1;
		if ((led & 1) == 0)
			LStat [t] = ((LStat [t] & 0x0f) | (stat << 4));
		else
			LStat [t] = ((LStat [t] & 0xf0) | (stat     ));
	};

	public:

	int ledup_status ();
	void fastblink (Boolean);

	void leds_op (word, word);

	LEDSM (data_le_t*);
	~LEDSM ();

	void rst ();
};

typedef	struct	{

	unsigned int pin:8;
	unsigned int stat:8;
	unsigned int value:16;

} pin_update_t;

mailbox PUpdates (long) {

	inline void queue (pin_update_t u) {
		this->put (*((long*)(&u)));
	};

	inline pin_update_t retrieve () {
		long it = this->get ();
		return *((pin_update_t*)(&it));
	};
};

mailbox MUpdates (Long) {

	inline void queue (Long nid) {
		if (!queued (nid))
			this->put (nid);
	};
};

station PicOSNode;

class PINS {
/*
 * The Pins module
 */
	friend class PinsHandler;
	friend class PinsInput;
	friend class PulseMonitor;

	TIME	TimedRequestTime;	// For timed updates

	lword	pmon_cnt,		// Pulse monitor counter
		pmon_cmp;		// And comparator

	PicOSNode	*TPN;		// Node backpointer

	byte	PIN_MAX,		// Number of pins (0 ... MAX - 1)
		PIN_MAX_ANALOG,		// Analog capable pin range from 0
		PIN_MONITOR [2],	// Pulse monitor and notifier
		PIN_DAC [2],		// DAC0- and DAC1-capable pins
		PIN_DAC_USED [2],	// true/false: DAC pin used as DAC
		PASIZE,			// Pin array size
		AASIZE,			// Analog pin array size
		NASIZE,			// PASIZE in nibbles
		MonPolarity,		// Trigger value of monitor pin
		NotPolarity;		// Trigger value of notifier pin

	Boolean	adc_inuse,		// Pending ADC cpnversion
		pmon_cnt_on,		// Counter is ON
		pmon_cmp_on,		// Comparator is ON
		pmon_cmp_pending,	// Comparator event pending
		pmon_not_on,		// Notifier is ON
		pmon_not_pending;	// Pending notifier event

	const byte *DefIVa,		// Default (input) values
		   *Status;		// Which pins from the range are there

	byte 	*Direction,		// Current direction of the pin
		*Analog,		// Tells ADC pins (like SEL)
		*IValues,		// As set from the outside
		*OValues;		// As set by the praxis

	short	VDAC [2],		// DAC voltage, valid if DAC_USED
		*VADC;			// ADC voltage, set from outside

	const short *DefAVo;		// Default (inout) voltage

	FLAGS	Flags;

	int 	SLen;			// String length for string input

	union	{
		Dev	   *I;
		const char *S;
	};

	Dev	*O;

	char	*UBuf;			// Buffer for updates

	PUpdates *Upd;

	Process	*InputThread, *OutputThread, *MonitorThread, *NotifierThread;

	public:

	inline static int gbit (const byte *b, int n) {
		// Value of n-th bit in the bit array
		return (b [n >> 3] >> (n & 7)) & 1;
	};

	inline static void sbit (byte *b, int n) {
		// Set n-th bit
		b [n >> 3] |= (1 << (n & 7));
	};

	inline static void cbit (byte *b, int n) {
		// Clear n-th bit
		b [n >> 3] &= ~(1 << (n & 7));
	};

	byte pin_gstat (word p);

	void qupd_pin (word p);

	void qupd_all ();

	inline Boolean pin_available (word p) {
		return pin_gstat (p) <= PINSTAT_ADC;
	};

	Boolean pin_adc_available (word p);

	inline Boolean pin_adc (word p) {
		return (p < PIN_MAX_ANALOG) && gbit (Analog, p);
	};

	inline Boolean pin_output (word p) {
		return (p < PIN_MAX && gbit (Direction, p));
	};

	inline word pin_ivalue (word p) {
		return gbit (IValues, p);
	};

	inline word pin_ovalue (word p) {
		return gbit (OValues, p);
	};

	inline Boolean pin_dac_available (word p) {
		return (p == PIN_DAC [0] || p == PIN_DAC [1]);
	};

	inline Boolean pin_dac (word p) {
		return 	(p == PIN_DAC [0] && PIN_DAC_USED [0]) ||
			(p == PIN_DAC [1] && PIN_DAC_USED [1]) ;
	};

	inline Boolean pin_dac_present () {
		return PIN_DAC [0] != 0xff || PIN_DAC [1] != 0xff;
	};

	inline void pin_set_dac (word p) {
		if (p == PIN_DAC [0])
			PIN_DAC_USED [0] = 1;
		else
			PIN_DAC_USED [1] = 1;
		cbit (Analog, p);
	};

	inline void pin_clear_dac (word p) {
		if (p == PIN_DAC [0])
			PIN_DAC_USED [0] = 0;
		else if (p == PIN_DAC [1])
			PIN_DAC_USED [1] = 0;
	};

	inline void pin_set_adc (word p) {
		pin_clear_dac (p);
		sbit (Analog, p);
	};

	inline void pin_set_input (word p) {
		pin_clear_dac (p);
		cbit (Analog, p);
		cbit (Direction, p);
	};

	inline void pin_set_output (word p) {
		pin_clear_dac (p);
		cbit (Analog, p);
		sbit (Direction, p);
	};

	inline void pin_set   (word p) { sbit (OValues, p); };
	inline void pin_clear (word p) { cbit (OValues, p); };

	word adc (short, int);
	short dac (word, int);

	void pin_new_value (word, word);
	void pin_new_voltage (word, word);
	
	int pinup_status (pin_update_t);
	int pinup_update (char*);

	void pmon_update (byte);

	void rst ();

	PINS (data_pn_t*);
		
	// Why do we need this? Why do we need this at UART? Ah, I see, a
	// station may jettison the UART if it is no longer needed, thus
	// reclaiming some simulator's resources. Does it make a lot of
	// sense?
	~PINS ();

	word pin_read (word);
	int pin_write (word, word);
	int pin_read_adc (word, word, word, word);
	int pin_write_dac (word, word, word);
	void pmon_start_cnt (long, Boolean);
	void pmon_stop_cnt ();
	void pmon_set_cmp (long);
	lword pmon_get_cnt ();
	lword pmon_get_cmp ();
	void pmon_start_not (Boolean);
	void pmon_stop_not ();
	word pmon_get_state ();
	Boolean pmon_pending_not ();
	Boolean pmon_pending_cmp ();
	void pmon_dec_cnt ();
	void pmon_sub_cnt (long);
	void pmon_add_cmp (long);
};

process	AgentInterface {
/*
 * This one is started at the beginning and remains alive all the time
 */

	Dev *M;

	states { WaitConn };
	void setup ();
	perform;
};

extern word ZZ_Agent_Port;

#endif
