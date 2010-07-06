#ifndef	__picos_agent_h__
#define	__picos_agent_h__

#define	AGENT_SOCKET		4443

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

#define	PWRT_CPU		0		// Power up
#define	PWRT_CPU_FP		0		// Full power state
#define	PWRT_CPU_LP		1		// Low power state

#define	PWRT_RADIO		1
#define	PWRT_RADIO_OFF		0
#define	PWRT_RADIO_RCV		1
#define	PWRT_RADIO_XMT		2
#define	PWRT_RADIO_XCV		3

#define	PWRT_STORAGE		2
#define	PWRT_STORAGE_OFF	0
#define	PWRT_STORAGE_ON		1
#define	PWRT_STORAGE_READ	2
#define	PWRT_STORAGE_WRITE	3
#define	PWRT_STORAGE_ERASE	4
#define	PWRT_STORAGE_SYNC	5

#define	PWRT_SENSOR		3
#define	PWRT_SENSOR_OFF		0
#define	PWRT_SENSOR_ON		1

#define	XTRN_MBX_BUFLEN		64		// Mailbox buffer size
#define	PRQS_INPUT_BUFLEN	82		// PIN request buffer size
#define	PUPD_OUTPUT_BUFLEN	48		// PIN update buffer size
#define	MRQS_INPUT_BUFLEN	112		// MOVE request buffer size
#define	SRQS_INPUT_BUFLEN	64		// SENSOR request buffer size
#define	ARQS_INPUT_BUFLEN	64		// PANEL request buffer size
#define	AUPD_OUTPUT_BUFLEN	64		// Actuator update buffer size
#define	LCDG_OUTPUT_BUFLEN	1024		// LCDG output buffer size (bts)

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
#define	AGENT_RQ_PANEL		5
#define	AGENT_RQ_CLOCK		6
#define	AGENT_RQ_SENSORS	7
#define	AGENT_RQ_LCDG		8
#define	AGENT_RQ_PWRT		9

#define	ECONN_MAGIC		0		/* Illegal magic */
#define	ECONN_STATION		1		/* Illegal station number */
#define	ECONN_UNIMPL		2		/* Function unimplemented */
#define	ECONN_ALREADY		3		/* Already connected to this */
#define	ECONN_TIMEOUT		4
#define	ECONN_ITYPE		5		/* Non-socket interface */
#define	ECONN_DISCONN		6		/* This is in fact a dummy */
#define	ECONN_LONG		7
#define	ECONN_INVALID		8		/* Invalid request */

#define	ECONN_NOMODULE		128
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

struct lcdg_update_struct {
//
// LCDG update
//
	struct lcdg_update_struct *Next;

	word Size;

	// The size of this will vary
	word Buf [0];
};

typedef struct lcdg_update_struct lcdg_update_t;

mailbox Dev (int) {

	int wl (int, char*&, int&);
	int wi (int, const char*, int);
	int ri (int, char*, int);
	int rs (int, char*&, int&);
};

// ============================================================================

process	UART_in;
process	UART_out;

class	UARTDV {

	friend  class UART_in;
	friend  class UART_out;
	friend	class UartHandler;

	union	{
		Dev 	    *I;	// Input mailbox (shared with string)
		char  	    *S;
	};

	Dev	*O;		// Output mailbox

	FLAGS	Flags;
	word	B_ilen, B_olen;
	TIME	ByteTime;

	word	DefRate, Rate;	// Default and current rate /100

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

	void setRate (word);

	word getRate () { return Rate; };

	void rst ();

	UARTDV (data_ua_t*);
	~UARTDV ();
};

process	UART_in {

	UARTDV	*U;
	TIME	TimeLastRead;

	states { Get, GetH1 };
	void setup (UARTDV*);
	perform;
};

process	UART_out {

	UARTDV	*U;
	TIME	TimeLastWritten;

	states { Put };
	void setup (UARTDV*);
	perform;
};

// ============================================================================

station PicOSNode;

class ag_interface_t {
//
// Agent's output interface
//
	public:

	FLAGS		Flags;

	PicOSNode	*TPN;

	Dev	*O;			// Output mailbox
	union {
					// Input mailbox or string
		Dev		*I;	// If from device (socket/file)
		char 		*S;	// If from string
	};

	Process	*OT, *IT;		// Output and input threads

	void init (FLAGS);
};

// ============================================================================

class LEDSM {
/*
 * The LEDs module
 */
	friend	class	LedsHandler;

	ag_interface_t	IN;

	word 	NLeds;			// Number of leds (<= 64)
	byte	OUpdSize;		// Update buffer size

	Boolean	Changed,		// Change flag
		Fast;			// Fast blink rate

	byte	*LStat;			// Led status, one nibble per LED

	char	*UBuf;			// Buffer for updates

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
	void setfast (Boolean);
	Boolean isfast () { return Fast; };
	
	void leds_op (word, word);

	LEDSM (data_le_t*);
	~LEDSM ();

	void rst ();
};

typedef	struct {

	unsigned int pin:8;
	unsigned int stat:8;
	unsigned int value:16;

} pin_update_t;

mailbox PUpdates (Long) {

// Pin updates

	inline void queue (pin_update_t u) {
		this->put (*((Long*)(&u)));
	};

	inline pin_update_t retrieve () {
		Long it = this->get ();
		return *((pin_update_t*)(&it));
	};
};

#define	SEN_TYPE_PARAMS		0	// Number of sens/act
#define	SEN_TYPE_SENSOR		1	// Update types
#define	SEN_TYPE_ACTUATOR	2

typedef struct {

        unsigned int tp:8;      // SEN_TYPE_SENSOR/SEN_TYPE_ACTUATOR
        unsigned int lm:8;      // Nonzero -> send bounds
        unsigned int sn:16;     // Sensor/actuator number

} act_update_t;

mailbox SUpdates (Long) {

// Sensor/actuator updates

	void queue (byte tp, byte sn, Boolean lm = NO) {

		act_update_t p;

		p.sn = sn;
		p.tp = tp;
		p.lm = lm;

		this->put (*((Long*)(&p)));
	};

	Boolean retrieve (byte &tp, byte &sn) {

		act_update_t p;

		*((Long*)(&p)) = this->get ();

		sn = p.sn;
		tp = p.tp;

		return p.lm;
	};
};

mailbox MUpdates (Long) {

	inline void queue (Long nid) {
		if (!queued (nid))
			this->put (nid);
	};
};

class PINS;

class ButtonPin {

	friend class PINS;
	friend class ButtonRepeater;

	TIME	LastUpdate;
	Process	*Repeater;
	PINS	*Pins;
	byte	Polarity, Pin, State;

	public:

	// Pin number, polarity
	ButtonPin (PINS*, byte, byte);

	Boolean pinon ();

	void update (word);

	void reset ();
};

class PINS {
/*
 * The Pins module
 */
	friend class PinsHandler;
	friend class PinsInput;
	friend class PulseMonitor;
	friend class ButtonPin;
	friend class ButtonRepeater;

	ag_interface_t	IN;

	lword	pmon_cnt,		// Pulse monitor counter
		pmon_cmp;		// And comparator

	ButtonPin	**Buts;		// The buttons

	byte	PIN_MAX,		// Number of pins (0 ... MAX - 1)
		PIN_MAX_ANALOG,		// Analog capable pin range from 0
		PIN_MONITOR [2],	// Pulse monitor and notifier
		PIN_DAC [2],		// DAC0- and DAC1-capable pins
		PIN_DAC_USED [2],	// true/false: DAC pin used as DAC
		PASIZE,			// Pin array size
		AASIZE,			// Analog pin array size
		NASIZE,			// PASIZE in nibbles
		MonPolarity,		// Trigger value of monitor pin
		NotPolarity,		// Trigger value of notifier pin
		NButs;			// Number of buttons

	Long	*Debouncers;		// Optional debouncers (7 entries)

		// Debouncers:
		//	- CNT on
		//	- CNT off
		//	- NOT on
		//	- NOT off
		//	- Button status change
		//	- Button repeat delay
		//	- Button repeat interval

	Boolean	adc_inuse,		// Pending ADC cpnversion
		pmon_cnt_on,		// Counter is ON
		pmon_cmp_on,		// Comparator is ON
		pmon_cmp_pending,	// Comparator event pending
		pmon_not_on,		// Notifier is ON
		pmon_not_pending;	// Pending notifier event

	const byte *DefIVa,		// Default (input) values
		   *Buttons,		// Button mapping
		   *Status;		// Which pins from the range are there

	byte 	*Direction,		// Current direction of the pin
		*Analog,		// Tells ADC pins (like SEL)
		*IValues,		// As set from the outside
		*OValues;		// As set by the praxis

	short	VDAC [2],		// DAC voltage, valid if DAC_USED
		*VADC;			// ADC voltage, set from outside

	const short *DefAVo;		// Default (inout) voltage

	char	*UBuf;			// Buffer for updates

	PUpdates *Upd;

	void (*ButtonsAction)(word);	// Function to call on button pressed

	Process *MonitorThread,
		*NotifierThread;

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

	void reset_buttons ();

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

	void buttons_action (void (*)(word));
	word pin_read (word);
	int pin_write (word, word);
	int pin_read_adc (word, word, word, word);
	int pin_write_dac (word, word, word);
	void pmon_start_cnt (lint, Boolean);
	void pmon_stop_cnt ();
	void pmon_set_cmp (lint);
	lword pmon_get_cnt ();
	lword pmon_get_cmp ();
	void pmon_start_not (Boolean);
	void pmon_stop_not ();
	word pmon_get_state ();
	Boolean pmon_pending_not ();
	Boolean pmon_pending_cmp ();
	void pmon_dec_cnt ();
	void pmon_sub_cnt (lint);
	void pmon_add_cmp (lint);
};

process ButtonRepeater {

	ButtonPin *BP;
	PINS *PS;

	Boolean done () {

		return !BP->pinon () || PS->ButtonsAction == NULL;
	};

	void doaction () {

		Station *sp;

		sp = TheStation;
		TheStation = (Station*)(PS->IN.TPN);
		(*(PS->ButtonsAction)) (PS->Buttons [BP->Pin]);
		TheStation = sp;
	};

	Long debounce (int i) {
		if (PS->Debouncers == NULL)
			return 0;
		return PS->Debouncers [i];
	}

	states { BRP_START, BRP_CHECK, BRP_REPEAT };

	perform;

	void setup (ButtonPin *bp) {
		PS = (BP = bp)->Pins;
	};

	~ButtonRepeater () {
		BP->Repeater = NULL;
	};
};

class SNSRS {
/*
 * Sensors and actuators
 */
	friend class SensorsHandler;
	friend class SensorsInput;

	ag_interface_t	IN;

	SensActDesc	*Sensors,	// The actual objects
			*Actuators;

	byte		NSensors, NActuators;

	char		*UBuf;		// Buffer for updates

	SUpdates	*Upd;

	public:

	void qupd_act (byte, byte, Boolean lm = NO);
	void qupd_all ();
	int act_status (byte, byte, Boolean);
	int sensor_update (char*);

	void read (int, word, address);
	void write (int, word, address);

	SNSRS (data_sa_t*);

	void rst ();			// Called on reset

	~SNSRS ();
};

class pwr_tracker_t {

	friend class PwrtHandler;
	friend class PwrtInput;

	ag_interface_t	IN;

	double	strt_tim, last_tim, last_val, average;

	pwr_mod_t	*Modules [PWRT_N_MODULES];
	word		States [PWRT_N_MODULES];	// Current states

	char		*UBuf;

	Boolean		Changed;

	void upd () {
		Changed = YES;
		if (IN.OT != NULL)
			IN.OT->signal (NULL);
	};

	int pwrt_status ();

	public:

	void rst ();

	pwr_tracker_t (data_pt_t*);

	// New power setting
	void pwrt_change (word md, word st);
	void pwrt_add (word md, word st, double tm);
	void pwrt_clear ();
	// Incoming requests from external agents
	int pwrt_request (const char*);
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
