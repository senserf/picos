#ifndef	__picos_agent_h__
#define	__picos_agent_h__

#define	AGENT_SOCKET		4443

#define	UART_IMODE_NONE		(0<<29)
#define	UART_IMODE_DEVICE	(1<<29)
#define	UART_IMODE_SOCKET	(2<<29)
#define	UART_IMODE_STRING	(3<<29)
#define	UART_IMODE_MASK		(3<<29)

#define	UART_OMODE_NONE		(0<<25)
#define	UART_OMODE_DEVICE	(1<<25)
#define	UART_OMODE_SOCKET	(2<<25)
#define	UART_OMODE_MASK		(3<<25)

#define	UART_IMODE_TIMED	(1<<28)
#define	UART_IMODE_UNTIMED	(0<<28)
#define	UART_IMODE_HEX		(1<<27)
#define	UART_IMODE_ASCII	(0<<27)

#define	UART_OMODE_HEX		(1<<24)
#define	UART_OMODE_ASCII	(0<<24)
#define	UART_OMODE_HOLD		(1<<23)		/* hold output (socket) */
#define	UART_OMODE_NOHOLD	(0<<23)

#define	UART_IMODE_STRLEN	0x00FFFFFF

#define	CONNECTION_TIMEOUT	(1024*30)	/* Thirty seconds */
#define	SHORT_TIMEOUT		(1024*10)	/* Ten seconds */
#define	AGENT_MAGIC		htons (0xBAB4)

#define	AGENT_RQ_UART		1		/* Remote UART */
#define	AGENT_RQ_PINS		2		/* Pin control */
#define	AGEBT_RQ_LEDS		3		/* LED display */
#define	AGENT_RQ_MOVE		4		/* Mobility */

#define	ECONN_MAGIC		0		/* Illegal magic */
#define	ECONN_STATION		1		/* Illegal station number */
#define	ECONN_UNIMPL		2		/* Function unimplemented */
#define	ECONN_NOUART		3		/* Station has no UART */
#define	ECONN_ALREADY		4		/* Already connected to this */
#define	ECONN_OK		129		/* Positive ack */

typedef	unsigned char	byte;

#define	tohex(d)	((char) (((d) > 9) ? (d) + 'a' - 10 : (d) + '0'))

typedef	struct {
/*
 * Incoming request header
 */
	unsigned int	magic:16,	// Magic for a quick sanity check
			rqcod:16,	// Request code
			stnum:32;	// Station ID
} rqhdr_t;

mailbox Dev (int) {

	inline int w (int st, char *buf, int nc) {
		if (this->write (buf, nc) != ACCEPTED) {
			if (!isActive ())
				return ERROR;
			this->wait (OUTPUT, st);
			sleep;
		}
		return OK;
	};

	inline int r (int st, char *buf, int nc) {
		if (this->read (buf, nc) != ACCEPTED) {
			if (!isActive ())
				return ERROR;
			this->wait (NEWITEM, st);
			sleep;
		}
		return OK;
	};
};

process	UART_in;
process	UART_out;

class	UART {

	friend  class UART_in;
	friend  class UART_out;
	friend	class AgentConnector;

	Dev	*I, *O;		// Input and output mailboxes
	FLAGS	Flags;
	int	B_len;
	TIME	ByteTime;

	char 	*String;
	int	SLen;

	byte	*IBuf;
	int	IB_in, IB_out;

	byte	*OBuf;
	int	OB_in, OB_out;

	char	*TI_aux;
	int	TCS, TI_ptr;
	TIME	TimedChunkTime;

	UART_in	 	*PI;
	UART_out 	*PO;

	public:

	inline Boolean ibuf_full () {
		return ((IB_in + 1) % B_len) == IB_out;
	};

	inline void ibuf_put (int b) {
		IBuf [IB_in++] = (byte) b;
		if (IB_in == B_len)
			IB_in = 0;
	};

	inline int ibuf_get () {
		int k;
		if (IB_in == IB_out)
			return -1;
		k = IBuf [IB_out++];
		if (IB_out == B_len)
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
		if (++OB_out == B_len)
			OB_out = 0;
	};

	inline int obuf_put (byte b) {
		if (((OB_in + 1) % B_len) == OB_out)
			return -1;
		OBuf [OB_in++] = b;
		if (OB_in == B_len)
			OB_in = 0;
		return 0;
	};

	char getOneByte (int);
	void sendStuff (int, char*, int n);

	void getTimed (char*, int);

	int ioop (int st, int op, char*, int len);

	void rst ();

	UART (int, const char*, const char*, int, int);
	~UART ();
};

process	UART_in {

	UART	*U;
	TIME	TimeLastRead;

	states { Get, GetH0, GetH1 };
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

process	AgentConnector {
/*
 * Started by AgentInterface to handle one incoming UART connection. It
 * disappears as soon as things have been set up properly. We need a process
 * for this because we may need to flush pending UART output.
 */
	Dev 	*Agent;
	UART	*UA;
	char	*buf;
	byte	c;
	int	left;

	states { Init,

			DoUart, AckUart, UartFlush,

				KillConnection, WaitKilled, KillAnyway };

	void setup (Dev*);

	perform;
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

#endif
