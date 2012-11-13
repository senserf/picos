/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-12   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

//#define	ZZ_RF_DEBUG

/* --- */

/* ---------------------------------------------------------- */
/* This  is  the  standard  include  file  for  user protocol */
/* modules. It also constitutes  the  basic  part  of  system */
/* include files.                                             */
/* ---------------------------------------------------------- */


#include "../version.h"
#include "../LIB/lib.h"

#define VA_TYPE va_list
#define	INLINE inline

/* ------------------- */
/* Scratch array sizes */
/* ------------------- */
#define         ZZ_ACSSIZE              1024
#define         ZZ_EETSIZE              16384
/* --------------------------------------------------------------------- */
/* ACSSIZE is the size of a scratch array (of type TIME) used by some of */
/* the  link  subroutines.  It is allocated on the stack and with static */
/* size - to avoid fragmentation that could have occurred, had  it  been */
/* allocated  by  malloc.  EETSIZE  is the size of another scratch array */
/* used (by get_event) and allocated on the stack for  similar  reasons. */
/* ACSSIZE  can  be increased if 'stack overflow' is signalled by a link */
/* subroutine. For some systems, e.g. SunOS's, allocating  large  arrays */
/* on  the  stack  is impossible and ACSSIZE may have to be reduced. The */
/* amount of actual stack space required is equal  to  ACSSIZE  *  (8  * */
/* BIG_precision  +  8)  bytes.                                          */
/* --------------------------------------------------------------------- */

/* -------------------- */
/* Clock tick frequency */
/* -------------------- */
#ifndef         HZ
#define         ZZ_HZ                   60
#else
#define         ZZ_HZ                   HZ
#endif

#define		NOP			do { } while (0)

// How to cast a pointer to integer
#define		__cpint(a)		((int)(IPointer)(a))

/* ------------------- */
/* User-visible macros */
/* ------------------- */

/* --------- */
/* Assertion */
/* --------- */
#ifdef	assert
#undef	assert
#endif
#if     ZZ_ASR
#define assert(a,b, ...) do { if (!(a)) excptn (b, ## __VA_ARGS__); } while (0)
#else
#define assert(a,b, ...) do { } while (0)	/* Void */
#endif
#define Assert(a,b, ...) do { if (!(a)) excptn (b, ## __VA_ARGS__); } while (0)

/* --------------------- */
/* Pointers to fixed AIs */
/* --------------------- */
#define         Timer   (&zz_AI_timer)
#define		Monitor	(&zz_AI_monitor)

#if	ZZ_NOC
#define         Client  (&zz_AI_client)
#endif

/* ---------------------------------------------- */
/* A scratch pointer for SMURPH object generation */
/* ---------------------------------------------- */
extern  void    *zz_COBJ[];
extern  int     zz_clv;

/* ============================= */
/* For sleep from out of process */
/* ============================= */
extern	jmp_buf	zz_waker;

/* ------------------------------ */
/* Converts station id to station */
/* ------------------------------ */
#if     ZZ_ASR
#define         idToStation(id)         (isStationId (id) ? zz_st [id] :\
      (Station*) (IPointer) excptn ("idToStation: illegal station Id %d", (id)))
#else
#define         idToStation(id)         zz_st [id]
#endif

/* ----------------------------------------- */
/* Checks if a station identifier is correct */
/* ----------------------------------------- */
#define         isStationId(id)         ((id) >= 0 && (id) < ::NStations)


#if	ZZ_NOC

/* ------------------------------ */
/* Converts Traffic id to Traffic */
/* ------------------------------ */
#if     ZZ_ASR
#define         idToTraffic(id)         (isTrafficId (id) ? zz_tp [id] :\
      (Traffic*) (IPointer) excptn ("idToTraffic: illegal Traffic id %d", (id)))
#else
#define         idToTraffic(id)         (zz_tp [id])
#endif

/* ----------------------------------------- */
/* Checks if a Traffic identifier is correct */
/* ----------------------------------------- */
#define         isTrafficId(id)         ((id) >= 0 && (id) < ::NTraffics)

#endif /* NOC */


#if	ZZ_NOL

/* ------------------------ */
/* Converts Link id to Link */
/* ------------------------ */
#if     ZZ_ASR
#define         idToLink(id)            (isLinkId (id) ? zz_lk [id] :\
		(Link*) (IPointer) excptn ("idToLink: illegal Link Id %d", (id)))
#else
#define         idToLink(id)            (zz_lk [id])
#endif

/* -------------------------------------- */
/* Checks if a Link identifier is correct */
/* -------------------------------------- */
#define         isLinkId(id)            ((id) >= 0 && (id) < ::NLinks)

#endif	/* NOL */

#if	ZZ_NOR

class		ZZ_RF_ACTIVITY;

/* ---------------------------------- */
/* Converts RFChannel id to RFChannel */
/* ---------------------------------- */
#if     ZZ_ASR
#define         idToRFChannel(id)       (isRFChannelId (id) ? zz_rf [id] :\
(RFChannel*) (IPointer) excptn ("idToRFChannel: illegal RFChannel Id %d", (id)))
#else
#define         idToRFChannel(id)            (zz_rf [id])
#endif

/* -------------------------------------------- */
/* Checks if an RFChannel identifier is correct */
/* -------------------------------------------- */
#define         isRFChannelId(id)            ((id) >= 0 && (id) < ::NRFChannels)

#define         TheTransceiver               ((Transceiver*)(Info02))

#endif	/* NOR */

/* ----------------------------------- */
/* Converts whatever to the identifier */
/* ----------------------------------- */
#define         ident(a)                ((a)->getId ())

/* ----------------------------- */
/* Aliases for Info01 and Info02 */
/* ----------------------------- */

#if	ZZ_NOC

#define         TheTraffic              __cpint (Info02)
#define         ThePacket               ((Packet*)(Info01))
#define         ThePort                 ((Port*)(Info02))
#define         TheMessage              ((Message*)(Info01))

#endif

#define         TheEvent                __cpint (Info02)
#define         TheCount                __cpint (Info01)
#define         TheItem                 Info01
#define         TheMailbox              ((Mailbox*)(Info02))
#define         TheExitCode             __cpint (Info01)
#define         TheSender               ((Process*)(Info02))
#define         TheSignal               Info01
#define         TheBarrier              __cpint (Info01)
#define		TheEnd			((Boolean)__cpint (Info01))

/* ---------------------------------------------------------- */
/* One  more  alias of the same type, but for something else. */
/* This is the pointer to  the  current  window's  associated */
/* object                                                     */
/* ---------------------------------------------------------- */
#define         TheWFrame               (zz_the_window->frame)

/* ------------------- */
/* Operations on flags */
/* ------------------- */
#define         setFlag(f,i)            (f |= (1 << (i)))
#define         clearFlag(f,i)          (f &= ~(1 << (i)))
#define         flagSet(f,i)            ((f & (1 << (i))) != 0)
#define         flagCleared(f,i)        ((f & (1 << (i))) == 0)

/* ---------------------------------------------------------- */
/* Operations  on  queues,  requests, and events (global, but */
/* invisible to the user)                                     */
/* ---------------------------------------------------------- */
#define         zz_eq           (zz_fsent.next) // Event queue pointer

/* =============================================================== */
/* Returns the dummy head element given the head pointer. The type */
/* of the element is cast to pointer to indicated type. The 0x8000 */
/* is accidental; it used to be zero, but the new compiler version */
/* started to complain.                                            */
/* =============================================================== */
#define		zz_hptr(hd)\
((typeof(hd))(((char*)&(hd))-(((char*)(&(((typeof(hd))0x8000)->next)))-((char*)0x8000))))

#define         pool_in(item,hd)      {(item)->next = (hd);\
					 (item)->prev = zz_hptr (hd); \
	if ((hd) != NULL) (hd)->prev = (item);\
	(hd) = (item);}

#define         pool_out(item)                      {\
	if ((item)->next != NULL) ((item)->next)->prev = (item)->prev;\
	((item)->prev)->next = (item)->next;}

#define	for_pool(p,h)	for ((p) = (h); (p) != NULL; (p) = (p)->next)

#define	trim_pool(p,h,c) do { \
				typeof (h) pb; \
				for ((p) = (h); (p) != NULL; (p) = pb) { \
					pb = (p)->next; \
					if (c) { \
						pool_out (p); \
						delete (p); \
					} \
				} \
			} while (0)

/* ---------------------------------- */
/* Convert stream pointers to streams */
/* ---------------------------------- */
#define         Inf                     (*zz_ifpp)
#define         Ouf                     (*zz_ofpp)

/* ------------------------- */
/* Global symbolic constants */
/* ------------------------- */

/* -------------- */
/* Boolean values */
/* -------------- */
#define         NO              0       // Logical value 'false'
#define         YES             1       // Logical value 'true'
#define         ON              1       // A flag is on
#define         OFF             0       // A flag is off

#if	ZZ_NOC

/* -------------------------- */
/* Traffic distribution flags */
/* -------------------------- */
#define         MIT_exp         1
#define         MIT_unf         2
#define         MIT_fix         4
#define         MLE_exp         8
#define         MLE_unf         16
#define         MLE_fix         32
#define         BIT_exp         64
#define         BIT_unf         128
#define         BIT_fix         256
#define         BSI_exp         512
#define         BSI_unf         1024
#define         BSI_fix         2048
#define         SCL_on          4096
#define         SCL_off         8192
#define         SPF_on          16384
#define         SPF_off         32768

/* --------------------------------------- */
/* Events generated by the standard Client */
/* --------------------------------------- */
#define		SUSPEND			(-1)
#define		RESUME			(-2)
#define		RESET			(-3)
#define         ARRIVAL                 (-4)
#define         INTERCEPT               (-5)

/* ------------------------------ */
/* Client time distribution modes */
/* ------------------------------ */
#define         EXPONENTIAL             0
#define         UNIFORM                 1
#define         FIXED                   2
#define         UNDEFINED               3

/* ------------------- */
/* Packet status flags */
/* ------------------- */
#define         PF_broadcast    31      // A broadcast packet
#define         PF_last         30      // The last packet of a message
#define         PF_full         29      // The packet buffer is full
#define         PF_damaged      28      // Packet damaged
#define         PF_hdamaged     27      // Packet header damaged

#define         PF_usr0         0       // User-accessible flags
#define         PF_usr1         1
#define         PF_usr2         2
#define         PF_usr3         3
#define         PF_usr4         4
#define         PF_usr5         5
#define         PF_usr6         6
#define         PF_usr7         7
#define         PF_usr8         8
#define         PF_usr9         9
#define         PF_usr10        10
#define         PF_usr11        11
#define         PF_usr12        12
#define         PF_usr13        13
#define         PF_usr14        14
#define         PF_usr15        15
#define         PF_usr16        16
#define         PF_usr17        17
#define         PF_usr18        18
#define         PF_usr19        19
#define         PF_usr20        20
#define         PF_usr21        21
#define         PF_usr22        22
#define         PF_usr23        23
#define         PF_usr24        24
#define         PF_usr25        25
#define         PF_usr26        26

/* ------------------- */
/* Station group types */
/* ------------------- */
#define         GT_selection    0
#define         GT_broadcast    NONE    // -1

/* ----------------------------------------------- */
/* Special constants for addSender and addReceiver */
/* ----------------------------------------------- */
#define         BROADCAST       NONE    // -1
#define         ALL             ((Station*)NULL)

#endif	/* NOC */

/* ---------------------- */
/* Function return status */
/* ---------------------- */
#define         OK              0       // No error
#define         ERROR       (-127)      // Error

/* ---------------------------- */
/* Simulation termination codes */
/* ---------------------------- */
#define         EXIT_msglimit   0       // Message number limit
#define         EXIT_stimelimit 1       // Simulated time limit
#define         EXIT_rtimelimit 2       // Real (CPU) time limit
#define         EXIT_noevents   3       // Event queue empty
#define         EXIT_user       4       // User-requested
#define         EXIT_abort      5       // Aborted

/* --------- */
/* Wildcards */
/* --------- */
#define         NONE            (-1)    		// Dummy parameter
#define         ANY             ((IPointer)(-1))    	// For observers

/* -------------------------------- */
/* Integer (and TIME) type ordinals */
/* -------------------------------- */
#define         TYPE_long       0       // Long integer (standard)
#define		TYPE_LONG	TYPE_long
#define         TYPE_short      1       // Short integer
#define         TYPE_BIG        2       // TIME
#define		TYPE_double	3
#define		TYPE_int	4
#define		TYPE_hex	5	// Last two used solely by parseNumbers

/* ---------------------------------- */
/* Seeds for random number generators */
/* ---------------------------------- */
#define         SEED_traffic    0       // Traffic seed
#define         SEED_delay      1       // Spacing seed
#define         SEED_toss       2       // Toss-a-coin seed
#define         ZZ_N_SEEDS      (SEED_toss+1)

/* ----------- */
/* Link events */
/* ----------- */

#if	ZZ_NOL || ZZ_NOR
/*
 * These also apply to radio channels
 */
#define         SILENCE         0
#define         ACTIVITY        1
#define         BOT             4       // Start of packet
#define         EOT             5       // End of transfer
#define         BMP             6       // Start of packet addressed to me
#define         EMP             7       // End of packet addressed to me
#define         ANYEVENT        8       // Any status change of the carrier

#define		AEV_MODE_PERSISTENT	0	// Persistent ANYEVENT mode
#define		AEV_MODE_TRANSITION	1	// Non-persistent

/* ---------------------------------------------------------- */
/* User-visible  Bus  activity  types  (returned  in TheEvent */
/* after ANYEVENT)                                            */
/* ---------------------------------------------------------- */
#define         JAM                     0
#define         TRANSFER                1

#endif

#if	ZZ_NOR
/*
 * Radio channels only
 */
#define		BOP		2	// Beginning of preamble
#define		EOP		3	// End of preamble

#define		BERROR		9	// Bit error event

#define		INTLOW		10	// Interference lower than threshold
#define		INTHIGH		11	// Interference higher than threshold

#define		SIGLOW		12	// Signal low
#define		SIGHIGH		13	// Signal high

#define		ZZ_N_RFCHANNEL_EVENTS	(SIGHIGH+1)

#define	ZZ_N_TRANSCEIVER_EVENTS	ZZ_N_RFCHANNEL_EVENTS

#define	PREAMBLE	JAM	// These are activity types, not events
#define	ENDPREAMBLE	ENDJAM

/*
 * sigLevel ordinals
 */
#define	SIGL_OWN	0	// activity's onw signal level
#define	SIGL_IFM	1	// maximum interference
#define	SIGL_IFA	2	// average interference
#define	SIGL_IFC	3	// current interference

#endif

#if	ZZ_NOL
/*
 * Wired links only
 */
#define         BOJ             2       // Start of jam
#define         EOJ             3       // End of jam
#define         COLLISION       9
#define         ZZ_MY_COLLISION 10      // Invisible to the user
#define		ABTTRANSFER	10	// A special value for ANYEVENT

#define         ZZ_N_LINK_EVENTS        (ZZ_MY_COLLISION+1)

/* --------------------------- */
/* Link fault processing types */
/* --------------------------- */
#define         FT_LEVEL0               0
#define         FT_LEVEL1               1
#define         FT_LEVEL2               2

/* ---------- */
/* Link types */
/* ---------- */
#define         LT_broadcast            0       // Standard ether-type
#define         LT_cpropagate           1       // With propagating collisions
#define         LT_unidirectional       2       // Segmented fiber optic
#define         LT_pointtopoint         3       // Straight unidirectional

#endif	/* NOL */

/* -------------- */
/* Mailbox events */
/* -------------- */
#define         EMPTY                   0
#define         OUTPUT                  (-1)
#define		SENTINEL		(-2)
#define         GET                     (-3)
#define         NEWITEM                 (-4)
#define		UPDATE			NEWITEM
#define         PUT                     NEWITEM
#define         NONEMPTY                (-5)
#define         RECEIVE                 (-6)

/* -------------------- */
/* Mailbox socket flags */
/* -------------------- */
#define         LOCAL                   0
#define         INTERNET                1
#define		DEVICE			2
#define		ZZ_MTYPEMASK		3
#define         CLIENT                  0
#define         SERVER                  4
#define         WAIT                    8
#define         NOWAIT                  0
#define		RAW   			WAIT
#define		COOKED			NOWAIT
#define		READ			16
#define		WRITE			32
#define		MASTER                  64	// Master socket for connection

/* ---------------------------------- */
/* Events generated by the process AI */
/* ---------------------------------- */
#define         DEATH                   (-1)
#define         SIGNAL                  (-2)
                                                // -3 = START -> system.h
#define         CLEAR                   (-4)    // Signal repository empty
#if ZZ_REA
#else
#define         STALL                   (-5)    // No events
#endif

#define		CHILD			(-6)	// Child termination

/* ---------------------------- */
/* Forward definitions of types */
/* ---------------------------- */
#if	ZZ_NOS
class RVariable;
#endif

#if	ZZ_NOC
class SGroup;
class CGroup;
class Message;
class Packet;
class Traffic;
class ZZ_PFItem;
#endif

#if	ZZ_NOL
class Port;
class Link;
class ZZ_LINK_ACTIVITY;
#endif

#if	ZZ_NOR
class Transceiver;
class RFChannel;
#endif

class ZZ_Object;
class EObject;
class AI;
class Mailbox;
class Station;
class Process;
class Observer;

void terminate (Process*);
void terminate (Observer*);

class ZZ_QITEM;
class ZZ_SIG;
class ZZ_EVENT;
class ZZ_REQUEST;
class ZZ_INSPECT;
class ZZ_WINDOW;
class ZZ_SYSTEM;

#define         ZZ_EMD_0        4096    // Display expose offset

/* --------------------------------------------- */
/* Response after putting an item into a mailbox */
/* --------------------------------------------- */
#define         ACCEPTED        0       // Immediately taken
#define         REJECTED        1       // Rejected and ignored (mailbox full)
#define         QUEUED          2       // Accepted into the mailbox

/* ---------- */
/* AI classes */
/* ---------- */

#if	ZZ_NOC
#define         AIC_client      1
#define         AIC_traffic     5
#endif

#if	ZZ_NOL
#define         AIC_link        3
#define         AIC_port        4
#endif

#if	ZZ_NOR
#define		AIC_rfchannel	13
#define		AIC_transceiver	14
#endif

#if	ZZ_NOS
#define         OBJ_rvariable   8
#endif

#define         AIC_timer       0
#define         AIC_mailbox     2
#define         AIC_process     6
#define         AIC_observer    7
#define         OBJ_station     11      // Three honorary AIs
#define         OBJ_eobject     12      // User-created displayable
#define		AIC_monitor	15

/* ------------------------- */
/* A few function prototypes */
/* ------------------------- */

/* ----------------------------------------- */
/* Simulation startup (provided by the user) */
/* ----------------------------------------- */
void    ZZRoot ();

/* ------------------------------------------------- */
/* Forces topology construction at the point of call */
/* ------------------------------------------------- */
void    buildNetwork ();

/* ------------------ */
/* Actual termination */
/* ------------------ */
void    zz_done ();
int     zz_noevents ();                 // Run out of events
void	unwait ();			// Cancels wait requests issued so far

#if	ZZ_NOC

/* ---------------------------------------------------------- */
/* Sets  the queue flush flag. A call of this function before */
/* the simulation is started has the same effect as selecting */
/* the -c call option of the simulator.                       */
/* ---------------------------------------------------------- */
void    setFlush ();

/* ---------------------------------------------------------- */
/* Sets  the  limit  on  the  number  of messages that can be */
/* queued at messages awaiting transmission. If this limit is */
/* reached, new generated messages are ignored.               */
/* ---------------------------------------------------------- */
void    setQSLimit (Long lim = MAX_Long);

/* ------------------------------------------- */
/* Cleanup after a nonstandard packet creation */
/* ------------------------------------------- */
void    zz_mkpclean ();

#endif /* NOC */

/* ------------------- */
/* Sets the time units */
/* ------------------- */
void    setEtu (double);
void    setItu (double);
void	setDu (double);

#if  ZZ_REA
#if  ZZ_NFP
// We are hardwired at 1us
#define Second 1000000
#else

#define Second Etu
#define setSecond(a) setEtu (a)

#endif
#endif

#if  ZZ_RSY
// Exclusive with REA
extern	Long ZZ_ResyncMsec;
void setResync (Long, double);
#define Second Etu
#define setSecond(a) setEtu (a)
#endif

/* ---------------------------------------------------------- */
/* Sets  the  clock  tolerance parameters. They are in effect */
/* for all stations defined since the  call  until  the  next */
/* call.                                                      */
/* ---------------------------------------------------------- */
void    setTolerance (double t = 0.0, int q = 2);
double	getTolerance (int *q = NULL);

/* -------------------- */
/* Exception processing */
/* -------------------- */
int     excptn (const char*, ...);      // Aborts the simulation
#if     ZZ_AER
int     zz_aerror (const char*);        // Same, but after arithmetic error
#endif
void    zz_ierror (const char*);        // Fatal error before starting

#if	ZZ_NFP

void 	zz_nfp (const char*);
Long	cpuTime ();			// If no FP, this is in seconds

#else	/* NFP */

double  cpuTime ();                     // Gives the CPU execution time

#define	Distance_inf	HUGE
#define	Distance_1	1.0
#define	Distance_0	0.0
#define	Time_0		0.0
#define	Time_1		1.0
#define	Time_inf	HUGE

extern	double		Du, Itu, Etu;

#endif	/* NFP */

double	pow_int (double, int);

char    *tDate ();                      // Gives current date and time
Long getEffectiveTimeOfDay (Long *usec = NULL);

/* --------------------------------------------------- */
/* Encodes long numbers with optional decimal exponent */
/* --------------------------------------------------- */
void    encodeLong (LONG, char*, int nc = 10);
void    encodeInt (int i, char *s, int nc = 5);


#if	ZZ_NOS

/* ------------------------ */
/* Combine random variables */
/* ------------------------ */
void   combineRV (RVariable*, RVariable*, RVariable*);
inline void combineRV (RVariable *a, RVariable *b) { combineRV (a, b, b); };
inline void combineRV (RVariable &a, RVariable &b, RVariable &c) {
		combineRV (&a, &b, &c);
	    };
inline void combineRV (RVariable &a, RVariable &b) { combineRV (&a, &b, &b); };

#endif	/* NOS */

/* ----------- */
/* Basic types */
/* ----------- */

/* ------------------------------------- */
/* Binary flags (e.g. packet attributes) */
/* ------------------------------------- */
typedef         Long            FLAGS;
typedef         unsigned char   Boolean;
typedef         unsigned char   boolean;
typedef		unsigned char	byte;

// Macros telling whether we are being traced =================================

#if ZZ_DBG

extern Boolean zz_debug_tracing_on ();

#define	DebugTracing	(zz_debug_tracing_on ())

#else
#define	DebugTracing	0
#endif

extern Boolean zz_tracing_on ();

#define	Tracing	(zz_tracing_on ())

// ============================================================================

/* ---------- */
/* BIG / TIME */
/* ---------- */

#if     BIG_precision > 1

/* ----------------------------------------------------- */
/* Definitions for multiple precision arithmetic package */
/* ----------------------------------------------------- */

class   BIG     {

	protected:

	LONG x [BIG_precision];

	public:
#if  ZZ_TAG
	friend  class   ZZ_TBIG;
#endif
	inline  BIG () { };                     // Dummy constructor

	inline  BIG (const BIG& a) {            // Assignment of big to big
#if     BIG_precision > 5
		bcopy ((BCPCASTS)(a.x), (BCPCASTD)x,
			sizeof (LONG) * BIG_precision);
#else
		x [0] = a.x [0];
		x [1] = a.x [1];
#if     BIG_precision > 2
		x [2] = a.x [2];
#if     BIG_precision > 3
		x [3] = a.x [3];
#if     BIG_precision > 4
		x [4] = a.x [4];
#endif
#endif
#endif
#endif
	};

/* ----------- */
/* Conversions */
/* ----------- */

	INLINE  BIG (LONG i);                   // From long integer
	BIG     (double);                       // Conversion from double/float

	// The following dummy conversion is used as a means of initializing
	// the package which must be done before any user generated conversion
	// (e.g. at static initialization of BIG from int or double) is
	// attempted (see the beginning of mpartihm.c)
	BIG     (BIG*);

	operator        LONG () const;          // Conversion to long/int
	operator        double () const;        // Conversion to float/double

	// To check convertibility to double
	friend  int     convertible (const BIG&);
	// Internal
	friend  void    zz_shiftB (BIG&, int);

/* ----------------------------------------- */
/* Basic arithmetic relations and operations */
/* ----------------------------------------- */

	friend  INLINE  int  operator== (const BIG &a, const BIG &b);
	friend  INLINE  int  operator!= (const BIG &a, const BIG &b);

	friend  inline  int  operator== (const BIG &a, LONG b);
	friend  inline  int  operator!= (const BIG &a, LONG b);
	friend  inline  int  operator== (LONG a, const BIG &b);
	friend  inline  int  operator!= (LONG a, const BIG &b);
	friend  inline  int  operator== (const BIG &a, double b);
	friend  inline  int  operator!= (const BIG &a, double b);
	friend  inline  int  operator== (double a, const BIG &b);
	friend  inline  int  operator!= (double a, const BIG &b);

	friend  int     operator<  (const BIG&, const BIG&);  // Inequalities
	friend  int     operator<= (const BIG&, const BIG&);
	friend  int     operator>  (const BIG&, const BIG&);
	friend  int     operator>= (const BIG&, const BIG&);

	friend  inline int operator< (const BIG &a, LONG b);
	friend  inline int operator< (LONG a, const BIG &b);
	friend  inline int operator< (const BIG &a, double b);
	friend  inline int operator< (double a, const BIG &b);
	friend  inline int operator<= (const BIG &a, LONG b);
	friend  inline int operator<= (LONG a, const BIG &b);
	friend  inline int operator<= (const BIG &a, double b);
	friend  inline int operator<= (double a, const BIG &b);
	friend  inline int operator> (const BIG &a, LONG b);
	friend  inline int operator> (LONG a, const BIG &b);
	friend  inline int operator> (const BIG &a, double b);
	friend  inline int operator> (double a, const BIG &b);
	friend  inline int operator>= (const BIG &a, LONG b);
	friend  inline int operator>= (LONG a, const BIG &b);
	friend  inline int operator>= (const BIG &a, double b);
	friend  inline int operator>= (double a, const BIG &b);

	friend  BIG     operator+ (const BIG&, const BIG&); // Addition
	friend  BIG     operator+ (const BIG&, LONG);       // Addition of LONG
	friend  double  operator+ (const BIG&, double);     // Addition of dble

	friend  inline BIG operator+ (LONG a, const BIG &b);
	friend  inline double operator+ (double a, const BIG &b);

	friend  BIG     operator- (const BIG&, const BIG&); // Subtraction
	friend  BIG     operator- (const BIG&, LONG);       // ... of int
	friend  double  operator- (const BIG&, double);     // ... float
	friend  double  operator- (double, const BIG&);     // ... from float

	inline  BIG     operator++ () {                 // Autoincrement
		return (*this = *this + (LONG)1);
	};
	inline  BIG     operator-- () {                 // Autodecrement
		return (*this = *this - (LONG)1);
	};
	inline  BIG     operator+= (const BIG a) {
		return (*this = *this + a);
	};
	inline  BIG     operator-= (const BIG a) {
		return (*this = *this - a);
	};
	inline  BIG     operator+= (LONG a) {
		return (*this = *this + a);
	};
	inline  BIG     operator-= (LONG a) {
		return (*this = *this - a);
	};

	friend  BIG     operator* (const BIG&, const BIG&); // Multiplication
	friend  BIG     operator* (const BIG&, LONG);       // By LONG
	friend  double  operator* (const BIG&, double);     // By double

	friend  inline BIG operator* (LONG a, const BIG &b);
	friend  inline double operator* (double, const BIG&);

	friend  BIG     operator/ (const BIG&, const BIG&); // Division
	friend  double  operator/ (const BIG&, double);     // By double
	friend  double  operator/ (double, const BIG&);     // Of double

	friend  inline BIG operator/ (const BIG&, LONG);

	friend  BIG     operator% (const BIG&, const BIG&); // Modulo
	friend  LONG    operator% (const BIG&, LONG);       // By LONG

	inline  BIG     operator*= (const BIG a) {
		return (*this = *this * a);
	};
	inline  BIG     operator*= (LONG a) {
		return (*this = *this * a);
	};
	inline  BIG     operator/= (const BIG a) {

		return (*this = *this / a);
	};
	inline  BIG     operator/= (LONG a) {

		return (*this = *this / a);
	};
	inline  BIG     operator%= (const BIG a) {

		return (*this = *this % a);
	};
	inline  BIG     operator%= (LONG a) {

		return (*this = *this % a);
	};

	friend  BIG     atob (const char*);     // String to BIG conversion

						// And vice versa
	friend  char    *btoa (const BIG&, char *s = NULL, int nc = 15);

#if	ZZ_NFP
	inline  BIG (float a) { zz_nfp ("BIG (float)"); };
	inline  operator float () const {
                                           zz_nfp ("float (BIG)");
					   return 0.0;
	};
#else
	inline  BIG (float a) { *this = (double) a; };
	inline  operator float () const { return ((float)((double) (*this))); };
#endif

#if ZZ_LONG_is_not_long
	INLINE  BIG (long);

	friend  inline  int  operator== (const BIG &a, long b) {
		return (a == (BIG) b);
	};
	friend  inline  int  operator!= (const BIG &a, long b) {
		return (a != (BIG) b);
	};
        friend  inline  int  operator== (long a, const BIG &b) {
		return ((BIG) a == b);
	};
	friend  inline  int  operator!= (long a, const BIG &b) {
		return ((BIG) a != b);
	};
        friend  inline int operator< (const BIG &a, long b) {
		return (a < (BIG) b);
	};
	friend  inline int operator< (long a, const BIG &b) {
		return ((BIG) a < b);
	};
	friend  inline int operator<= (const BIG &a, long b) {
		return (a <= (BIG) b);
	};
	friend  inline int operator<= (long a, const BIG &b) {
		return ((BIG) a <= b);
	};
	friend  inline int operator> (const BIG &a, long b) {
		return (a > (BIG) b);
	};
	friend  inline int operator> (long a, const BIG &b) {
		return ((BIG) a > b);
	};
	friend  inline int operator>= (const BIG &a, long b) {
		return (a >= (BIG) b);
	};
	friend  inline int operator>= (long a, const BIG &b) {
		return ((BIG) a >= b);
	};
	friend  inline BIG operator+ (const BIG &a, long b) {
		return (a + (LONG)b);
	};
	friend  inline BIG operator+ (long a, const BIG &b) {
		return (b + (LONG) a);
	};
	friend  inline BIG operator- (const BIG &a, long b) {
		return (a - (LONG)b);
	};

	inline  BIG     operator+= (long a) {
		return (*this = *this + (LONG) a);
	};
	inline  BIG     operator-= (long a) {
		return (*this = *this - (LONG) a);
	};

	friend  inline BIG operator* (const BIG &a, long b) {
		return (a * (LONG) b);
	};
	friend  inline BIG operator* (long a, const BIG &b) {
		return (b * (LONG) a);
	};
        friend  inline BIG operator/ (const BIG &a, long b) {
		return (a / (BIG) b);
	};
	friend  inline long operator% (const BIG &a, long b) {
		return ((long)(a % (LONG) b));
	};

	inline  BIG     operator*= (long a) {
		return (*this = *this * (LONG) a);
	};
	inline  BIG     operator/= (long a) {
		return (*this = *this / (LONG) a);
	};
	inline  BIG     operator%= (long a) {
		return (*this = (BIG) (*this % (LONG) a));
	};
#endif

#if	ZZ_NFP
	friend  inline  int  operator== (const BIG &a, float b) {
		zz_nfp ("BIG == float"); return 0;
	};
	friend  inline  int  operator!= (const BIG &a, float b) {
		zz_nfp ("BIG != float"); return 0;
	};
	friend  inline  int  operator== (float a, const BIG &b) {
		zz_nfp ("float == BIG"); return 0;
	};
	friend  inline  int  operator!= (float a, const BIG &b) {
		zz_nfp ("float != BIG"); return 0;
	};
	friend  inline int operator< (float a, const BIG &b) {
		zz_nfp ("float < BIG"); return 0;
	};
	friend  inline int operator< (const BIG &a, float b) {
		zz_nfp ("BIG < float"); return 0;
	};
	friend  inline int operator<= (const BIG &a, float b) {
		zz_nfp ("BIG <= float"); return 0;
	};
	friend  inline int operator<= (float a, const BIG &b) {
		zz_nfp ("float <= BIG"); return 0;
	};
	friend  inline int operator> (const BIG &a, float b) {
		zz_nfp ("BIG > float"); return 0;
	};
	friend  inline int operator> (float a, const BIG &b) {
		zz_nfp ("float > BIG"); return 0;
	};
	friend  inline int operator>= (float a, const BIG &b) {
		zz_nfp ("float >= BIG"); return 0;
	};
	friend  inline int operator>= (const BIG &a, float b) {
		zz_nfp ("BIG >= float"); return 0;
	};
	friend  inline float operator+ (const BIG &a, float b) {
		zz_nfp ("BIG + float"); return 0.0;
	};
	friend  inline float operator+ (float a, const BIG &b) {
		zz_nfp ("float + BIG"); return 0.0;
	};
	friend  inline float operator- (float a, const BIG &b) {
		zz_nfp ("float - BIG"); return 0.0;
	};
	friend  inline float operator- (const BIG &a, float b) {
		zz_nfp ("BIG - float"); return 0.0;
	};
	friend  inline double  operator* (const BIG &a, float b) {
		zz_nfp ("BIG * float"); return 0.0;
	};
	friend  inline double operator* (float a, const BIG &b) {
		zz_nfp ("float * BIG"); return 0.0;
	};
	friend  inline float operator/ (const BIG &a, float b) {
		zz_nfp ("BIG / float"); return 0.0;
	};
	friend  inline float operator/ (float a, const BIG &b) {
		zz_nfp ("float / BIG"); return 0.0;
	};
#else
	friend  inline  int  operator== (const BIG &a, float b) {
		return (a == (BIG) b);
	};
	friend  inline  int  operator!= (const BIG &a, float b) {
		return (a != (BIG) b);
	};
	friend  inline  int  operator== (float a, const BIG &b) {
		return ((BIG) a == b);
	};
	friend  inline  int  operator!= (float a, const BIG &b) {
		return ((BIG) a != b);
	};
	friend  inline int operator< (float a, const BIG &b) {
		return ((BIG) a < b);
	};
	friend  inline int operator< (const BIG &a, float b) {
		return (a < (BIG) b);
	};
	friend  inline int operator<= (const BIG &a, float b) {
		return (a <= (BIG) b);
	};
	friend  inline int operator<= (float a, const BIG &b) {
		return ((BIG) a <= b);
	};
	friend  inline int operator> (const BIG &a, float b) {
		return (a > (BIG) b);
	};
	friend  inline int operator> (float a, const BIG &b) {
		return ((BIG) a > b);
	};
	friend  inline int operator>= (float a, const BIG &b) {
		return ((BIG) a >= b);
	};
	friend  inline int operator>= (const BIG &a, float b) {
		return (a >= (BIG) b);
	};
	friend  inline float operator+ (const BIG &a, float b) {
		return ((float)(((double) a) + b));
	};
	friend  inline float operator+ (float a, const BIG &b) {
		return ((float)(((double) b) + a));
	};
	friend  inline float operator- (float a, const BIG &b) {
		return ((float)(((double)a) - b));
	};
	friend  inline float operator- (const BIG &a, float b) {
		return ((float)(((double) a) - b));
	};
	friend  inline double  operator* (const BIG &a, float b) {
		return (a * (double) b);
	};
	friend  inline double operator* (float a, const BIG &b) {
		return (((double)b) * a);
	};
	friend  inline float operator/ (const BIG &a, float b) {
		return ((float)(a / (double)b));
	};
	friend  inline float operator/ (float a, const BIG &b) {
		return ((float)((double)a / b));
	};
#endif

//#if ZZ_SOL > 4

	INLINE  BIG (int);

	friend  inline  int  operator== (const BIG &a, int b) {
		return (a == (LONG) b);
	};
	friend  inline  int  operator!= (const BIG &a, int b) {
		return (a != (LONG) b);
	};
	friend  inline  int  operator== (int a, const BIG &b) {
		return ((LONG) a == b);
	};
	friend  inline  int  operator!= (int a, const BIG &b) {
		return ((LONG) a != b);
	};
	friend  inline int operator< (const BIG &a, int b) {
		return (a < (LONG) b);
	};
	friend  inline int operator< (int a, const BIG &b) {
		return ((LONG) a < b);
	};

	friend  inline int operator<= (const BIG &a, int b) {
		return (a <= (LONG) b);
	};
	friend  inline int operator<= (int a, const BIG &b) {
		return ((LONG) a <= b);
	};
	friend  inline int operator> (const BIG &a, int b) {
		return (a > (LONG) b);
	};
	friend  inline int operator> (int a, const BIG &b) {
		return ((LONG) a > b);
	};
	friend  inline int operator>= (const BIG &a, int b) {
		return (a >= (LONG) b);
	};
	friend  inline int operator>= (int a, const BIG &b) {
		return ((LONG) a >= b);
	};
	friend  inline BIG operator+ (const BIG &a, int b) {
		return (a + (LONG)b);
	};
	friend  inline BIG operator+ (int a, const BIG &b) {
		return (b + (LONG) a);
	};
	friend  inline BIG operator- (const BIG &a, int b) {
		return (a - (LONG)b);
	};
	inline  BIG     operator+= (int a) {
		return (*this = *this + (LONG) a);
	};
	inline  BIG     operator-= (int a) {
		return (*this = *this - (LONG) a);
	};
	friend  inline BIG operator* (const BIG &a, int b) {
		return (a * (LONG) b);
	};
	friend  inline BIG operator* (int a, const BIG &b) {
		return (b * (LONG) a);
	};
	friend  inline BIG operator/ (const BIG &a, int b) {
		return (a / (long) b);
	};
	friend  inline int operator% (const BIG &a, int b) {
		return ((int)(a % (LONG) b));
	};
	inline  BIG     operator*= (int a) {
		return (*this = *this * (LONG) a);
	};
	inline  BIG     operator/= (int a) {
		return (*this = *this / (LONG) a);
	};
	inline  BIG     operator%= (int a) {
		return (*this = (BIG) (*this % (LONG) a));
	};
//#endif
	
};
	// Note: the following garbage could have been written with
	// the 'friend' specifications above. I had to move it here
	// to circumvent a bug in g++ version 2.3.1 which happens
	// to be a malicious piece of sh*t.

inline  int  operator== (const BIG &a, LONG b) {
	return (a == (BIG) b);
};
inline  int  operator!= (const BIG &a, LONG b) {
	return (a != (BIG) b);
};
inline  int  operator== (LONG a, const BIG &b) {
	return ((BIG) a == b);
};
inline  int  operator!= (LONG a, const BIG &b) {
	return ((BIG) a != b);
};
inline int operator< (const BIG &a, LONG b) {
	return (a < (BIG) b);
};
inline int operator< (LONG a, const BIG &b) {
	return ((BIG) a < b);
};
inline int operator<= (const BIG &a, LONG b) {
	return (a <= (BIG) b);
};
inline int operator<= (LONG a, const BIG &b) {
	return ((BIG) a <= b);
};
inline int operator> (const BIG &a, LONG b) {
	return (a > (BIG) b);
};
inline int operator> (LONG a, const BIG &b) {
	return ((BIG) a > b);
};
inline int operator>= (const BIG &a, LONG b) {
	return (a >= (BIG) b);
};
inline int operator>= (LONG a, const BIG &b) {
	return ((BIG) a >= b);
};
inline BIG operator+ (LONG a, const BIG &b) {
	return (b + a);
};
inline BIG operator* (LONG a, const BIG &b) {
	return (b * a);
};
inline BIG operator/ (const BIG &a, LONG b) {
	return (a / (BIG) b);
};

#if	ZZ_NFP

inline  int  operator== (const BIG &a, double b) {
	zz_nfp ("BIG == double"); return 0;
};
inline  int  operator!= (const BIG &a, double b) {
	zz_nfp ("BIG != double"); return 0;
};
inline  int  operator== (double a, const BIG &b) {
	zz_nfp ("double == BIG"); return 0;
};
inline  int  operator!= (double a, const BIG &b) {
	zz_nfp ("double != BIG"); return 0;
};
inline int operator< (const BIG &a, double b) {
	zz_nfp ("BIG < double"); return 0;
};
inline int operator< (double a, const BIG &b) {
	zz_nfp ("double < BIG"); return 0;
};
inline int operator<= (const BIG &a, double b) {
	zz_nfp ("BIG <= double"); return 0;
};
inline int operator<= (double a, const BIG &b) {
	zz_nfp ("double <= BIG"); return 0;
};
inline int operator> (const BIG &a, double b) {
	zz_nfp ("BIG > double"); return 0;
};
inline int operator> (double a, const BIG &b) {
	zz_nfp ("double > BIG"); return 0;
};
inline int operator>= (const BIG &a, double b) {
	zz_nfp ("BIG >= double"); return 0;
};
inline int operator>= (double a, const BIG &b) {
	zz_nfp ("double >= BIG"); return 0;
};
inline double operator+ (double a, const BIG &b) {
	zz_nfp ("double + BIG"); return 0.0;
};
inline double operator* (double a, const BIG &b) {
	zz_nfp ("double * BIG"); return 0.0;
};

#else

inline  int  operator== (const BIG &a, double b) {
	return (a == (BIG) b);
};
inline  int  operator!= (const BIG &a, double b) {
	return (a != (BIG) b);
};
inline  int  operator== (double a, const BIG &b) {
	return ((BIG) a == b);
};
inline  int  operator!= (double a, const BIG &b) {
	return ((BIG) a != b);
};
inline int operator< (const BIG &a, double b) {
	return (a < (BIG) b);
};
inline int operator< (double a, const BIG &b) {
	return ((BIG) a < b);
};
inline int operator<= (const BIG &a, double b) {
	return (a <= (BIG) b);
};
inline int operator<= (double a, const BIG &b) {
	return ((BIG) a <= b);
};
inline int operator> (const BIG &a, double b) {
	return (a > (BIG) b);
};
inline int operator> (double a, const BIG &b) {
	return ((BIG) a > b);
};
inline int operator>= (const BIG &a, double b) {
	return (a >= (BIG) b);
};
inline int operator>= (double a, const BIG &b) {
	return ((BIG) a >= b);
};
inline double operator+ (double a, const BIG &b) {
	return (b + a);
};
inline double operator* (double a, const BIG &b) {
	return (b * a);
};

#endif

#if ZZ_TAG
class   ZZ_TBIG : public BIG {

	protected:

	LONG tag;

	public:

	inline  ZZ_TBIG () { };                 // Dummy constructor

	inline  ZZ_TBIG (const ZZ_TBIG& a) {    // Assignment of big to big
#if     BIG_precision > 5
		bcopy ((BCPCASTS)(a.x), (BCPCASTD)x,
			sizeof (LONG) * (BIG_precision+1));
#else
		x [0] = a.x [0];
		x [1] = a.x [1];
#if     BIG_precision > 2
		x [2] = a.x [2];
#if     BIG_precision > 3
		x [3] = a.x [3];
#if     BIG_precision > 4
		x [4] = a.x [4];
#endif
#endif
#endif
		tag = a.tag;
#endif
	};

	inline void set (const BIG &a, LONG tg) {

#if     BIG_precision > 5
		bcopy ((BCPCASTS)(a.x), (BCPCASTD)x,
			sizeof (LONG) * BIG_precision);
#else
		x [0] = a.x [0];
		x [1] = a.x [1];
#if     BIG_precision > 2
		x [2] = a.x [2];
#if     BIG_precision > 3
		x [3] = a.x [3];
#if     BIG_precision > 4
		x [4] = a.x [4];
#endif
#endif
#endif
#endif
		tag = tg;
	};

	inline void set (const BIG &a) {

#if     BIG_precision > 5
		bcopy ((BCPCASTS)(a.x), (BCPCASTD)x,
			sizeof (LONG) * BIG_precision);
#else
		x [0] = a.x [0];
		x [1] = a.x [1];
#if     BIG_precision > 2
		x [2] = a.x [2];
#if     BIG_precision > 3
		x [3] = a.x [3];
#if     BIG_precision > 4
		x [4] = a.x [4];
#endif
#endif
#endif
#endif
	};

	inline void get (BIG &a) {

#if     BIG_precision > 5
		bcopy ((BCPCASTS)x, (BCPCASTD)(a.x),
			sizeof (LONG) * BIG_precision);
#else
		a.x [0] = x [0];
		a.x [1] = x [1];
#if     BIG_precision > 2
		a.x [2] = x [2];
#if     BIG_precision > 3
		a.x [3] = x [3];
#if     BIG_precision > 4
		a.x [4] = x [4];
#endif
#endif
#endif
#endif
	};

	inline LONG gett () { return tag; };

	int cmp (const ZZ_TBIG&);
	int cmp (const BIG&, LONG);
	int cmp (const BIG&);
	friend  INLINE  int  operator== (const ZZ_TBIG &a, const ZZ_TBIG &b);
};
#endif

#endif

#if     BIG_precision == 1

/* ----------------------------------------------- */
/* Definitions for single precision BIG arithmetic */
/* ----------------------------------------------- */

typedef LONG    BIG;

#if	ZZ_NFP
inline  int  convertible (const BIG&) { return 0; };
#else
inline  int  convertible (const BIG&) { return 1; };
#endif

/* ---------------------------------- */
/* Prototypes of some other functions */
/* ---------------------------------- */

BIG     atob (const char*);                     // String to BIG conversion

char    *btoa (const BIG a, char *s = NULL, int nc = 15);  // And vice versa

#if ZZ_TAG
class   ZZ_TBIG {

	protected:

	LONG x, tag;

	public:

	inline  ZZ_TBIG () { };                           // Dummy constructor

	inline  ZZ_TBIG (const ZZ_TBIG &a) {
		x = a.x;
		tag = a.tag;
	};

	inline void set (const BIG a, LONG tg) {
	  x = a;  tag = tg;
	};

	inline void set (const BIG a) {
	  x = a;
	};

	inline void get (BIG &a) {
	  a = x;
	};

	inline LONG gett () { return tag; };

	inline int cmp (const ZZ_TBIG &a) {
	  if (x < a.x)
	    return (-1);
	  else if (x > a.x)
	    return (1);
	  else if (tag < a.tag)
	    return (-1);
	  else if (tag > a.tag)
	    return (1);
	  else
	    return (0);
	};

	inline int cmp (BIG a, LONG tg) {
	  if (x < a)
	    return (-1);
	  else if (x > a)
	    return (1);
	  else if (tag < tg)
	    return (-1);
	  else if (tag > tg)
	    return (1);
	  else
	    return (0);
	};

	inline int cmp (BIG a) {
	  if (x < a)
	    return (-1);
	  else if (x > a)
	    return (1);
	  return (0);
	};

	friend  inline  int  operator== (const ZZ_TBIG&, const ZZ_TBIG&);
	friend  inline  int  operator== (const ZZ_TBIG&, BIG);
	friend  inline  int  operator== (BIG, const ZZ_TBIG&);
	friend  inline  int  operator!= (const ZZ_TBIG&, BIG);
	friend  inline  int  operator!= (BIG, const ZZ_TBIG&);
	friend  inline  int  operator>= (const ZZ_TBIG&, BIG);
	friend  inline  int  operator<= (const ZZ_TBIG&, BIG);
	friend  inline  int  operator<= (BIG, const ZZ_TBIG&);
	friend  inline  int  operator>= (BIG, const ZZ_TBIG&);
	friend  inline  int  operator<  (const ZZ_TBIG&, BIG);
};
inline  int  operator== (const ZZ_TBIG &a, const ZZ_TBIG &b) {
	return (a.x == b.x && a.tag == b.tag);
};
inline  int  operator== (const ZZ_TBIG &a, BIG b) {
	return (a.x == b);
};
inline  int  operator== (BIG b, const ZZ_TBIG &a) {
	return (a.x == b);
};
inline  int  operator!= (const ZZ_TBIG &a, BIG b) {
	return (a.x != b);
};
inline  int  operator!= (BIG b, const ZZ_TBIG &a) {
	return (a.x != b);
};
inline  int  operator>= (const ZZ_TBIG &a, BIG b) {
	return (a.x >= b);
};
inline  int  operator<= (const ZZ_TBIG &a, BIG b) {
	return (a.x <= b);
};
inline  int  operator<= (BIG b, const ZZ_TBIG &a) {
	return (a.x >= b);
};
inline  int  operator>= (BIG b, const ZZ_TBIG &a) {
	return (a.x <= b);
};
inline  int  operator< (const ZZ_TBIG &a, BIG b) {
	return (a.x < b);
};
#endif

#endif

extern         BIG   BIG_0, BIG_1, BIG_inf;     // Some constants of type big

#define TIME_0          BIG_0
#define TIME_1          BIG_1
#define TIME_inf        BIG_inf

#if     BIG_precision != 1
/* -------------------------------------- */
/* Overload "<<" to print out BIG numbers */
/* -------------------------------------- */
ostream &operator<< (ostream&, const BIG&);

/* --------------------------------- */
/* Overload ">>" to read BIG numbers */
/* --------------------------------- */
istream &operator>> (istream&, BIG&);
#endif

/* ---------------------------------------------------------- */
/* Two macros for recognizing infinite (undefined) BIG values */
/* ---------------------------------------------------------- */
#define         def(a)          ((a) != BIG_inf)
#define         undef(a)        ((a) == BIG_inf)

typedef         BIG             TIME;                   // An alias

/* ------------------------------- */
/* Distances, bit counts and rates */
/* ------------------------------- */

#if     ZZ_DST
	// Distances should be of type BIG
typedef BIG     DISTANCE;
#define TYPE_DISTANCE   TYPE_BIG
#define MAX_DISTANCE    BIG_inf         // Maximum distance
#define DISTANCE_inf    BIG_inf         // Maximum distance
#define DISTANCE_0      BIG_0
#define DISTANCE_1      BIG_1
#else
typedef LONG    DISTANCE;               // Default: type long
#define TYPE_DISTANCE   TYPE_long
#define MAX_DISTANCE    MAX_LONG
#define DISTANCE_inf    MAX_LONG        // Maximum distance
#define DISTANCE_0      (0L)
#define DISTANCE_1      (1L)
#endif

#if     ZZ_BTC
	// Counters should be of type BIG
typedef BIG     BITCOUNT;
#define TYPE_BITCOUNT   TYPE_BIG
#define MAX_BITCOUNT    BIG_inf
#define BITCOUNT_inf    BIG_inf
#define BITCOUNT_0      BIG_0
#define BITCOUNT_1      BIG_1
#else
typedef LONG    BITCOUNT;               // Default: type long
#define TYPE_BITCOUNT   TYPE_long
#define MAX_BITCOUNT    MAX_LONG
#define BITCOUNT_inf    MAX_LONG
#define BITCOUNT_0      (0L)
#define BITCOUNT_1      (1L)
#endif

#if     ZZ_RTS
	// Transfer rates should be BIG
typedef BIG     RATE;
#define TYPE_RATE       TYPE_BIG
#define MAX_RATE        BIG_inf
#define RATE_inf        BIG_inf
#define RATE_0          BIG_0
#define RATE_1          BIG_1
#else
typedef LONG    RATE;                   // Default: type long
#define TYPE_RATE       TYPE_long
#define MAX_RATE        MAX_LONG
#define RATE_inf        MAX_LONG
#define RATE_0          (0L)
#define RATE_1          (1L)
#endif

/* ------------------------ */
/* More function prototypes */
/* ------------------------ */

/* ------------------------- */
/* Setting simulation limits */
/* ------------------------- */
#if	ZZ_NFP

void   setLimit (Long nm, TIME mt, Long ct);

#else	/* NFP */

void   setLimit (Long nm, TIME mt, double ct);

inline double ituToDu (DISTANCE d) {

	if (d == DISTANCE_inf)
		return Distance_inf;
	return (double)d/Du;
};

inline DISTANCE duToItu (double d) {

	if (d == Distance_inf)
		return DISTANCE_inf;
	return (DISTANCE) (((d) * Du) + 0.5);
};

inline	TIME etuToItu (double t) {

	if (t == Time_inf)
		return TIME_inf;
	return (TIME) (t * Etu);
};

inline	double ituToEtu (TIME t) {

	if (t == TIME_inf)
		return Time_inf;

	return ((double) t) * Itu;
};

#endif	/* NFP */

void   setLimit (Long nm, TIME mt);
void   setLimit (Long nm);

/* ------------------------ */
/* Standard input functions */
/* ------------------------ */
void    readIn (LONG&);
#if	ZZ_LONG_is_not_long
// LONG is different from long
void	readIn (long&);
#endif
void    readIn (double&);
#if     BIG_precision != 1
void    readIn (BIG&);
#endif
void    readIn (int&);
void    readIn (float&);

/* ------------------------- */
/* Standard output functions */
/* ------------------------- */
void print (LONG n, const char *h = NULL, int ns = 15, int hs = 0);
inline void print (LONG n, int ns) { print (n, NULL, ns); };
void print (double n, const char *h = NULL, int ns = 15, int hs = 0);
inline void print (double n, int ns) { print (n, NULL, ns); };
#if     ZZ_LONG_is_not_long
inline void print (long n, const char *h = NULL, int ns = 15, int hs = 0) {
	print ((LONG) n, h, ns, hs);
};
inline void print (long n, int ns) { print ((LONG) n, NULL, ns); };
#endif

inline void print (int n, const char *h = NULL, int ns = 15, int hs = 0) {
	print ((LONG) n, h, ns, hs);
};
inline void print (int n, int ns) { print ((LONG) n, NULL, ns); };
inline void print (float n, const char *h = NULL, int ns = 15, int hs = 0) {
#if	ZZ_NFP
	zz_nfp ("print (float, ...)");
#else
	print ((double) n, h, ns, hs);
#endif
};
#if	ZZ_NFP
inline void print (float n, int ns) { zz_nfp ("print (float, ...)"); };
#else
inline void print (float n, int ns) { print ((double) n, NULL, ns); };

typedef	struct	{
/* -------------------- */
/* Used by parseNumbers */
/* -------------------- */
	int	type;		// YES if the number is floating point
	union {
		double	DVal;
		LONG	LVal;
		int	IVal;
	};
} nparse_t;

int parseNumbers (const char*, int, nparse_t*);

#endif	/* Floating point available */

#if     BIG_precision != 1
void print (const BIG &n, const char *h = NULL, int ns = 15, int hs = 0);
inline void print (const BIG &n, int ns) { print (n, NULL, ns); };
#endif
void print (const char *n, int ns = 0);
sxml_t sxml_parse_input (char del = '\0', char **data = NULL, int *len = NULL);

/* ---------------------------------------------------------- */
/* User-invisible global function to issue an inspect request */
/* ---------------------------------------------------------- */
void    zz_inspect_rq (Station*, void*, char*, int, int);

/* ------------------------ */
/* Global display functions */
/* ------------------------ */

/* -------------------------------- */
/* Request display from the program */
/* -------------------------------- */
int     requestDisplay (const char *msg = NULL);

/* ------------------ */
/* Refresh the screen */
/* ------------------ */
void    refreshDisplay ();

/* --------------------------------------------- */
/* Remove an object from the display server pool */
/* --------------------------------------------- */
void    zz_DREM (ZZ_Object*);

/* ------------------------------------------------ */
/* User-callable dynamic display interface routines */
/* ------------------------------------------------ */

void    displayNote (const char*);      // Send a message to display server
void    display (LONG);                 // Display an integer value
void    display (double);               // Display a fp number
#if     ZZ_LONG_is_not_long
inline  void    display (long a) { display ((LONG)a); };
#endif
inline  void    display (int a) { display ((LONG)a); };
#if	ZZ_NFP
inline  void    display (float a) { zz_nfp ("display (float)"); };
#else
inline  void    display (float a) { display ((double)a); };
#endif
#if     BIG_precision != 1
void    display (BIG);                  // Display a BIG number
#endif
#if  ZZ_TAG
void    display (ZZ_TBIG&);
#endif
void    display (const char*);          // Display text
					// Display a single-segment region
void    display (double, double, double, double, Long, int, double*, double*);
					// Unscaled single-segment region
void    display (Long, int, double*, double*);
void    display (int, double*, double*);// As above with default attributes
					// Start a multi-segment region
void    display (char);                // For a single character

void    startRegion (double, double, double, double);
void    startRegion ();                 // Unscaled multi-segment
void    endRegion ();                   // Terminate a multi-segment region
					// Display a single segment of a msr
void    displaySegment (Long, int, double*, double*);
					// The same with default attributes
void    displaySegment (int n, double*, double*);
void    startSegment (Long att = NONE); // Start a segment (point mode)
void    displayPoint (double, double);  // Display a single point
void    endSegment ();                  // Close a segment (point mode)

/* ---------------------------------------------------------- */
/* Data   structures   and   intrinsic  functions  for  event */
/* processing                                                 */
/* ---------------------------------------------------------- */

extern  Station                *TheStation;    // Announcements (see below)
extern  Process                *TheProcess;
extern  Process                *ZZ_Main;
extern  ZZ_REQUEST             *zz_c_other;
extern  ZZ_EVENT               *zz_pe;
extern  ZZ_REQUEST             *zz_pr;
extern  Long                   zz_npee;
extern  TIME                   Time;

extern  Long                   NStations;
extern  Station                **zz_st;

#if	ZZ_NOL
extern  Long                   NLinks;
extern  Link                   **zz_lk;
#endif

#if	ZZ_NOR
extern	Long		       NRFChannels;
extern	RFChannel	       **zz_rf;
#endif

#if 	ZZ_NOC
extern  Long                   NTraffics;
extern  Traffic                **zz_tp;
extern	unsigned int	       zz_pvoffset;
#endif

#if     ZZ_OBS
extern  ZZ_INSPECT             *zz_inlist_tail;
extern  Observer               *zz_current_observer;
extern  int                    zz_observer_running;
#endif
extern  int                    TheState;
extern  void                   *Info01, *Info02, *zz_mbi;
extern  ZZ_EVENT               zz_fsent, zz_rsent, *zz_sentinel_event;
extern  ZZ_REQUEST             *zz_nqrqs;

// Program call arguments passed to the model
extern	int 		       PCArgc;
extern	const char 	       **PCArgv;

// Tracing variables (soft + hard)

extern  TIME                   TracingTimeStart, TracingTimeStop;
extern  Long                   *TracingStations;
extern	Long		       NTracingStations;

#if	ZZ_DBG
extern	TIME		       DebugTracingTimeStart,
			       DebugTracingTimeStop;

extern	Long		       *DebugTracingStations;
extern	Long		       DebugNTracingStations;
#endif

// -------------------------------

extern  volatile int           DisplayActive;
extern  ZZ_SYSTEM              *System;
extern  int                    zz_dispfound;    // A flag for screen exposures
#if     ZZ_REA || ZZ_RSY
extern	Mailbox 	       *zz_socket_list; // List of connected mailboxes
#endif

/* ---------------------------------------------------------- */
/* Random   number  generators  (made  inline  for  increased */
/* efficiency)                                                */
/* ---------------------------------------------------------- */
#if     ZZ_R48

extern  unsigned  short  zz_Seeds [ZZ_N_SEEDS] [ 3 ];

#else

extern  Long    zz_Seeds [ZZ_N_SEEDS];

#if	ZZ_NFP
#else
extern  double  zz_rndd;
#endif

#define         ZZ_RND1         843314861
#define         ZZ_RND2         169366695
#define         ZZ_RND3         453816693

#endif

inline  double  rnd (int seed) {

/* ----------------------------------------------------- */
/* Generates a [0,1) uniformly distributed random number */
/* ----------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("rnd");
#else
#if     ZZ_R48
	return (erand48 (zz_Seeds [seed]));
#else
	register Long           cs;

	if ((cs = zz_Seeds [seed] * ZZ_RND1) > ZZ_RND2) {
		cs = cs - MAX_Long;
		cs --;
	}
	if ((cs += ZZ_RND3) < 0) {
		cs = cs + MAX_Long;
		cs ++;
	}

	return ((double) (zz_Seeds [seed] = cs) / zz_rndd);
#endif
#endif
};

inline LONG    lRndUniform (LONG a, LONG b) {

/* ---------------------------------------------------------- */
/* Generates  a  uniformly  distributed random number of type */
/* LONG                                                       */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("lRndUniform");
	return 0;
#else
	register LONG   res;

	res = (LONG) (a + (b - a + 1) * rnd (SEED_traffic));
	return (res);
#endif
};

inline LONG    lRndUniform (double a, double b) {

/* ---------------------------------------------------------- */
/* Generates  a  uniformly  distributed random number of type */
/* LONG                                                       */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("lRndUniform");
	return 0;
#else
	register LONG   res;

	res = (LONG) (a + (b - a + 1.0) * rnd (SEED_traffic));
	return (res);
#endif
};

inline double dRndUniform (double a, double b) {

/* ---------------------------------------------------------- */
/* Generates  a  uniformly  distributed random number of type */
/* double                                                     */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("dRndUniform");
	return 0;
#else
	return a + (b - a) * rnd (SEED_traffic);
#endif
};

inline TIME    tRndUniform (TIME a, TIME b) {

/* ---------------------------------------------------------- */
/* Generates  a  uniformly  distributed random number of type */
/* TIME                                                       */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("tRndUniform");
	return TIME_0;
#else
	TIME    res;

	res = a + (TIME) (((double)(b - a) + 1.0) * rnd (SEED_traffic));
	return (res > b ? b : res);
#endif
};

inline TIME    tRndUniform (double a, double b) {

/* ---------------------------------------------------------- */
/* Generates  a  uniformly  distributed random number of type */
/* TIME                                                       */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("tRndUniform");
	return TIME_0;
#else
	double  res;

	res = a + ((b - a) + 1.0) * rnd (SEED_traffic);
	if (res > b) res = b;
	return ((TIME) res);
#endif
};

inline LONG lRndPoisson (double mean) {

/* ---------------------------------------------------------- */
/* Exponentially distributed random number of type LONG based */
/* on double or LONG mean                                     */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("lRndPoisson");
	return 0;
#else
	return ((LONG)(-mean * log (1.0 - rnd (SEED_traffic)) + 0.5));
#endif
};

inline double dRndPoisson (double mean) {

/* ------------------------------------------------------------ */
/* Exponentially distributed random number of type double based */
/* on double or LONG mean                                       */
/* ------------------------------------------------------------ */
#if	ZZ_NFP
	zz_nfp ("dRndPoisson");
	return 0;
#else
	return (-mean * log (1.0 - rnd (SEED_traffic)));
#endif
};

inline TIME tRndPoisson (double mean) {

/* ---------------------------------------------------------- */
/* Exponentially distributed random number of type TIME based */
/* on double, LONG, or TIME-type mean                         */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("tRndPoisson");
	return 0;
#else
	return ((TIME)(-mean * log (1.0 - rnd (SEED_traffic)) + 0.5));
#endif
};

inline Long toss (Long n) {

/* ---------------------------------------------------------- */
/* Tossing  a  multi-sided  coin  (a  random  integer  number */
/* between 0 and n-1)                                         */
/* ---------------------------------------------------------- */
#if	ZZ_NFP
	zz_nfp ("toss");
	return 0;
#else
	register double r;

#if     ZZ_R48
	r = erand48 (zz_Seeds [SEED_toss]);
#else
	register Long           cs;

	if ((cs = zz_Seeds [SEED_toss] * ZZ_RND1) > ZZ_RND2) {
		cs = cs - MAX_Long;
		cs --;
	}
	if ((cs += ZZ_RND3) < 0) {
		cs = cs + MAX_Long;
		cs ++;
	}

	r = ((double) (zz_Seeds [SEED_toss] = cs)) / zz_rndd;
#endif
	return ((Long) (n * r));
#endif
};

inline int flip () {

/* ---------------------------------------------------------- */
/* Flipping  a  coin,  the  result  is  0  or 1 with the same */
/* probability                                                */
/* ---------------------------------------------------------- */

// This is the only one that stays if there's no floating point

#if     ZZ_R48
	return ((jrand48 (zz_Seeds [SEED_toss]) & 010000000000) == 0L);
#else
	register Long           cs;

	if ((cs = zz_Seeds [SEED_toss] * ZZ_RND1) > ZZ_RND2) {
		cs = cs - MAX_Long;
		cs --;
	}
	if ((cs += ZZ_RND3) < 0) {
		cs = cs + MAX_Long;
		cs ++;
	}

	zz_Seeds [SEED_toss] = cs;
	return ((cs & 010000000000) == 0L);
#endif
};

INLINE LONG 	lRndTolerance (LONG, LONG, int);
INLINE LONG 	lRndTolerance (double, double, int);
INLINE double	dRndTolerance (double, double, int);
INLINE double 	dRndGauss (double, double);
Long	lRndBinomial (double, Long);
Long	lRndZipf (double s, Long max = MAX_Long, Long v = 1);
TIME	tRndTolerance (TIME, TIME, int);

INLINE TIME    tRndTolerance   (double, double, int);

/* =========================================== */

void zz_processWindowPhrase ();
void zz_adjust_ownership ();

/* ------------------------------------------- */
/* General displayable and/or printable object */
/* ------------------------------------------- */
class   ZZ_Object  {

#if	ZZ_NOC
	friend  class   Message;
	friend  class   Traffic;
	friend  class   zz_client;
	friend  void    zz_start_client ();
#endif

#if	ZZ_NOL
	friend  class   Port;
	friend  class   Link;
#endif

#if	ZZ_NOR
	friend	class	Transceiver;
	friend	class	RFChannel;
#endif

#if	ZZ_NOS
	friend  class   RVariable;
#endif

	friend  class   EObject;
	friend  class   Station;
	friend  class   ZZ_REQUEST;
	friend  class   ZZ_INSPECT;
	friend  class   AI;

	friend  class   zz_timer;
	friend  class   zz_monitor;
	friend  class   Mailbox;
	friend  class   Process;
	friend  class   ZZ_KERNEL;
	friend  class   Observer;
#if     ZZ_DBG
	friend  void    zz_print_debug_info ();
#endif
	friend  int     main (int, char *[]);
	friend  void    zz_adjust_ownership ();
	friend  void    zz_sendObjectMenu (ZZ_Object*);
#ifdef  CDEBUG
	friend  void    zz_dumpTree (ZZ_Object*);
#endif
	friend  ZZ_Object *zz_findAdvance (ZZ_Object*);
	friend  ZZ_Object *zz_findWAdvance (ZZ_Object*);
	friend  void    zz_processWindowPhrase ();
        friend  void    zz_DREM (ZZ_Object*);
	friend  void    zz_inspect_rq (Station*, void*, char*, int, int);
	friend  void    terminate (Process*);
	friend  void    terminate (Observer*);
	friend	void	zz_pcslook (ZZ_Object*);

	// All objects whose contents are to be displayable and/or
	// printable in a natural way must be declared as subobjects of
	// type Object.

	protected:

	ZZ_Object       *next,  // Pointers to put the object on the owner's
			*prev;  // list. Note: these attributes must be the
				// first attributes of Object.

	Long    Id;             // Numerical identifier
	int     Class;          // Object's class

	public:

	char    *zz_nickname;   // The object's nickname (or NULL)

	ZZ_Object () { zz_nickname = NULL; };

	inline  void    setup () { };

	void    zz_start ();

	// Note: the function zz_start is supposed to play the role of a non-
	//       standard constructor. Regular constructors cannot be used
	//       sometimes, as a virtual function called from a superclass
	//       constructor is taken from that superclass rather than the
	//       subclass.

	inline  Long    getId () {
		return (Id);
	};

	inline  int     getClass () {
		return (Class);
	};

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	// Returns the object type identifier
	virtual const char *getTName () { return ("object"); };
	
	// Returns the user-assigned nickname of the object
	const char *getNName ();

	// This is to change the object's nickname
        void setNName (const char*);

	// Returns the standard name of the object. This name consists of the
	// object's type name combined with the Id, provided that the Id is not
	// NONE.
	const char *getSName ();

	// Returns a printable name of an object, i.e. the object's nickname, if
	// one is defined, or the standard name, otherwise.
	const char *getOName ();

	// Returns the base standard name of the object. This name is built
	// similarly to the regular standard name with exception that the
	// base type name is used instead of the actual type name. For
	// instance, the base standard name of any station (of a possibly
	// derived type) is "Station nn".
	const char *getBName ();

	inline  void    printOut (int mode, const char *hdr = NULL,
                                                             Long sid = NONE) {
		zz_expose (mode, hdr, sid);
	};

	inline  void    printOut (int mode, Long sid) {
		zz_expose (mode, NULL, sid);
	};

	inline  void    displayOut (int mode, Long sid = NONE) {
		zz_expose (mode+ZZ_EMD_0, NULL, sid);
	};
};

/* -------------------------------------------------------- */
/* User-visible superclass for defining displayable objects */
/* -------------------------------------------------------- */
class   EObject : public ZZ_Object  {

	private:

	static  Long    sernum;                 // Object serial number

	public:
	void zz_start ();                       // Nonstandard constructor
	virtual const char *getTName () { return ("EObject"); };
	virtual ~EObject ();
};

#if	ZZ_NOS
/* --------- */
/* RVariable */
/* --------- */
class   RVariable : public ZZ_Object {

#if	ZZ_NOC
	friend  class   ZZ_TRVariable;          // Temporary RVariable
#endif
	private:

	// Exposure functions

	void    exPrint0 (const char*);         // Contents
	void    exPrint1 (const char*, int);    // Abbreviated/short contents
	void    exDisplay0 (int reg = YES);     // Full contents region optional
	void    exDisplay1 ();                  // Abbreviated contents

	static Long sernum;                     // Serial number

	protected:

	LONG            *s;                     // The storage

	// Note: this type is implemented in a somewhat tricky way -- mainly
	//       for efficiency reasons.

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	const char    *getTName () {
		return ("RVariable");
	};

	void zz_start ();                       // Nonstandard constructor

	void setup (int type = TYPE_long, int moments = 2);

	~RVariable      ();            		// Deallocates memory

	// The following operator intentionally returns nothing -- to avoid
	// problems with unnecessarily invoked destructors
	void operator= (RVariable&);

	void    erase ();
	void    update (double, LONG inc = 1);  // Add a sample
	void    calculate (double &min, double &max, double *mom, LONG &count);

#if     BIG_precision != 1
	// BIG is different from LONG
	void    update (double, BIG inc);
	void    calculate (double &min, double &max, double *mom, BIG &count);
#endif
	friend  void combineRV (RVariable*, RVariable*, RVariable*);

	inline void     printCnt (const char *hd = NULL) {
		// Print contents
		printOut (0, hd);
	};

	inline void     printACnt (const char *hd = NULL) {
		// Print abbreviated contents
		printOut (1, hd);
	};

	inline void     printSCnt (const char *hd = NULL) {
		// Print short contents
		printOut (2, hd);
	};
};

inline  void    zz_bld_RVariable (char *nn = NULL) {
	register RVariable *p;
	zz_COBJ [++zz_clv] = (void*) (p = new RVariable);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

#endif	/* NOS */

/* ------------------ */
/* Station definition */
/* ------------------ */
class   Station : public ZZ_Object {

#if	ZZ_NOC
	friend  class   SGroup;
	friend  class   CGroup;
	friend  class   Traffic;
	friend  class   zz_client;
	friend  class   Packet;
#endif

#if	ZZ_NOL
	friend  class   Port;
	friend  class   Link;
	friend  Port    *idToPort (int);
#endif

#if	ZZ_NOR
	friend	class		Transceiver;
	friend	class		RFChannel;
	friend	Transceiver	*idToTransceiver (int);
#endif

#if	ZZ_NOS
	friend  class   RVariable;
#endif

	friend  class   ZZ_REQUEST;
	friend  class   Mailbox;
	friend  class   zz_timer;
	friend  class   zz_monitor;
	friend  class   Systat;         // The system station
	friend  class   AI;
	friend  class   Observer;
	friend  class   Process;
	friend  class   ZZ_SYSTEM;
	friend  class   ZZ_INSPECT;
	friend  class   EObject;
	friend  int     main (int, char *[]);
	friend  void    zz_adjust_ownership ();
	friend  void    zz_sendObjectMenu (ZZ_Object*);
	friend  Mailbox *idToMailbox (int);
#ifdef  CDEBUG
	friend  void    zz_dumpTree (ZZ_Object*);
#endif
	friend  ZZ_Object *zz_findAdvance (ZZ_Object*);
	friend  ZZ_Object *zz_findWAdvance (ZZ_Object*);
	friend 	void	zz_pcslook (ZZ_Object*);

	public:

#if	ZZ_NOC
	Message         **MQHead,       // Message queue heads
			**MQTail;       // Message queue tails
#endif
	private:

#if	ZZ_NOC

	ZZ_PFItem       *Buffers;       // The list of buffers
	ZZ_REQUEST      *CWList;        // Client wait requests
#if     ZZ_QSL
	Long            QSLimit,        // Message queue size limit
			QSize;
#endif

#endif	/* NOC */

#if	ZZ_NOL
	Port            *Ports;         // The list of Ports
#endif	/* NOL */

#if	ZZ_NOR
	Transceiver	*Transceivers;
#endif	/* NOR */

	Mailbox         *Mailboxes;     // The list of Mailboxes

	ZZ_Object       *ChList;        // Child list (objects owned)

#if     ZZ_TOL
	double          CTolerance;     // Tolerance parameters
	int             CQuality;
#endif
	static Long sernum;             // Serial number

	// Exposure functions

	void    exPrint0 (const char*); // Processes
	void    exPrint2 (const char*); // Mailboxes
	void    exDisplay0 ();          // Processes
	void    exDisplay2 ();          // Mailboxes

#if	ZZ_NOC
	void    exPrint1 (const char*); // Buffers
	void    exDisplay1 ();          // Buffers
#endif

#if	ZZ_NOL
	void    exPrint3 (const char*); // Activities
	void    exPrint4 (const char*); // Ports
	void    exDisplay3 ();          // Activities
	void    exDisplay4 ();          // Ports
#endif

#if	ZZ_NOR
	void    exPrint5 (const char*); // RF Activities
	void    exPrint6 (const char*); // Transceivers
	void    exDisplay5 ();          // RF Activities
	void    exDisplay6 ();          // Transceivers
#endif
	static  void    pttrav (ZZ_Object*);    // Recursive printer
	static  void    dttrav (ZZ_Object*);    // Recursive displayer

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	void zz_start ();               // Nonstandard constructor

	~Station () {
	   excptn ("Station: [%1d] once created, a station cannot be destroyed",
	      getId ());
	};

	// Kill all processes owned by this station
	void terminate ();

#if	ZZ_TOL
	void	setTolerance (double t, int q);
	double	getTolerance (int *q = NULL);
#endif

#if	ZZ_NOC

	BITCOUNT getMQBits (int tp = NONE);
	Long	 getMQSize (int tp = NONE);

	void    setQSLimit (Long l = MAX_Long); // Sets QSLimit (see above)

	inline  void    printBuf (const char *hd = NULL) {
		// Print buffers
		printOut (1, hd);
	};
#endif	/* NOC */

#if	ZZ_NOL
	INLINE  Port    *idToPort (int);

	inline  void    printAct (const char *hd = NULL) {
		// Print activities
		printOut (3, hd);
	};

	inline  void    printPort (const char *hd = NULL) {
		// Print ports's status
		printOut (4, hd);
	};
#endif	/* NOL */

#if	ZZ_NOR

	INLINE	Transceiver *idToTransceiver (int);

#if	ZZ_R3D
	void	setLocation (double, double, double);
	INLINE	void getLocation (double &x, double &y, double &z);
#else
	void	setLocation (double, double);
	INLINE	void getLocation (double &x, double &y);
#endif
	inline  void    printRAct (const char *hd = NULL) {
		// Print radio activities
		printOut (5, hd);
	};

	inline  void    printTransceiver (const char *hd = NULL) {
		// Print transceiver status
		printOut (6, hd);
	};

#endif	/* NOR */

	INLINE  Mailbox *idToMailbox (int);

	inline  void    printPrc (const char *hd = NULL) {
		// Print processes
		printOut (0, hd);
	};

	inline  void    printMail (const char *hd = NULL) {
		// Prints the contents of the station's mailboxes
		printOut (2, hd);
	};
};

inline  void    zz_bld_Station (char *nn = NULL) {
	register Station *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Station);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

class   ZZ_REQUEST {

/* ------------------------------------ */
/* Description of a single wait request */
/* ------------------------------------ */

#if	ZZ_NOC
	friend  class   zz_client;
	friend  class   Traffic;
#endif

#if	ZZ_NOL
	friend  class   Port;
	friend  class   Link;
#endif

#if	ZZ_NOR
	friend	class	Transceiver;
	friend	class	RFChannel;
#endif

	friend  class   ZZ_EVENT;
	friend  class   ZZ_SIG;
	friend  class   zz_timer;
	friend  class   zz_monitor;
	friend  class   Mailbox;
	friend  class   Process;
	friend  class   Station;
	friend  class   ZZ_KERNEL;
	friend  int     main    (int, char *[]);
	friend  void    terminate (Process*);
        friend  int     zz_noevents ();

	private:

	ZZ_REQUEST      *next,          // For linking requests at AIs
			*prev,

			*other;         // To link requests at events
	ZZ_EVENT        *event;         // Event backpointer
	void            *what;          // A general purpose fullword
#if  ZZ_TAG
	ZZ_TBIG         when;           // Time when the event will occur
#else
	TIME            when;           // Time when the event will occur
#endif
	Long            id;             // Link-relative station id
	int             pstate;         // The state
	AI              *ai;            // AI pointer
	LPointer        event_id;       // Event identifier
	void            *Info01,        // Corresponding to Info01 and Info02
			*Info02;        // for event class

	// Note: the id attribute for a process requests is -3 to speed up
	//       identification of 'special processing upon release' for this
	//       request type

	INLINE ZZ_REQUEST *min_request ();

	public:

	// The constructors

	inline ZZ_REQUEST      (ZZ_REQUEST **q,         // The head
					 AI *a,         // The AI pointer
				  LPointer eid,         // Event identifier
				       int act,         // The state
			       Long sid = NONE,         // L-R station id
				     void *in1 = NULL,  // Info 1
				     void *in2 = NULL,  // Info 2
				     void *wht = NULL)  // Aux
	{
		what = wht;
		id = (sid == NONE) ? TheStation->Id : sid;
		pstate = act;
		ai = a;
		event_id = eid;
		Info01 = in1;
		Info02 = in2;
		if (zz_c_other != NULL) zz_c_other -> other = this;
		zz_c_other = this;
		pool_in (this, *q);
	};

	inline ZZ_REQUEST      (         AI *a,         // The AI pointer
				  LPointer eid,         // Event identifier
				       int act,         // The state
			       Long sid = NONE,         // L-R station id
				     void *in1 = NULL,  // Info 1
				     void *in2 = NULL,  // Info 2
				     void *wht = NULL)  // Aux
	{
		what = wht;
		id = (sid == NONE) ? TheStation->Id : sid;
		pstate = act;
		ai = a;
		event_id = eid;
		Info01 = in1;
		Info02 = in2;
		if (zz_c_other != NULL) zz_c_other -> other = this;
		zz_c_other = this;
		// Use a dummy queue
		pool_in (this, zz_nqrqs);
	};
};

class   ZZ_EVENT {

#if	ZZ_NOC
	friend  class   zz_client;
	friend  class   Traffic;
#endif

#if	ZZ_NOL
	friend  class   Port;
	friend  class   Link;
#endif

#if	ZZ_NOR
	friend	class	Transceiver;
	friend	class	RFChannel;
	friend	class	ZZ_RF_ACTIVITY;
#endif

	friend  class   zz_timer;
	friend  class   zz_monitor;
	friend  class   Mailbox;
	friend  class   Process;
	friend  class   Station;
	friend  class   ZZ_KERNEL;
	friend  class   ZZ_REQUEST;
	friend  class   ZZ_SIG;
	friend  class   Observer;
	friend  class   ObserverService;
	friend  int     main (int, char *[]);
	friend  void    terminate (Observer*);
	friend  void    terminate (Process*);
	friend  void    zz_print_event_info (ostream&);
	friend  int     zz_noevents ();
#if     ZZ_DBG
	friend  void    zz_print_debug_info ();
#endif
#if	ZZ_REA || ZZ_RSY
        friend  void    zz_advance_real_time ();
#endif

	private:

	ZZ_EVENT        *next,          // To link events into event queue
			*prev;

	ZZ_REQUEST      *chain;         // Request chain
#if  ZZ_TAG
	ZZ_TBIG         waketime;       // Time the event is scheduled at
#else
	TIME            waketime;       // Time the event is scheduled at
#endif
	Station         *station;       // Station to be awakened or NULL
	Process         *process;       // The station's process
	int             pstate;         // Action triggered
	AI              *ai;            // AI pointer
	LPointer        event_id;       // Event identifier
	void            *Info01,        // Two general-purpose fullwords to
			*Info02;        // return various information

	INLINE  void    enqueue ();
	INLINE  void    enqueue (ZZ_EVENT *h);

	inline  void    store () {

	// Puts (stores) the event at the end of the time queue

		zz_npee++;

		assert (undef (waketime),
			"EVENT->store: definite time event to store");
		zz_rsent.prev->next = this;
		prev = zz_rsent.prev;
		next = &zz_rsent;
		zz_rsent.prev = this;
	};

	inline  void    remove () {

	// Removes event from the queue without purging it

		pool_out (this);
		zz_npee--;
	};

	inline  void    reschedule () {

	// Reschedule the event

		remove ();
		enqueue ();
	}

	inline  void    new_top_request (ZZ_REQUEST *r) {

	// Change the current request of the event

		waketime = r -> when;
		pstate   = r -> pstate;
		ai       = r -> ai;
		event_id = r -> event_id;
		Info01   = r -> Info01;
		Info02   = r -> Info02;
		chain    = r;

		reschedule ();
	}

	static  INLINE  ZZ_EVENT  *get ();

	public:                         // Constructors

	INLINE  void    cancel ();

	inline  ZZ_EVENT () { };        // Dummy constructor

	// Note: the following constructors are used for system events only

	inline  ZZ_EVENT (TIME wk,
				Station *st,
				void *i_1,
				void *i_2,
				Process *prc,
				AI   *a,
				int  e,
				int  pst,
				ZZ_REQUEST *ch) {

	// Generates a new event

#if  ZZ_TAG
		waketime.set (wk, 0);
#else
		waketime = wk;
#endif
		station  = st;
		Info01   = i_1;
		Info02   = i_2;
		process  = prc;
		ai       = a;
		event_id = e;
		pstate   = pst;
		chain    = ch;

		enqueue ();
	};

	// Generates a new "stored" event with undefined waketime
	INLINE  ZZ_EVENT (
				Station *,
				void *,
				void *,
				Process *,
				AI   *,
				int,
				int,
				ZZ_REQUEST *);

	inline  ZZ_EVENT (ZZ_EVENT *h, TIME wk,
				Station *st,
				void *i_1,
				void *i_2,
				Process *prc,
				AI   *a,
				int  e,
				int  pst,
				ZZ_REQUEST *ch) {

	// Generates a new event with "hint" h

#if  ZZ_TAG
		waketime.set (wk, 0);
#else
		waketime = wk;
#endif
		station  = st;
		Info01   = i_1;
		Info02   = i_2;
		process  = prc;
		ai       = a;
		event_id = e;
		pstate   = pst;
		chain    = ch;

		enqueue (h);
	};
};

/* --------------------- */
/* Activity interpreters */
/* --------------------- */

/* ------------------------------------------ */
/* Common definitions for AI event processing */
/* ------------------------------------------ */

class   AI : public ZZ_Object { // Activity interpreter

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	const char *zz_eid (LPointer);   // Return event identifier
	Long  zz_aid ();           // Return AI absolute id (used in exposures)
};

/* ========== */
/* Monitor AI */
/* ========== */

#define	ZZ_MONITOR_HASH_SIZE	128

class	zz_monitor : public AI {

	private:

	// Exposure functions

	void exPrint0 (const char*, int);
	void exPrint1 (const char*, int);

	void exDisplay0 (int);
	void exDisplay1 (int);

	ZZ_REQUEST	*WList [ZZ_MONITOR_HASH_SIZE];

	inline int hash (void *e) {
		return ((((long)e) >> 3) & (ZZ_MONITOR_HASH_SIZE - 1));
	};

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	zz_monitor ();

#if  ZZ_TAG
	void wait (void*, int, LONG tag = 0L);
#else
	void wait (void*, int);
#endif
	int signal (void*);
	int signalP (void*);

	const char	*getTName () { return ("Monitor"); };

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print full request list
		printOut (0, hd, s);
	};

	inline  void    printARqs (const char *hd = NULL, Long s = NONE) {
		// Print abbreviated request list
		printOut (1, hd, s);
	};
};

extern	zz_monitor  zz_AI_monitor;

// A macro to define object-related event ordinals
#define	MONITOR_LOCAL_EVENT(o)	((void*)(((char*)TheStation)+(o)))

/* -------- */
/* Timer AI */
/* -------- */

class   zz_timer : public AI {

	private:

	// Exposure functions
	void exPrint0 (const char*, int);
	void exPrint1 (const char*, int);

	void exDisplay0 (int);
	void exDisplay1 (int);
         
	ZZ_REQUEST      *WList;        // Processes waiting for timers

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	zz_timer ();    // Constructor

#if  ZZ_TAG
	void wait (TIME, int, LONG tag = 0L);
#if  ZZ_NFP
#else
	inline void delay (double et, int st, LONG tag = 0L) {
		wait (etuToItu (et), st, tag);
	};
#endif
	void zz_proceed (int, LONG tag = 0L);
	void zz_skipto (int, LONG tag = 0L);
#else
	void wait (TIME, int);
#if  ZZ_NFP
#else
	inline void delay (double et, int st) { wait (etuToItu (et), st); };
#endif
	void zz_proceed (int);
	void zz_skipto (int);
#endif
        TIME zz_getdelay (Process*, int);
        int zz_setdelay (Process*, int, TIME);
#if     BIG_precision != 1
#if  ZZ_TAG
	void wait (LONG, int, LONG tag = 0L);
#else
	void wait (LONG, int);
#endif
#endif
	const char    *getTName () { return ("Timer"); };

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print full request list
		printOut (0, hd, s);
	};

	inline  void    printARqs (const char *hd = NULL, Long s = NONE) {
		// Print abbreviated request list
		printOut (1, hd, s);
	};
};

extern  zz_timer  zz_AI_timer;                         // Announcement

/* ------------------- */
/* Mailbox and company */
/* ------------------- */

#if  ZZ_JOU

class ZZ_Journal {
  // Description of a journal file being created
  friend class Mailbox;
 private:
  Mailbox *MB;            // The mailbox being journalled
  int FD;                 // File descriptor of the journal
  char *JName;            // Journal mailbox/file name
  int LeftInBlock;
 public:
  Long WhenSec, WhenUSec;          // Time of next event
  ZZ_Journal *next;                // To link all journals
  char JType;                      // Type: 'J', 'I', 'O'
  ZZ_Journal (const char*, char);
  int openFile ();
  int readBlock (char*, int), readData (char*, int);
  int writeBlock (const char*, int);
  void connectBlock ();
  int skipConnectBlock ();
  void writeData (const char*, int, char);
};

#endif

class   Mailbox : public AI {

	friend  class   Station;
	friend  class   ZZ_Object;
	friend  Mailbox *idToMailbox (int);
	friend  void    zz_adjust_ownership ();
#if  ZZ_REA || ZZ_RSY
#if  ZZ_JOU
        friend  class   ZZ_Journal;
#endif
	friend  void	zz_wait_for_sockets (Long);
	friend  void 	zz_advance_real_time ();
#endif
	friend  int     main (int, char *[]);

	private:

	// Serial number used for numbering mailboxes created
	// after the network initialization phase
	static Long sernum;

	// Exposure funtions

	void    exPrint0 (const char*, int);
	void    exPrint1 (const char*, int);
	void    exPrint2 (const char*, int);

	void    exDisplay0 (int);
	void    exDisplay1 (int);
	void    exDisplay2 ();
        int 	checkImmediate (Long);

	Long            count,          // Mailbox element count
			limit;          // Limit for the number of elements
	ZZ_QITEM        *head, *tail;
	Mailbox         *nextm;         // Linking mboxes of the same station
	ZZ_REQUEST      *WList;         // Event waiting list

#if  ZZ_REA || ZZ_RSY
        int             csd,            // Client socket descriptor
			sfd,            // Socket file descriptor
                        skt;            // Type flags
        char		snt;		// Sentinel
        char            *ibuf,          // Input buffer
                        *obuf;          // Output buffer
        Long            iin, iout,      // Input buffer pointers
                        oin, oout,      // Output buffer pointers

			oldlimit;

        Mailbox         *nexts;         // Next on the socket list

#if  ZZ_JOU
        ZZ_Journal      *JT, *JO;       // Journals
        void    findJournals ();        // Locate journals for this mailbox
#endif

        void    flushOutput (), readInput ();
        int     isWaiting ();
        int     rawWrite (const char*, int);
        int     rawRead (char*, int);
        void	setUnblocking (int which = 0), setBlocking (int which = 0),
		doClose (int which = 0);
	void	destroy_bound ();
#endif

	void	trigger_all_events ();
	
	public:

#if  ZZ_REA || ZZ_RSY
        int  connect (int, const char *nm, int port = 0, int bs = 0,
                                        int sp = 0, int par = 0, int stpb = 0);
        int  connect (int fg, int port, int bs = 0) {
          // A shortcut for server mailbox
          return connect (fg, (char*)NULL, port, bs);
        };
        int  connect (Mailbox *m, int bs = 0) {
          // This may be clumsy, but it is effective
          return connect (NONE, (char*)m, 0, bs);
        };
        int  disconnect (int who = SERVER);
        int  isConnected ();
        int  isPending () {
          // Check if a MASTER mailbox is connection-pending
          return isConnected () && ibuf == NULL && obuf != (void*) NONE;
        };
        int  isActive ();
        int  read (char*, int);              // Block get
        int  readToSentinel (char*, int);    // Block get to sentinel
        int  readAvailable (char*, int);     // Block get available
        int  write (const char*, int);       // Block put
	void setSentinel (char c) { snt = (char) c; };
        int  sentinelFound ();
	Long outputPending ();
	int  resize (int);
#endif
	inline  Long    getCount () { return count; };
	Long		setLimit (Long lim = 0);
	inline	Long	getLimit () { return limit; };
	inline	Long	free () { return limit - count; };

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

#if  ZZ_TAG
	void wait (IPointer, int, LONG tag = 0L);
#else
	void wait (IPointer, int);
#endif

	virtual const char *getTName () { return ("Mailbox"); };

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print full request list
		printOut (0, hd, s);
	};

	inline  void    printARqs (const char *hd = NULL, Long s = NONE) {
		// Print abbreviated request list
		printOut (1, hd, s);
	};

	inline  void    printCnt (const char *hd = NULL) {
		// Print mailbox contents
		printOut (2, hd);
	};

	inline  void    printSCnt (const char *hd = NULL) {
		// Print short contents
		printOut (3, hd);
	};

	void    zz_start ();            // Nonstandard constructor
 	// For static declarations
	Mailbox (Long limit = 0, const char *nn = NULL);
	Mailbox (const char *nn) {
          // Nickname-only version (for static initializers)
          Mailbox (0, nn);
        };

	void    setup (Long limit = 0);

	virtual ~Mailbox ();

	// Default functions (counters only)

	int     get ();                 // YES or NO
	inline  int first () { return (count != 0); };
	int     put ();                 // Default put (no arguments)
	int     putP ();                // Default priority put
	int     erase ();               // Returns number of erased items

	inline  int empty () { return (count == 0); };
	inline  int nonempty () { return !empty (); };
	inline  int full () { return (limit >= 0 && count >= limit); };
	inline  int notfull () { return !full (); };
	inline	int signal (IPointer sig) {
		assert (limit < 0, "Mailbox->signal: %s, not a barrier mailbox",
			getSName ());
		return zz_put ((void*)sig);
	};
	inline	int signalP (IPointer sig) {
		assert (limit < 0, "Mailbox->signalP: %s, not a barrier "
			"mailbox", getSName ());
		return zz_putP ((void*)sig);
	};

	// The following functions are called from instantiations of get/put

	void    *zz_get ();             // Returns object pointer or value      
	void    *zz_first ();           // As above
	int     zz_put (void*);
	int     zz_putP (void*);
	int     zz_erase ();

	Boolean zz_queued (void*);

	virtual void zz_initem () { };  // Dummy
	virtual void zz_outitem () { };
};

inline  void    zz_bld_Mailbox (char *nn = NULL) {
	register Mailbox *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Mailbox);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

class ZZ_QITEM {        // Queue item for the Mailbox AI

	protected:

	ZZ_QITEM        *next;                  // Link
	void            *item;                  // The item pointer

	public:

	inline ZZ_QITEM (void *it) {
		next = NULL;
		item = it;
	};

	friend class Mailbox;
	friend  int     main (int, char *[]);
};

#if	ZZ_NOL
/* ------- */
/* Link AI */
/* ------- */
class   Port : public AI {

	friend  class   Link;
	friend  class   zz_client;
	friend  class   Traffic;
	friend  class   Station;
	friend  class   ZZ_SYSTEM;
	friend  ZZ_LINK_ACTIVITY *zz_gen_activity (int, Port*, int,
		Packet *p = (Packet*) NULL);
	friend  class   LinkService;
	friend  void    setD (Port*, Port*, double);
	friend  void    zz_adjust_ownership ();
	friend  INLINE  Port *idToPort (int);

	private:

	Port             *nextp;        // Next on the station's list

	DISTANCE         *DV,           // Distance vector to other ports
			 MaxDistance;   // Maximum value in DV

	RATE             TRate;         // Transmission rate
	Link             *Lnk;          // Link pointer
	int              LRId;          // Link-relative port id

	ZZ_LINK_ACTIVITY *Activity,     // Station's activity at the port

			 *Interpolator; // Last-sensed activity interpolator
					// (unidirectional links only)

	Boolean		 AevMode;	// ANYEVENT mode

	TIME             IJTime;        // Estimated time when the interpolator
					// will leave the link 

	Boolean transmitting () { return Activity != NULL; };

	// Determines the earliest silence period heard on the port

	TIME    silenceTime ();

	// Determines the earliest collision heard on the port

	TIME    collisionTime (int);

	// Determines when the port will hear the beginning of any
	// activity

	TIME    activityTime ();

	// Determines when the port will hear the nearest beginning of
	// a jamming signal

	TIME    bojTime ();

	// Determines when the port will hear the end of the earliest
	// jamming signal

	TIME    eojTime ();

	// Determines when the port will hear the nearest beginning of a
	// packet

	TIME    botTime ();

	// Determines when the port will hear the nearest end of packet

	TIME    eotTime ();

	// Determines when the port will hear the nearest beginning of
	// a packet addressed to this station

	TIME    bmpTime ();

	// determines when the port will hear the nearest end of a
	// packet addressed to this station

	TIME    empTime ();

	// Determines when the port will perceive the nearest beginning/end
	// of any activity

	TIME    aevTime ();

	// Rescheduling functions

	void    reschedule_act (int);
	void    reschedule_aev (int);
	void    reschedule_mpa ();
	void    reschedule_emp ();
	void    reschedule_col (int);
	void    reschedule_sil (int);
	void    reschedule_boj ();
	void    reschedule_eoj (int);
	void    reschedule_eot ();

	INLINE  ZZ_LINK_ACTIVITY    *initLAI ();

	// A function to find the nearest hint event for the given activity
	INLINE  ZZ_EVENT *findHint (ZZ_LINK_ACTIVITY*);

	// Exposure functions

	void    exPrint0 (const char*);       // Request list
	void    exPrint1 (const char*);       // Activities
	void    exPrint2 (const char*);       // Predicted events

	void    exDisplay0 ();          // Request list
	void    exDisplay1 ();          // Activities
	void    exDisplay2 ();          // Predicted events

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	// Set distance

	void    setDTo (Port*, double);
	void    setDTo (Port&, double);
	void    setDFrom (Port*, double);
	void    setDFrom (Port&, double);

	double	distTo (Port*);

	// Connect to a link

	void    connect (Link *lk, int lrid = NONE);
	void    connect (Long lk, int lrid = NONE);

	// Start packet transmission

	void    startTransfer (Packet*);

	// Transmit packet

#if  ZZ_TAG
	inline void transmit (Packet*, int, LONG tag = 0L);
	inline void transmit (Packet&, int, LONG tag = 0L);
#else
	inline void transmit (Packet*, int);
	inline void transmit (Packet&, int);
#endif

	// Convert bits to time

	INLINE TIME bitsToTime (Long n = 1);

	// Start jamming

	void    startJam ();
	inline void sendJam (TIME d, int st) {
		startJam ();
		zz_AI_timer . wait (d, st);
	};

	// Terminate transfer or jamming signal

	int     stop ();

	// Abort transfer (or stop jamming signal)

	int     abort ();

	// Port inquiries

	TIME    lastCOL ();
	TIME    lastEOA ();

	inline TIME lastBOS () { return (lastEOA ()); };

	TIME    lastBOT ();
	TIME    lastEOT ();
	TIME    lastBOJ ();
	TIME    lastEOJ ();
	TIME    lastBOA ();

	inline TIME lastEOS () { return (lastBOA ()); };

	int     activities (int&, int&);

	inline int  activities (int &t) {
		int junk;
		return (activities (t, junk));
	};

	inline int  activities () {
		int junk1, junk2;
		return (activities (junk1, junk2));
	};

	inline Boolean busy () {
		return (undef (lastEOA ()));
	};

	inline Boolean idle () {
		return (def (lastEOA ()));
	};

	int     events (int);
	Packet  *anotherPacket ();

	// Dumps link activities to the standard output

	inline  void    dump () {
		printOut (1);
	};

	Port ();

	void    zz_start ();                    // Nonstandard constructor

	~Port ();

	const char    *getTName () { return ("Port"); };
	int	getSID (), getYID ();

	// Setup functions

	void    setup (RATE r = RATE_0);

	RATE setTRate (RATE);
	Boolean setAevMode (Boolean);
	inline RATE getTRate () { return TRate; };
	inline Boolean getAevMode () { return AevMode; };

#if  ZZ_TAG
	void wait (int, int, LONG tag = 0L);
#else
	void wait (int, int);
#endif

	inline  void    printRqs (const char *hd = NULL) {
		// Print request list
		printOut (0, hd);
	};

	inline  void    printAct (const char *hd = NULL) {
		// Print activities
		printOut (1, hd);
	};

	inline  void    printEvs (const char *hd = NULL) {
		// Print anticipated event list
		printOut (2, hd);
	};
};

inline  void    zz_bld_Port (char *nn = NULL) {
	register Port *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Port);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

class   Link : public AI {

	friend  class   Port;
	friend  class   Station;
	friend  class   zz_client;
	friend  class   LinkService;
	friend  class   ZZ_SYSTEM;
	friend  ZZ_LINK_ACTIVITY *zz_gen_activity (int, Port*, int, Packet*);
	friend  void    setD (Port*, Port*, double);

	private:

	ZZ_LINK_ACTIVITY  *Alive,               // Alive link activities
			  *Archived,            // Archived activities

			  *AliveTail,           // LT_pointtopoint only
			  *ArchivedTail;        // >= LT_unidirectional

	void (*PCleaner) (Packet*);		// Packet cleaner funtion

#if ZZ_FLK
	double  FRate;                          // Fault rate
	int     FType;                          // Type of fault processing
#endif
	static Long sernum;                     // Serial number

	// Exposure functions

	void    exPrint0 (const char*, int);          // Requests
	void    exPrint1 (const char*);               // Performance
	void    exPrint2 (const char*, int);          // Activities

	void    exDisplay0 (int);               // Requests
	void    exDisplay1 ();                  // Performance
	void    exDisplay2 (int);               // Activities

	void	complete ();			// Completes link definition

	public:

	void    setFaultRate (double r = 0.0, int ft = FT_LEVEL1);
	void    setTRate (RATE);
	void	setAevMode (Boolean);

	void	setPacketCleaner (void (*)(Packet*));

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	// Tells whether standard performance measures are to be
	// calculated

	char            FlgSPF;                 // = ON

	int             Type,                   // = LT_broadcast;

	// Note: during the link initialization phase (while not all of its
	//       ports are defined), NAlive contains -1 to indicate the
	//       incomplete status of the link. During that phase, Alive
	//       stores the pointer to the link's distance matrix. This
	//       matrix is later turned into port distance vectors and
	//       deallocated.
	//       During dynamic initialization of the distance matrix,
	//       NAliveTransfers stores the last "i" position (row) and
	//       NAliveJams -- the last "j" (column) position.
	//       NArchived tells the number of ports already defined

			NPorts,                 // Number of ports

			NAlive,                 // Number of alive activities
			NAliveTransfers,        // Number of alive transfers
			NAliveJams,             // Number of alive jams

			NArchived,              // Number of archived activities
			NArchivedTransfers,     // Number of archived transfers
			NArchivedJams;          // Number of archived jams

	Long            NTJams,                 // Total number of jams
			NTAttempts;             // ... of transfer attempts

	BITCOUNT        NRBits,                 // Total bits received
			NTBits;                 // Total bits transmitted

	Long            NRPackets,              // Total packets received
			NTPackets,              // Total packets transmitted

			NRMessages,             // Total messages received
			NTMessages;             // Total messages transmitted
#if ZZ_FLK
	Long            NDPackets,              // Number of damaged packets
			NHDPackets;             // Number of h-damaged packets
	BITCOUNT        NDBits;                 // Number of damaged bits
#endif

	TIME            ArchiveTime;            // Archiving period

	ZZ_REQUEST      *RQueue [ZZ_N_LINK_EVENTS];     // Request queues

	void zz_start ();                       // Nonstandard constructor

	void setup (Long np, TIME at = TIME_0, int spf = ON);

	~Link () {
		excptn ("Link: [%1d] once created, a link cannot be destroyed",
			getId ());
	};

	virtual const char* getTName () { return ("Link"); };

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print request list
		printOut (0, hd, s);
	};

	inline  void    printPfm (const char *hd = NULL) {
		// Print performance measures
		printOut (1, hd);
	};

	inline  void    printAct (const char *hd = NULL, Long s = NONE) {
		// Print activities
		printOut (2, hd, s);
	};

	// Set one element in the distance matrix
	void setD     (int, int, double);
	void setDFrom (int, double);
	void setDTo   (int, double);

	// User-definable extensions

	virtual void    pfmMTR (Packet*) {};
	virtual void    pfmMRC (Packet*) {};
	virtual void    pfmPTR (Packet*) {};
	virtual void    pfmPRC (Packet*) {};
	virtual void    pfmPAB (Packet*) {};
#if ZZ_FLK
	virtual void    pfmPDM (Packet*) {};
	virtual void    packetDamage (Packet*);
#endif

	private:

	// Link performance measuring functions

	// Standard functions

	inline void     stdpfmMTR (Packet *p);
	inline void     stdpfmMRC (Packet *p);
	inline void     stdpfmPTR (Packet *p);
	inline void     stdpfmPRC (Packet *p);
	inline void     stdpfmPAB (Packet *p);
#if ZZ_FLK
	inline void     stdpfmPDM (Packet *p);
#endif

	// Called each time the last packet of a message is transmitted

	inline  void    spfmMTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMTR (p);
		pfmMTR (p);
	};

	// Called each time a packet is transmitted

	inline  void    spfmPTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPTR (p);
		pfmPTR (p);
	};

	// Called each time a packet is received

	inline  void    spfmPRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPRC (p);
		pfmPRC (p);
	};

	// Called each time a packet is aborted

	inline  void    spfmPAB (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPAB (p);
		pfmPAB (p);
	};

	// Called each time a message is received

	inline  void    spfmMRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMRC (p);
		pfmMRC (p);
	};
#if ZZ_FLK
	inline  void    spfmPDM (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPDM (p);
		pfmPDM (p);
	};
#endif
};

/* ---------------------------------------------------------- */
/* Link-independent  functions  for  defining  link  distance */
/* matrices. These functions operate directly on ports.       */
/* ---------------------------------------------------------- */

void setD     (Port*, Port*, double);
void setD     (Port&, Port&, double);
void setDFrom (Port*, double);
void setDFrom (Port&, double);
void setDTo   (Port*, double);
void setDTo   (Port&, double);

/* ------------------- */
/* Standard link types */
/* ------------------- */

inline  void    zz_bld_Link (char *nn = NULL) {
	register Link *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Link);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

/* -------------------------- */
/* A broadcast link (default) */
/* -------------------------- */
class   BLink : public Link {
	public:
	virtual const char *getTName () { return ("BLink"); };
};

inline  void    zz_bld_BLink (char *nn = NULL) {
	register BLink *p;
	zz_COBJ [++zz_clv] = (void*) (p = new BLink);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

/* --------------------- */
/* A unidirectional link */
/* --------------------- */
class   ULink : public Link {
	public:
	virtual const char *getTName () { return ("ULink"); };
	inline void setup (Long np, TIME at = TIME_0, int spf = ON) {
		Type = LT_unidirectional;
		Link::setup (np, at, spf);
	};
};

inline  void    zz_bld_ULink (char *nn = NULL) {
	register ULink *p;
	zz_COBJ [++zz_clv] = (void*) (p = new ULink);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

/* ------------------------------------ */
/* A unidirectional point-to-point link */
/* ------------------------------------ */
class   PLink : public Link {
	public:
	virtual const char *getTName () { return ("PLink"); };
	inline void setup (Long np, TIME at = TIME_0, int spf = ON) {
		Type = LT_pointtopoint;
		Link::setup (np, at, spf);
	};
};

inline  void    zz_bld_PLink (char *nn = NULL) {
	register PLink *p;
	zz_COBJ [++zz_clv] = (void*) (p = new PLink);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

/* ---------------------------------------------- */
/* A broadcast link in which collisions propagate */
/* ---------------------------------------------- */
class   CLink : public Link {
	public:
	virtual const char *getTName () { return ("CLink"); };
	inline void setup (Long np, TIME at = TIME_0, int spf = ON) {
		Type = LT_cpropagate;
		Link::setup (np, at, spf);
	};
};

inline  void    zz_bld_CLink (char *nn = NULL) {
	register CLink *p;
	zz_COBJ [++zz_clv] = (void*) (p = new CLink);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

#endif	/* NOL */

#if	ZZ_NOR
/* ================ */
/* RF Channel stuff */
/* ================ */

#include "rftagtype.h"

typedef	struct {
/* ============================ */
/* Interference histogram entry */
/* ============================ */
	TIME	Interval;
	double	Value;
} IHEntry;

typedef	struct {
/* =========================== */
/* Received signal description */
/* =========================== */
	double		Level;
	RF_TAG_TYPE	Tag;
} SLEntry;

class	IHist {

/* ====================== */
/* Interference histogram */
/* ====================== */

	friend	class	ZZ_RSCHED;
	friend	class	ZZ_RF_ACTIVITY;
	friend	class	Transceiver;
	friend	class	RFChannel;

	private:

	int	csize;
	double	clevel;
	TIME	last;

	public:

	int 	NEntries;
	IHEntry *History;

	private:

	inline void init () {

		NEntries = 0;
		last = Time;
		clevel = 0.0;
	};

	inline void update () {

		if (Time <= last)
			return;

		if (NEntries && clevel == History [NEntries-1] . Value) {
			// Extend last entry
			History [NEntries-1] . Interval += (Time - last);
			last = Time;
			return;
		}

		if (NEntries == csize) {
			// Grow the array
			IHEntry *tmp;
			tmp = new IHEntry [csize << 1];
			while (csize--)
				tmp [csize] = History [csize];
			delete [] History;
			History = tmp;
			csize = NEntries + NEntries;
		}

		History [NEntries] . Interval = Time - last;
		History [NEntries] . Value = clevel;
		NEntries++;
		last = Time;
	};

	inline void update (double sig) {

		if (sig == clevel)
			return;
		update ();
		clevel = sig;
	};

	inline void start () { History = new IHEntry [csize = 8]; init (); };
	inline void stop () { delete [] History; };
	
	public:

	inline void entry (int i, TIME &iv, double &v) const {
		// Interval in ITUs
		assert (i < NEntries, "IHist->entry: illegal entry index %1d",
			i);
		iv = History [i] . Interval;
		v = History [i] . Value;
	};

	inline void entry (int i, double &iv, double &v) const {
		// Interval in ETUs
		assert (i < NEntries, "IHist->entry: illegal entry index %1d",
			i);
		iv = ituToEtu (History [i] . Interval);
		v = History [i] . Value;
	};

	inline void entry (int i, RATE r, Long &iv, double &v) const {

		// Interval in bits. Note: in case the number of bits is
		// fractional, the returned number is properly randomized.

		double rb, ft, fr;

		assert (i < NEntries, "IHist->entry: illegal entry index %1d",
			i);
		rb = (double)(History [i] . Interval)/(double)r;
		ft = trunc (rb);
		// The fraction
		fr = rb - ft;
		iv = (Long) ft;
		if (fr > 0.0 && rnd (SEED_toss) < fr)
			iv ++;
		v = History [i] . Value;
	};

	inline TIME duration () const {
		TIME res;
		int i;
		res = TIME_0;
		for (i = 0; i < NEntries; i++)
			res += History [i] . Interval;
		return res;
	};

	inline Long bits (RATE r) const {

		// Duration in bits (randomized if fractional)

		double rb, ft, fr;
		Long res;

		rb = (double) duration () / (double) r;
		ft = trunc (rb);
		// The fraction
		fr = rb - ft;
		res = (Long) ft;
		if (fr > 0.0 && rnd (SEED_toss) < fr)
			res ++;
		return res;
	};

	inline double avg () const {
		// Total average
		int i;
		double s, t, u, d;

		s = t = 0.0;
		for (i = 0; i < NEntries; i++) {
			d = (double) (History [i] . Interval);
			u = t + d;
			s = s * (t / u) + ((History [i] . Value * d) / u);
			t = u;
		}

		return s;
	};

	// Average from ts for du (duration), or until the end of activity
	double avg (double ts, double du = -1.0) const;

	inline double avg (RATE r, Long sb, Long nb = -1) const {
		// Starting from bits sb for nb bits. If sb is negative,
		// take nb bits from the end.
		double ts;

		if (sb < 0) {
			// nb bits from the end
			if (nb < 0)
				// Means 'all'
				return avg ();
			ts = (double) duration () - ((double) r * (double) nb);
			if (ts <= 0.0)
				return avg ();
			return avg (ts);
		}
			
		return avg ((double)r * (double)sb,
				(nb < 0) ? -1.0 : (double)r * (double)nb);
	};

	inline double avg (TIME ts, TIME du = TIME_inf) const {
		if (ts == TIME_inf) {
			if (du == TIME_inf)
				return avg ();
			return avg (-1.0, (double) du);
		}
		return avg ((double)ts, (du == TIME_inf) ? -1.0 : (double)du);
	};

	inline double max () const {
		// Total max
		int i;
		double s;

		s = 0.0;
		for (i = 0; i < NEntries; i++)
			if (History [i] . Value > s)
				s = History [i] . Value;

		return s;
	};

	// Maximum from ts for du (duration), or until the end of activity
	double max (double ts, double du = -1.0) const;
	
	inline double max (RATE r, Long sb, Long nb = -1) const {
		// Starting from bit sb for nb bits. If sb < 0, take nb
		// bits from the end.
		double ts;

		if (sb < 0) {
			// nb bits from the end
			if (nb < 0)
				// Means 'all'
				return max ();
			ts = (double) duration () - ((double) r * (double) nb);
			if (ts <= 0.0)
				return max ();
			return max (ts);
		}

		return max ((double)r * (double)sb,
			(nb < 0) ? -1.0 : (double)r * (double)nb);
	};

	inline double max (TIME ts, TIME du = TIME_inf) const {
		// Starting from bit sb for nb bits
		if (ts == TIME_inf) {
			if (du == TIME_inf)
				return max ();
			return max (-1.0, (double) du);
		}
		return max ((double)ts, (du == TIME_inf) ? -1.0 : (double)du);
	};

	inline double cur () const {
		return clevel;
	};
};

class	Transceiver;

typedef struct {

	DISTANCE	Distance;
	Transceiver	*Neighbor;

} ZZ_NEIGHBOR;

class ZZ_RSCHED {
/*
 * One item associated with a transmitted packet describing the schedule
 * of events at a one-hop neighbor
 */
	private:

	friend class	ZZ_RF_ACTIVITY;
	friend class	Transceiver;

	// These ones are for per-receiver linkage
	ZZ_RSCHED	*next, *prev;

	// This one is for Schedule linkage
	ZZ_RSCHED	*Next;
	
	public:

	Transceiver	*Destination;
	DISTANCE	Distance;

	// Time of next event
	TIME		Schedule,
	// Time of last (previous) event
			LastEvent;

	// Activity pointer
	ZZ_RF_ACTIVITY	*RFA;

	// Received signal strength along with the copy of the Transceiver
	// tag at the moment of transmission. They are often used as a pair.
	SLEntry		RSS;

	Boolean		Killed,
			Done;
	unsigned char	Stage;

	// Original transmitted power; needed in those rare cases when some
	// attribute of receiver (used by RFC_att) changes half way through
	// packet perception, and we have to re-assess the signal level using
	// its original transmitted power.
	double		OXPower;

	/*
	 * This one is for calculating interference statistics
	 */
	IHist		INT;

	// No constructor, don't we need an empty one?

	inline void initAct (), initSS ();

	inline Boolean in_packet (), within_packet ();

	inline int any_event ();
};

class	RFChannel : public AI {

	friend class Transceiver;
	friend class RosterService;
	friend class ZZ_RSCHED;
	friend class ZZ_SYSTEM;
	friend class Station;
	friend class zz_client;

	private:

	static	Long	sernum;

	// The set of all Transceivers belonging to the channel
	Transceiver	**Tcvs;

	void (*PCleaner) (Packet*);		// Packet cleaner funtion

	// Exposures:
	//	0 - wait requests (the usual); only Transceivers can have
	//	    wait requests
	//	1 - performance statistics (counters)
	//	2 - activities (packets in transit, one line per packet)
	//	3 - topology (paper only)
	// 
	void	exPrint0 (const char*, int);	// Requests
	void	exPrint1 (const char*);		// Performance statistics
	void	exPrint2 (const char*, int);	// Activities
	void	exPrint3 (const char*);		// Topology

	void    exDisplay0 (int);               // Requests
	void    exDisplay1 ();                  // Performance
	void    exDisplay2 (int);               // Activities
	void	exDisplay3 (int);		// Topology

	// Common code of SYSTEM's exPrint0 and our exPrint3
	void		exPrtTop (Boolean);
	// Shared by Station exposure
	static  void	exPrtRfa (int, const RFChannel *rfc = NULL);
	static  void	exDspRfa (int, const RFChannel *rfc = NULL);

	void	complete ();

	public:

	void zz_expose (int, const char *h = NULL, Long s = NONE);

	/*
	 * Here we have virtual functions for calculating interference
	 * and deciding on whether certain packet events are perceptible
	 * by the receiver.
	 */
	virtual double RFC_att (
			const SLEntry*,	// Source power + source Tag
			double, 	// Distance
			Transceiver* 	// Source
		);
		// This one determines how the perceived signal level depends
		// on the xpower, distance, source, destination. Note: although
		// in principle the Tag in the first argument can be recovered
		// from Source, it may change when RFC_att is invoked to
		// "reassess". Also, destination is available as TheTransceiver.

	virtual double RFC_add (int, int, const SLEntry**, const SLEntry*);
		// This one determines how the levels of multiple received
		// signals combine. It can be called to find the interference
		// affecting a selected signal or to calculate the global
		// signal level, e.g., busy/idle. The first argument is the
		// number of entries in the second array (listing the levels
		// of individual signals perceived at the transceiver). 
		// If the second argument is >= 0, it gives the index of
		// the activity to be ignored (e.g., when we are calculating
		// the interference for one selected activity.
		// The last argument contains the receiver's Tag and the
		// power level of own activity, if present (i.e., XPower).
		// If the transceiver is not transmitting, this power level
		// is 0.0.

	virtual Boolean RFC_act (
			double,		// Combined signal level
			const SLEntry*	// Receiver sensitivity + Tag
		);
		// This one determines whether the receiver senses an activity
		// based on the combined signal level and receiver sensitivity.
		// Used to assess ACTIVITY/SILENCE.

	virtual Boolean RFC_bot (
			RATE,		// Transmission rate
			const SLEntry*,	// Received signal level
			const SLEntry*,	// Receiver sensitivity (+Tag)
			const IHist*	// interference histogram
		);
		// This one determines whether the beginning of packet is
		// recognized by the interface.

	virtual Boolean RFC_eot (
			RATE,		// Transmission rate
			const SLEntry*,	// Received signal level
			const SLEntry*,	// Receiver sensitivity
			const IHist*	// interference histogram
		);
		// This one determines whether the end of packet is
		// recognized by the interface.

	virtual	Long RFC_erb (
			RATE,		// Transmission rate
			const SLEntry*,	// Received signal level
			const SLEntry*,	// receiver sensitivity
			double,		// interference level
			Long		// signal length (number of bits)
		);
		// This one returns the number of error bits within a signal
		// of a given length, under the given interference conditions.
		// The values returned by this function should be randomized
		// according to the BER distribution.

	virtual	Long RFC_erd (
			RATE,		// Transmission rate
			const SLEntry*,	// received signal level
			const SLEntry*,	// receiver sensitivity
			double,		// interference level
			Long		// run length (number of bits)
		);
		// This one returns the randomized length of a run of correct
		// bits until the first error bit (if the last argument is 1).
		// If the last argument is greater than 1, then it represents
		// the number of consecutive error bits. In that case, the
		// method should return the randomized length of a run of
		// bits until the first run of that many incorrect bits.

	virtual double RFC_cut (
			double,		// Transmitter power
			double		// Receiver sensitivity
		);
		// This one determines the cut-off distance, i.e., the "range"
		// based on the transmitter power and receiver sensitivity.
		// Nodes separated by more than this range are not considered
		// neighbors, i.e., they cannot hear each other.

	virtual TIME RFC_xmt (
			RATE,
			Long	// Total packet length (TLength)
		);
		// This one isn't really an assessment method. It calculates
		// the transmission time of a packet with a given total
		// length excluding the preamble time. The default variant
		// returns rate * tlength.

	char		FlgSPF;

	int		NTransceivers;

	Long		NTAttempts,
			NRPackets,
			NTPackets,
			NRMessages,
			NTMessages;

	BITCOUNT	NRBits,
			NTBits;

	void zz_start ();

	void setup (Long np, int spf = ON);

	~RFChannel () {
		excptn ("RFChannel: [%1d] once created, an RFChannel cannot be "
			"destroyed", getId ());
	};

	virtual const char* getTName () { return ("RFC"); };

	inline void printRqs (const char *hd = NULL, Long s = NONE) {
		printOut (0, hd, s);
	};

	inline void printPfm (const char *hd = NULL) {
		printOut (1, hd);
	};

	inline void printRAct (const char *hd = NULL, Long s = NONE) {
		printOut (2, hd, s);
	};

	inline void printTop (const char *hd = NULL) {
		printOut (3, hd);
	};

	// User-definable extensions

	virtual void    pfmMTR (Packet*) {};
	virtual void    pfmMRC (Packet*) {};
	virtual void    pfmPTR (Packet*) {};
	virtual void    pfmPRC (Packet*) {};
	virtual void    pfmPAB (Packet*) {};

	void 	connect (Transceiver*);
	void	setTRate (RATE);
	void	setPreamble (Long);
	void	setTag (IPointer);
	void	setErrorRun (Long);
	void	setXPower (double);
	void	setRPower (double);
	void	setMinDistance (double);
	void	setAevMode (Boolean);

	void	setPacketCleaner (void (*)(Packet*));

	// If the histogram contains a bit error
	Boolean error (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Boolean error (RATE, const SLEntry*, const SLEntry*, const IHist*,
		Long, Long nb = -1);
	// How many bit errors in the histogram
	Long errors (RATE, const SLEntry*, const SLEntry*, const IHist*);
	Long errors (RATE, const SLEntry*, const SLEntry*, const IHist*,
		Long, Long nb = -1);
#if ZZ_R3D
	double	getRange (double&, double&, double&, double&, double&, double&);
#else
	double	getRange (double&, double&, double&, double&);
#endif
	private:

	// A wrapper for RFC_cut accepting two Transceivers
	inline double rfc_cut (const Transceiver*, const Transceiver*);

	// Extend the transceiver's neighborhood. Called, e.g., when the
	// transceiver increases its transmit power.
	void	nei_xtd (Transceiver*);

	// Trim the transceiver's neighborhood. Called, e.g., when the
	// transceiver decreases its transmit power.
	void	nei_trm (Transceiver*);

	// Completely (re)build the neighborhood, e.g., after move.
	void	nei_bld (Transceiver*);

	// Update the neighborhoods of all other nodes by considering addition
	// of this Transceiver as a new neighbor.
	void	nei_add (Transceiver*);

	// Update the neighborhoods of all other nodes by considering removal
	// of this Transceiver from the list of neighbors.
	void	nei_del (Transceiver*);

	// Update the neighborhoods of all other nodes after a move of this
	// one Transceiver
	void	nei_cor (Transceiver*);

	// Performance measuring functions

	inline void     stdpfmMTR (Packet *p);
	inline void     stdpfmMRC (Packet *p);
	inline void     stdpfmPTR (Packet *p);
	inline void     stdpfmPRC (Packet *p);
	inline void     stdpfmPAB (Packet *p);

	// Called each time the last packet of a message is transmitted

	inline  void    spfmMTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMTR (p);
		pfmMTR (p);
	};

	// Called each time a packet is transmitted

	inline  void    spfmPTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPTR (p);
		pfmPTR (p);
	};

	// Called each time a packet is received

	inline  void    spfmPRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPRC (p);
		pfmPRC (p);
	};

	// Called each time a packet is aborted

	inline  void    spfmPAB (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPAB (p);
		pfmPAB (p);
	};

	// Called each time a message is received

	inline  void    spfmMRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMRC (p);
		pfmMRC (p);
	};
};

inline  void    zz_bld_RFChannel (char *nn = NULL) {
	register RFChannel *p;
	zz_COBJ [++zz_clv] = (void*) (p = new RFChannel);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

extern  istream       *zz_ifpp;                 // Input stream
extern  ostream       *zz_ofpp;                 // Output file

class	Transceiver : public AI {

	friend	class	RFChannel;
	friend	class	ZZ_RSCHED;
	friend	class	ZZ_RF_ACTIVITY;
	friend	class	ZZ_SYSTEM;
	friend	class	zz_client;
	friend	class	Traffic;
	friend	class	Station;
	friend	Transceiver *idToTransceiver (int);
	friend  void    zz_adjust_ownership ();

	private:

	Transceiver	*nextp;		// Next on the station's list

	RFChannel	*RFC;

	Station		*Owner;

	ZZ_REQUEST	*RQueue [ZZ_N_TRANSCEIVER_EVENTS];

	/*
	 * Coordinates within the RFChannel's rectangle
	 */
	DISTANCE	X, Y;
#if	ZZ_R3D
	DISTANCE	Z;
#endif
	Long		Preamble;	// In bits
	RATE		TRate;		// In ITUs

	/*
	 * This is the receiver sensitivity combined with the transceiver's
	 * Tag. We keep them together because they are often used as a pair.
	 */
	SLEntry		SenTag;

	/*
	 * Transmitter power
	 */
	double		XPower;

	/*
	 * Threshold for signal-level events
	 */
	double		SigThreshold;

	/*
	 * Traced activity
	 */
	ZZ_RSCHED	*TracedActivity;

	/*
	 * Minimum distance to a neighbor. We can never move closer than
	 * this, even if we appear to be at exactly the same location.
	 */
	DISTANCE	MinDistance;

	/*
	 * This is the list of neighbor Transceivers sorted by the
	 * increasing distance from us.
	 */
	ZZ_NEIGHBOR	*Neighbors;
	int		NNeighbors;

	/*
	 * Activities arriving at the Receiver
	 */
	ZZ_RSCHED	*Activities;
	
	/*
	 * This is the number of perceived activities (Done = NO). The actual
	 * number of activities queued in Activities may be larger.
	 */
	int		NActivities;

	/*
	 * Error run: the number of consecutive error bits for triggering the
	 * BERROR event.
	 */
	Long		ErrorRun;

	/*
	 * Current activity
	 */
	ZZ_RF_ACTIVITY	*Activity;

	/*
	 * RCV on/off flag
	 */
	Boolean		RxOn,
			Mark,		// Used for searching
			AevMode;	// ANYEVENT occurs within this ITU

	// Here we have room for one more flag

	// Exposures:
	//	0 - wait requests (the usual)
	//	1 - activities (+ signal levels)
	//	2 - event schedule
	//	3 - neighborhood

	void    exPrint0 (const char*);       // Request list
	void    exPrint1 (const char*);       // Activities
	void    exPrint2 (const char*);       // Neighborhood

	void    exDisplay0 ();          // Request list
	void    exDisplay1 ();          // Activities
	void    exDisplay2 ();          // Neighborhood

	void	dspEvnt (int*);		// Event list for display/print

	inline ZZ_RF_ACTIVITY *gen_rfa (Packet*);

	void term_xfer (int);

	void updateIF ();

	Packet *findRPacket ();

	void 	handle_ifv ();		// Handle interference threshold events

	void 	reschedule_bop (),
		reschedule_eop (),
		reschedule_act (),
		reschedule_sil (),
		reschedule_aev (int),
		reschedule_bot (ZZ_RSCHED*),
		reschedule_eot (ZZ_RSCHED*),
		reschedule_thh (),
		reschedule_thl ();

	inline double qdst (const Transceiver *t) {
		// Note: this returns the distance converted to Du
#if	ZZ_R3D
		register double A, B, C;
		A = ((double)X - (double)(t->X)) / Du;
		B = ((double)Y - (double)(t->Y)) / Du;
		C = ((double)Z - (double)(t->Z)) / Du;
		return sqrt (A*A + B*B + C*C);
#else
		register double A, B;
		A = ((double)X - (double)(t->X)) / Du;
		B = ((double)Y - (double)(t->Y)) / Du;
		return sqrt (A*A + B*B);
#endif
	};

	public:

	void connect (RFChannel *rfc) {
		rfc->connect (this);
	};

	void connect (Long);

	INLINE TIME bitsToTime (Long n = 1);

	inline double distTo (Transceiver *tcv) {
		Assert (RFC != NULL && RFC == tcv->RFC,
			"Transceiver->distTo: %s -- %s, not connected or on "
			"different channels", getSName (), tcv->getSName ());
		return qdst (tcv);
	};

	void rcvOn (), rcvOff ();

	void startTransfer (Packet*);

	inline int stop () {
		term_xfer (EOT);
		return TRANSFER;
	};

	inline int abort () {
		term_xfer (SILENCE);
		return TRANSFER;
	};

	Boolean transmitting () { return Activity != NULL; };

	int activities (int&);

	inline int activities () {
		int junk;
		return activities (junk);
	};

	Packet *anotherPacket ();
	int anotherActivity ();

	int events (int);
	Packet *thisPacket ();

	double sigLevel ();
	double sigLevel (const Packet*, int which = SIGL_OWN);
	double sigLevel (int which);
	Boolean dead (const Packet *p = NULL);
	IHist *iHist (const Packet *p = NULL);

	inline Boolean error (const SLEntry *sl, IHist *h) {
		return RFC->error (TRate, sl, &SenTag, h);
	};

	inline Long errors (const SLEntry *sl, IHist *h) {
		return RFC->errors (TRate, sl, &SenTag, h);
	};

	Boolean error (Packet *p = NULL);
	Long errors (Packet *p = NULL);

	inline Boolean busy () {
		return RxOn ? RFC->RFC_act (sigLevel (), &SenTag) : 0;
	};

	inline Boolean idle () {
		return !busy ();
	};

#if  ZZ_TAG
	inline void transmit (Packet*, int, LONG tag = 0L);
	inline void transmit (Packet&, int, LONG tag = 0L);
	void wait (int, int, LONG tag = 0L);
#else
	inline void transmit (Packet*, int);
	inline void transmit (Packet&, int);
	void wait (int, int);
#endif
	inline TIME getXTime (Long);

	void	zz_start ();
	~Transceiver ();

	const char	*getTName () { return ("Tcv"); };
	int	getSID (), getYID ();

	RATE	setTRate (RATE);
	Long	setPreamble (Long);
	double	setXPower (double);
	double	setRPower (double);
	IPointer setTag (IPointer);
	Long	setErrorRun (Long);
	double 	setSigThreshold (double);
	double 	setMinDistance (double);
	Boolean	setAevMode (Boolean);
	Boolean follow (Packet *p = NULL);
	inline Boolean isFollowed (Packet*);

	void reassess ();

#if	ZZ_R3D

	Transceiver (RATE r = RATE_0, int pre = 0,
		double xp = 0.0, double rp = 0.0, double x = Distance_inf,
			double y = Distance_inf, double z = Distance_inf,
				char *nn = NULL);

	void	setup (RATE r = RATE_0, int pre = 0,
		double xp = 0.0, double rp = 0.0, double x = Distance_0,
			double y = Distance_0, double z = Distance_0);
	void 	setLocation (double, double, double);
	inline  void	getLocation (double &x, double &y, double &z) {
		x = ituToDu (X);
		y = ituToDu (Y);
		z = ituToDu (Z);
	};

#else	/* 2-D */

	Transceiver (RATE r = RATE_0, int pre = 0,
		double xp = 0.0, double rp = 0.0, double x = Distance_inf,
			double y = Distance_inf, char *nn = NULL);

	void	setup (RATE r = RATE_0, int pre = 0,
		double xp = 0.0, double rp = 0.0, double x = Distance_0,
			double y = Distance_0);
	void 	setLocation (double, double);
	inline  void	getLocation (double &x, double &y) {
		x = ituToDu (X);
		y = ituToDu (Y);
	};

#endif	/* ZZ_R3D */

	inline	RATE	getTRate () { return TRate; };
	inline	Long	getPreamble () { return Preamble; };
	inline  TIME	getPreambleTime () {
		return (TIME) TRate * (LONG) Preamble;
	};
	inline	double	getXPower () { return XPower; };
	inline	double	getRPower () { return SenTag.Level; };
	inline	IPointer getTag () { return SenTag.Tag; };
	inline	Long	getErrorRun () { return ErrorRun; };
	inline  double  getSigThreshold () { return SigThreshold; };
	inline	double  getMinDistance () { return ituToDu (MinDistance); };
	inline	Boolean	getAevMode () { return AevMode; };

	void zz_expose (int, const char *h = NULL, Long s = NONE);
	
	// Exposure
	inline  void    printRqs (const char *hd = NULL) {
		// Print request list
		printOut (0, hd);
	};

	inline  void    printRAct (const char *hd = NULL) {
		// Print activities
		printOut (1, hd);
	};

	inline void	printNei (const char *hd = NULL) {
		// Print nodes in the neighborhood
		printOut (2, hd);
	};

#ifdef	ZZ_RF_DEBUG
	void dump (const char*);
#endif 

};

inline  void    zz_bld_Transceiver (char *nn = NULL) {
	register Transceiver *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Transceiver);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

#endif	/* NOR */


#if	ZZ_NOC

extern	unsigned int zz_pvoffset;

/* ----------------------------------- */
/* Type of test function for getPacket */
/* ----------------------------------- */
typedef         int(*ZZ_MTTYPE)(Message*);

/* --------- */
/* Client AI */
/* --------- */

class   zz_client : public AI {

	private:

	// Exposure functions

	void exPrint0 (const char*, int);     // Request list
	void exPrint1 (const char*);          // Performance measures
	void exPrint2 (const char*, int);     // Contents (message queues)
	void exPrint3 (const char*);          // Definitions (traffic)

	void exDisplay0 (int);          // Request list
	void exDisplay1 ();             // Performance measures
	void exDisplay2 (int);          // Contents (message queues)
	void exDisplay3 ();             // Sent-received statistics

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	zz_client ();           // Constructor

	const char    *getTName () { return ("Client"); };

#if  ZZ_TAG
	void wait (int, int, LONG tag = 0L);
#else
	void wait (int, int);
#endif

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print request list
		printOut (0, hd, s);
	};

	inline  void    printPfm (const char *hd = NULL) {
		// Print performance measures
		printOut (1, hd);
	};

	inline  void    printCnt (const char *hd = NULL, Long s = NONE) {
		// Print queues
		printOut (2, hd, s);
	};

	inline  void    printDef (const char *hd = NULL) {
		// Print definition
		printOut (3, hd);
	};

	// To be called upon reception of a packet

#if	ZZ_NOL
	void receive (Packet *p, Link *l = (Link*) NULL);

	inline void receive (Packet &p, Link &l) {
		receive (&p, &l);
	};

	inline void receive (Packet *p, Port *pp) {
		receive (p, pp->Lnk);
	};

	inline void receive (Packet &p, Port &pp) {
		receive (&p, (&pp)->Lnk);
	};
#else
	void receive (Packet *p);
	inline void receive (Packet &p) {
		receive (&p);
	};
#endif

#if	ZZ_NOR

	void receive (Packet *p, RFChannel *l);

	inline void receive (Packet &p, RFChannel &l) {
		receive (&p, &l);
	};

	inline void receive (Packet *p, Transceiver *pp) {
		receive (p, pp->RFC);
	};

	inline void receive (Packet &p, Transceiver &pp) {
		receive (&p, (&pp)->RFC);
	};
#endif

	// Client (global) versions of getPacket

	int getPacket (Packet *p, Long mi = 0, Long mx = 0,
		    Long fr = 0);

	inline int getPacket (Packet &p, Long mi = 0, Long mx = 0,
		    Long fr = 0);

	int getPacket (Packet *p, ZZ_MTTYPE, Long mi = 0, Long mx = 0,
		    Long fr = 0);

	inline int getPacket (Packet &p, ZZ_MTTYPE, Long mi = 0, Long mx = 0,
		    Long fr = 0);

	// Checks if there is a packet of any type to be acquired from the
	// Client

	int isPacket ();
	int isPacket (ZZ_MTTYPE);

	// Releases a packet that has been successfully transmitted

	void release (Packet *p);

	inline void release (Packet &p);

	void resetSPF ();

	void suspend ();
	void resume ();
	int isSuspended ();
	inline int isResumed () { return (! isSuspended () ); };

	double throughput ();
};

extern  zz_client  zz_AI_client;         // Announcement

class   SGroup {                // A station group

	friend  class   zz_client;
	friend  class   Traffic;
	friend  class   CGroup;
	friend  class   Port;
	friend  void    zz_sort_group (SGroup*, float* = NULL);
	friend  void    zz_start_client ();

	private:

	int     NStations;      // The number of stations in the group
	short   *SIdents;       // The array of station identifiers
	char    sorted;         // YES, if the group is sorted

	public:

	void    zz_start () { sorted = NO; };
	void    setup (int, int*);      // List of stations given explicitely

	void    setup () {              // All stations
	
		NStations = (int) ::NStations;
		SIdents = NULL;
	};

	void    setup (int, int, ...);  // Explicit list of stations
	void    setup (int, Station*, ...);

	~SGroup () {

		excptn ("SGroup: once created, cannot be destroyed");
	};

	// Checks if a givent station occurs within the group
	int     occurs (Long);
	inline  int occurs (Station *s) {
		return (occurs (s->Id));
	};
};

inline  void    zz_bld_SGroup () {
	register SGroup *p;
	zz_COBJ [++zz_clv] = (void*) (p = new SGroup);
	p->zz_start ();
}

class   CGroup {                // A communication group

	friend  class   zz_client;
	friend  class   Traffic;
	friend  void    zz_start_client ();

	private:

	SGroup  *Senders,       // Legitimate transmitters
		*Receivers;     // Legitimate receivers

	float   *SWeights,      // Sender weights
		*RWeights,      // Receiver weights

		CGWeight;       // Group weight (to speed up things a little)

	public:

	// The most general case
	void    setup (SGroup*, float*, SGroup*, float*);

	// No receiver weights
	void    setup (SGroup *snd, float *snw, SGroup *rcv) {
		setup (snd, snw, rcv, (float*) NULL);
	};

	// No sender weights (receiver weights given)
	void    setup (SGroup *snd, SGroup *rcv, float *rcw) {
		setup (snd, (float*) NULL, rcv, rcw);
	};

	// No weights at all
	void    setup (SGroup *snd, SGroup *rcv) {
		setup (snd, (float*) NULL, rcv, (float*) NULL);
	};

	// Broadcast traffic (the last parameter can only be GT_broadcast)
	void    setup (SGroup*, float*, SGroup*, int);

	// Broadcast without sender weights
	void    setup (SGroup *snd, SGroup *rcv, int b) {
		setup (snd, (float*) NULL, rcv, b);
	};

	void    zz_start () { };

	~CGroup () {

		excptn ("CGroup: once created, cannot be destroyed");
	};
};

inline  void    zz_bld_CGroup () {
	register CGroup *p;
	zz_COBJ [++zz_clv] = (void*) (p = new CGroup);
	p->zz_start ();
}

class   Message {                       // Message skeleton

	friend  class   Traffic;
	friend  class   zz_client;
	friend  class   Packet;

	public:

	Message         *next, *prev;   // For putting messages into queue

	IPointer        Receiver;       // The recipient
	TIME            QTime;          // The time the message was queued
	Long            Length;         // The number of (information) bits
	int             TP;             // Traffic pattern identifier

	virtual void setup () {
		// Void -- to be provided by the user (in the unlikely case
		// he wants to generate messages by hand)
	};

	virtual int  frameSize () {
		// Returns the size of the subtype
		return (sizeof (Message));
	};

	// The function below is defined for compatibility. Nobody sane
	// will 'create' messages by hand
	void    zz_start () { TP = NONE; };     // See Packet

	inline  void operator= (Message &m) {
		// Intentionally void -- just in case
		bcopy ((BCPCASTS)(&m), (BCPCASTD)(this), m.frameSize ());
	};

	// Acquires a packet from this specific message

	int getPacket (Packet *p, Long mi = 0, Long mx = 0, Long fr = 0);

	inline int      getPacket (Packet &p, Long mi = 0, Long mx = 0,
		Long fr = 0) {

		return (getPacket (&p, mi, mx, fr));
	};

	inline  int     isNonstandard () { return (!isTrafficId (TP)); };
	inline  int     isStandard () { return (isTrafficId (TP)); };

	// Destructor for intercepting deallocation
	~Message ();
};

inline  void    zz_bld_Message () {
	register Message *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Message);
	p->zz_start ();
}

class   Packet  {                       // Packet skeleton

	public:

	TIME            QTime;          // Message queue time
	TIME            TTime;          // The time the packet appeared on
					// top of the message queue
	Long            Sender;         // The transmitter
	int             TP;             // Qualification

	FLAGS           Flags;          // E.g. the last packet of message

#if  ZZ_DBG
	Long            Signature;      // Packet signature for tracing
#endif

	Long            ILength;        // The length of the information part
	Long            TLength;        // Total length (incl. hdr+trailer)

	// Note: the following is an address-sized object. It has been
	// moved in front of zz_setptr to force address alignment (on
	// machines that require it)
	IPointer	Receiver;       // The receiver

	inline void zz_setptr (char *p) {
		*((char**)(((char*)this) + zz_pvoffset)) = p;
	}

	char *zz_getptr ();

	// For copying non-standard message information (from Message subtypes)
	// to user-defined Packet subtypes

	virtual void setup (Message *m); // From message
	virtual void setup () { };       // Raw

	virtual int  frameSize () {
		// Returns the size of the subtype
		return (sizeof (Packet));
	};

	inline  void setup (Message &m) { setup (&m); };

	// Checks if the packet is addressed to this station
	int     isMy (Station *s = NULL);

	// The packet version of release
	inline  void    release () {
		Client->release (this);
	};

	// Converts standard flags to printable format
	char    *zz_pflags ();

	// The following function is defined for 'create'. A packet created
	// this way is assumed to be nonstandard and its TP attribute is
	// set to NONE
	void    zz_start () { TP = NONE; Flags = 0; zz_mkpclean (); };

	inline  void operator= (Packet &p) {
		// Intentionally void -- just in case
		bcopy ((BCPCASTS)(&p), (BCPCASTD)(this), p.frameSize ());
	};

	void    fill (Station*, Station*, Long, Long il = NONE, Long tp = NONE);
	void    fill (Long, IPointer, Long, Long il = NONE, Long tp = NONE);

	Packet *clone () {
		unsigned int s = frameSize ();
		Packet *pp = (Packet*) (new char [s]);
		bcopy ((BCPCASTS) this, (BCPCASTD) pp, s);
		return pp;
	};

	// Constructor for creating statically-declared packet buffers
	Packet ();

	// Destructor for intercepting deallocations of dynamic packets
	~Packet ();

	// Methods to check the packet status

	inline  int     isFull () { return (flagSet (Flags, PF_full)); };
	inline  int     isEmpty () { return (flagCleared (Flags, PF_full)); };
	inline  int     isNonstandard () { return (!isTrafficId (TP)); };
	inline  int     isStandard () { return (isTrafficId (TP)); };
	inline  int     isLast () { return (flagSet (Flags, PF_last)); };
	inline  int     isNonlast () { return (flagCleared (Flags, PF_last)); };
	inline  int     isBroadcast () {
		return (flagSet (Flags, PF_broadcast));
	};
	inline  int     isNonbroadcast () {
		return (flagCleared (Flags, PF_broadcast));
	};
	inline  int  isDamaged () { return (flagSet (Flags, PF_damaged)); };
	inline  int  isValid () { return (flagCleared (Flags, PF_damaged)); };
	inline  int  isHDamaged () { return (flagSet (Flags, PF_hdamaged)); };
	inline  int  isHValid () { return (flagCleared (Flags, PF_hdamaged)); };
};

inline  void    zz_bld_Packet () {
	register Packet *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Packet);
	p->zz_start ();
}

class   Traffic : public AI {

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	friend  class   zz_client;
	friend  class   zz_client_service;
	friend  class   ZZ_LINK_ACTIVITY;
#if ZZ_NOL
	friend  ZZ_LINK_ACTIVITY *zz_gen_activity (int, Port*, int, Packet *p);
#endif
	friend  class   Message;
	friend  class   Packet;
	friend  void    zz_adjust_ownership ();
	friend  void    zz_start_client ();
	friend  void    zz_sendObjectMenu (ZZ_Object*);
#ifdef  CDEBUG
	friend  void    zz_dumpTree (ZZ_Object*);
#endif
	friend  ZZ_Object *zz_findAdvance (ZZ_Object*);
	friend  ZZ_Object *zz_findWAdvance (ZZ_Object*);

	// Flags

	char    DstMIT,         // Message interarrival time distribution
		DstMLE,         // Message length distribution
		DstBIT,         // Burst interarrival time distribution
		DstBSI,         // Burst size distribution
		FlgSUS,		// Suspended (standard/nonstandard)
		FlgACT,		// Active == FlgSCL && !FlgSUS
		FlgSCL,         // Standard Client ON/OFF
		FlgSPF;         // Standard performance measures ON/OFF

	// Note: A Dst (distribution flag) can be set to:
	//
	//              UNDEFINED       - (inconsitencies detected in run time)
	//              EXPONENTIAL     - Poisson distribution
	//              UNIFORM         - easy to guess
	//              FIXED           - 
	//
	// More distribution types will be added in the future

	double  ParMnMIT,       // Min (or Mean) message interarrival time
		ParMxMIT,       // Max message interarrival time
		ParMnMLE,       // Min (or Mean) message length
		ParMxMLE,       // Max message length
		ParMnBIT,       // Min (or Mean) burst interarrival time
		ParMxBIT,       // Max burst interarrival time
		ParMnBSI,       // Min (or Mean) burst size
		ParMxBSI;       // Max burst size

	int     NCGroups;       // The number of communication groups
	CGroup  **CGCluster;    // The cluster of communication groups
	double  TCGWeight;      // Total sender weight

	Long    LastSender;            // To avoid generating receiver the
	CGroup  *LastCGroup;           // same as sender


	// Note: the LANSF concept of a "flood" group and cluster has been
	//       abandoned. Nobody could find a reasonable application for
	//       it. Anyway, now it would not be difficult to define it in
	//       a non-standard way.

	// Slots for standard RVariables used to calculate standard
	// performance measures

	RVariable      *RVAMD,  // Absolute message delay
		       *RVAPD,  // Absolute packet delay
		       *RVWMD,  // Weighted message delay
		       *RVMAT,  // Message access time
		       *RVPAT,  // Packet access time
		       *RVMLS;  // Message length statistics

	// The following functions are declared as virtual. This way the
	// user is able to redefine them in his/her own favorite way

	virtual TIME     genMIT ();      // Generate message interarrival time
	virtual Long     genMLE ();      // Generate message length
	virtual TIME     genBIT ();      // Generate burst interarrival time
	virtual Long     genBSI ();      // Generate burst size
	virtual Long     genSND ();      // Generate sender
	virtual IPointer genRCV ();      // Generate receiver

	// Functions for dynamic initialization of communication groups

	void    addSender (Station *s = ALL, double w = 1.0, int gr = 0);
	void    addSender (Long s, double w = 1.0, int gr = 0);

	void    addReceiver (Station *s = ALL, double w = 1.0, int gr = 0);
	void    addReceiver (Long s, double w = 1.0, int gr = 0);

	// Generate a communication group including a given sender. The
	// group is selected according to its probability of being used
	// to generate the specified sender.

	virtual CGroup *genCGR (Long);

	inline  CGroup *genCGR (Station *s) {
		return (genCGR (s->Id));
	};

	// Generate message and queue it at the sender

	virtual Message *genMSG (Long,      // The sender identifier
				IPointer,   // The receiver identifier
				Long);      // The message length

	inline  Message *genMSG (Long s, SGroup *r, Long l) {
		return (genMSG (s, (IPointer)r, l));
	};

	inline  Message *genMSG (Station *s, Station *r, Long l) {
		return (genMSG (s->Id, (IPointer) (r->Id), l));
	};

	inline  Message *genMSG (Station *s, SGroup *r, Long l) {
		return (genMSG (s->Id, (IPointer)r, l));
	};

#if  ZZ_TAG
	void    wait (int, int, LONG tag = 0L);
#else
	void    wait (int, int);
#endif
	double	throughput ();

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Print request list
		printOut (0, hd, s);
	};

	inline  void    printPfm (const char *hd = NULL) {
		// Print performance measures
		printOut (1, hd);
	};

	inline  void    printCnt (const char *hd = NULL, Long s = NONE) {
		// Print queues
		printOut (2, hd, s);
	};

	inline  void    printDef (const char *hd = NULL) {
		// Print definition
		printOut (3, hd);
	};

	virtual const char   *getTName () {
		return ("Traffic");
	};

	private:

	// A preprocessor for addSender and addReceiver

	void    zz_addinit (int);
#if     ZZ_QSL
	Long    QSLimit;                        // Pending message count limit
#endif
	ZZ_Object  *ChList;                     // For RVariables owned

	// Exposure functions

	void    exPrint0 (const char*, int);    // Request list
	void    exPrint1 (const char*);         // Performance
	void    exPrint2 (const char*, int);    // Contents (message queues)
	void    exPrint3 (const char*);         // Traffic pattern definition

	void    exDisplay0 (int);               // Request list
	void    exDisplay1 ();                  // Performance
	void    exDisplay2 (int);               // Message queues
	void    exDisplay3 ();                  // Sent-received statistics

	// This junk should be invisible

	virtual Message *zz_mkm () {            // Create a (subclass) message
	    return (new Message);
	};

	virtual Packet  *zz_mkp ();             // Create a (subclass) packet
						// used for tricks only

	// Pointer to the virtual function table of the associated packet type.
	// We play some tricks here. This pointer is used to initialize 
	// packets that are created dynamically in a non-standard way.
	char    *ptviptr;

	static Long sernum;             // Serial number

	// Called each time a message is queued at a station

	inline  void    spfmMQU (Message *m) {
		if (FlgSPF == ON)
			stdpfmMQU (m);  // The standard part
		pfmMQU (m);             // The user-definable part
	};

	// Called each time a full message is transmitted

	inline  void    spfmMTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMTR (p);
		pfmMTR (p);
	};

	// Called each time a full message is received

	inline  void    spfmMRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmMRC (p);
		pfmMRC (p);
	};

	// Called each time a packet is transmitted

	inline  void    spfmPTR (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPTR (p);
		pfmPTR (p);
	};

	// Called each time a packet is received

	inline  void    spfmPRC (Packet *p) {
		if (FlgSPF == ON)
			stdpfmPRC (p);
		pfmPRC (p);
	};

	// Standard performance measuring functions

	void    stdpfmMQU (Message*);
	void    stdpfmMTR (Packet*);
	void    stdpfmMRC (Packet*);
	void    stdpfmPTR (Packet*);
	void    stdpfmPRC (Packet*);

	// To convert absolute weights into accumulated weights
	void    preprocess_weights ();

	public:

	// User-definable extensions

	virtual void    pfmMQU (Message*) {};
	virtual void    pfmMDE (Message*) {};
	virtual void    pfmMTR (Packet*) {};
	virtual void    pfmMRC (Packet*) {};
	virtual void    pfmPTR (Packet*) {};
	virtual void    pfmPRC (Packet*) {};
	virtual void	pfmPDE (Packet*) {};

	void setQSLimit (Long l = MAX_Long);    // Sets QSLimit (see above)

	// Counters

	Long            NQMessages,     // Number of queued messages
			NTMessages,     // Number of transmitted messages
			NRMessages,     // Number of received messages
			NTPackets,      // Number of transmitted packets
			NRPackets;      // Number of received packets

	BITCOUNT        NQBits,         // Number of queued bits
			NTBits,         // Number of transmitted bits
			NRBits;         // Number of received bits

	TIME            SMTime;         // When measurement started

	// Traffic version of receive (checks against type consistency)

#if	ZZ_NOL
	inline void receive (Packet *p, Link *l = (Link*) NULL);

	inline void receive (Packet &p, Link &l) {
		receive (&p, &l);
	};

	inline void receive (Packet *p, Port *pp) {
		receive (p, pp->Lnk);
	};

	inline void receive (Packet &p, Port &pp) {
		receive (&p, (&pp)->Lnk);
	};
#else
	inline void receive (Packet *p);
#endif
	inline void receive (Packet &p) {
		receive (&p);
	};


#if	ZZ_NOR
	inline void receive (Packet *p, RFChannel *l);

	inline void receive (Packet &p, RFChannel &l) {
		receive (&p, &l);
	};

	inline void receive (Packet *p, Transceiver *pp) {
		receive (p, pp->RFC);
	};

	inline void receive (Packet &p, Transceiver &pp) {
		receive (&p, (&pp)->RFC);
	};
#endif
	void suspend ();
	void resume ();
	int isSuspended ();
	inline int isResumed () { return (! isSuspended () ); };

	void resetSPF ();

	// Gets a packet from a message queue and puts it into a buffer.
	// Returns NO, if there is no packet to get

	int     getPacket (Packet *b, Long mi = 0, Long mx = 0, Long fr = 0);

	inline  int     getPacket (Packet &p, Long mi = 0, Long mx = 0,
		    Long fr = 0) {

		return (getPacket (&p, mi, mx, fr));
	};

	int     getPacket (Packet *b, ZZ_MTTYPE, Long mi = 0, Long mx = 0,
		    Long fr = 0);

	inline  int     getPacket (Packet &p, ZZ_MTTYPE tf, Long mi = 0,
		    Long mx = 0, Long fr = 0) {

		return (getPacket (&p, tf, mi, mx, fr));
	};

	// Checks if there is a packet of "this" type to be acquired from
	// the Client

	int isPacket ();
	int isPacket (ZZ_MTTYPE);

	// Member version of release (checks against type consistency)

	inline void release (Packet *p);

	inline void release (Packet &p) {
		release (&p);
	};

	Long    RemBSI;         // Residual burst size


	void    setup (CGroup**, int, int, ...);
	void    zz_setup_flags (int);

	//      Arguments:
	//
	//      1. The array of communication groups
	//      2. The number of communication groups
	//      3. The flag interpreted as sum of the following
	//              
	//              MIT_exp
	//              MIT_unf
	//              MIT_fix
	//              MLE_exp
	//              MLE_unf
	//              MLE_fix
	//              BIT_exp
	//              BIT_unf
	//              BIT_fix
	//              BSI_exp
	//              BSI_unf
	//              BSI_fix
	//              SCL_on
	//              SCL_off
	//              SPF_on
	//              SPF_off
	//
	//      The remaining arguments are double precision floating
	//      point numbers in the order of declaration of "ParXXX"
	//      variables (see above). Only the relevant values are
	//      expected and should be specified
	//

	// An alias for a single communication group

	void    setup (CGroup*, int, ...);

	// An alias for a nonstandard pattern

	void    setup (int, ...);

	void    zz_start ();                    // Nonstandard constructor

	~Traffic () {

		excptn ("Traffic: [%1d] once created, cannot be destroyed",
			getId ());
	};
};

inline  void    zz_bld_Traffic (char *nn = NULL) {
	register Traffic *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Traffic);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

#if ZZ_NOL

#define	ZZ_LINK_ACTIVITY_GENERIC 	\
					\
	friend  class   Port;		\
	friend  class   Link;		\
	friend  class   Station;	\
	friend  ZZ_LINK_ACTIVITY *zz_gen_activity (int, Port*, int, Packet *p);\
	friend  class   LinkService;	\
					\
	private:			\
					\
	ZZ_LINK_ACTIVITY   *next,       \
			   *prev;       \
					\
	int                LRId;        \
	Port               *GPort;      \
	TIME               STime,       \
			   FTime;       \
	int                Type;        \
	ZZ_EVENT           *ae

/*
	ZZ_LINK_ACTIVITY   *next,               // Next on list
			   *prev;               // Previous on list

	int                LRId;                // Link-relative generator id
	Port               *GPort;              // Generator port
	TIME               STime,               // Time when started
			   FTime;               // Time when finished
	int                Type;                // Transfer, jam, etc.
	ZZ_EVENT           *ae;                 // Archiving event pointer
*/

class   ZZ_LINK_ACTIVITY {

	ZZ_LINK_ACTIVITY_GENERIC;

	Packet             Pkt;

	// Note: when the activity is generated, the amount of memory
	// needed is determined by the actual size of the packet data
	// structure, possibly extended by the user; this is why Pkt is
	// last

	public:

	ZZ_LINK_ACTIVITY  () {
		excptn ("LINK_ACTIVITY: cannot create directly");
	};
};

class	ZZ_LINK_ACTIVITY_JAM {

	ZZ_LINK_ACTIVITY_GENERIC;
};

#endif	/* NOL */

/* ------------------------- */
/* Packet buffer description */
/* ------------------------- */
class   ZZ_PFItem       {

	public:

	ZZ_PFItem       *next;

	Packet          *buf;

	ZZ_PFItem (Packet *p);
};

#endif	/* NOC */

#if ZZ_NOR

class ZZ_RF_ACTIVITY {
/*
 * Describes a packet being transmitted. Each of these has an event responsible
 * for its advancement.
 */
	public:

	friend class	RosterService;
	friend class	Transceiver;

	// Roster size
	int		NNeighbors;
	// This is the roster pointer, i.e. the array of Schedules, whose size
	// equals the number of nodes at wich the activity is heard.
	ZZ_RSCHED	*Roster,

	// Pointers to event schedule lists: ordered by time (i.e., distance
	// to the destination
			*SchBOP, *SchBOT, *SchEOT,

	// Pointers to the ends of the last two lists. The first one needs
	// no end pointer, as events are never stored there - only removed.
			*SchBOTL, *SchEOTL;

	// Originating transceiver
	Transceiver	*Tcv;

	// Rate at which we have been transmitted
	RATE		TRate;

	ZZ_EVENT	*RE,		// Scheduler event
			*RF;		// End of preamble event

	TIME		BOTTime;	// Origin's time of BOT, if known
	TIME		EOTTime;	// Origin's time of EOT, if known

	Boolean		Aborted;	// Preamble only

	/*
	 * The usage of this is exactly as for ZZ_LINK_ACTIVITY. No data must
	 * be declared below this point.
	 */
	Packet		Pkt;

	private:

	void handleEvent ();
	void triggerBOT ();

	/*
	 * We need an explicit destructor method as the standard destructor
	 * cannot be used, because ZZ_RF_ACTIVITY is created as a character
	 * array and deallocated as such.
	 */
	void destruct () {

		int i;
		ZZ_RSCHED *ro;

		if (Roster != NULL) {
			for (i = 0; i < NNeighbors; i++) {
				ro = Roster + i;
				ro->INT.stop ();
				if (ro->Destination->TracedActivity == ro)
					// Make sure this never points into
					// garbage
					ro->Destination->TracedActivity = NULL;
				pool_out (ro);
			}
			delete [] Roster;
		}
	};

	public:

	ZZ_RF_ACTIVITY  () {
		excptn ("LINK_ACTIVITY: cannot create directly");
	};

#ifdef	ZZ_RF_DEBUG
	void dump ();
#endif

};

#endif	/* NOR */

/* ------------------ */
/* Process definition */
/* ------------------ */
extern  int zz_Process_prcs;     // Equivalent to ANY (process)

class   Process : public AI {

#if	ZZ_NOS
	friend  class   RVariable;
#endif

#if	ZZ_NOC
	friend  class   Traffic;
#endif

#if	ZZ_NOL
	friend  class   Link;
	friend  class   Port;
#endif

#if	ZZ_NOR
	friend	class	RFChannel;
	friend	class	Transceiver;
#endif

	friend  class   ZZ_Object;
	friend  class   EObject;
	friend  class   Observer;
	friend  class   Station;
	friend  class   Mailbox;
	friend  class   ZZ_KERNEL;
	friend  class   ZZ_INSPECT;
	friend  int     main (int, char *[]);
	friend  void    zz_adjust_ownership ();
	friend  void    zz_sendObjectMenu (ZZ_Object*);
#ifdef  CDEBUG
	friend  void    zz_dumpTree (ZZ_Object*);
#endif
        friend  int     zz_noevents ();
	friend  ZZ_Object *zz_findAdvance (ZZ_Object*);
	friend  ZZ_Object *zz_findWAdvance (ZZ_Object*);
	friend 	void	zz_pcslook (ZZ_Object*);

	private:

	static Long sernum;             // Serial number

	Station         *Owner;         // The station the proceess runs at
	Process		*Father;	// The creating process
	ZZ_REQUEST      *TWList,        // Processes waiting for termination
					// Signal wait requests also kept here

			*SWList;        // Processes waiting for states
	ZZ_Object       *ChList;        // Objects owned by the process

	Process         *ISender;       // Interrupt sender
	void            *ISpec;         // Interrupt specification

	// Exposure functions

	void    exPrint0 (const char*, int);  // Request list
	void    exPrint1 (const char*);       // Wait list

	void    exDisplay0 (int);       // Request list
	void    exDisplay1 ();          // Wait list

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	void    zz_start ();            // The constructor

	// The address of zz_"typename"_prcs, where "typename" is the name
	// of the process class uniquely identifies the
	// process type and is required to implement observers.
	// This address is kept in the process attribute zz_typeid

	void    *zz_typeid;

	// The process code

	virtual void zz_code () {
		excptn ("Process: process %s code undefined", getTName ());
	};

	// Returns the name of the state number i
	virtual const char *zz_sn (int);

#if  ZZ_TAG
	void    wait (int, int, LONG tag = 0L);
	void    setSP (LONG);   // Set startup priority
#else
	void    wait (int, int);
#endif
	int	nwait ();
	friend void terminate (Process *p);

	inline void terminate () {
		::terminate (this);
	};

	// returns the number of alive children
	int children ();

	// Sends a regular signal to the process
	int signal (void *sp = NULL);

	// Sends a priority signal to the process
	int signalP (void *sp = NULL);

	// Checks if a signal is pending
	int isSignal ();

	// Erases a pending signal at the process
	int erase ();

	// Destructor for derived classes
	virtual	~Process ();

        inline Station *getOwner () { return Owner; };
	inline void *getTypeId () { return zz_typeid; };

	inline  void    printRqs (const char *hd = NULL, Long sid = NONE) {
		// Print request list
		printOut (0, hd, sid);
	};

	inline  void    printWait (const char *hd = NULL) {
		// Print wait requests of this process
		printOut (1, hd);
	};
};

inline  void    zz_bld_Process (char *nn = NULL) {
	register Process *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Process);
	p->zz_start ();
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

Long zz_getproclist (Station*, void*, Process**, Long);

/* --------- */
/* Observers */
/* --------- */

class   ZZ_INSPECT      {

/* --------------------------------------- */
/* Description of a single inspect request */
/* --------------------------------------- */

	friend  class   Observer;
	friend  class   ObserverService;
	friend  void    terminate (Observer*);
	friend  int     main (int, char *[]);

	ZZ_INSPECT      *next;          // To link inspect requests into lists
	Station         *station;       // The inspected station
	void            *typeidn;       // Process type identifier

	char            *nickname;      // Process nickname

	int             pstate,         // Process state
			ostate;         // Observer's state

	const char      *getptype (),   // Return process type name
			*pttrav (ZZ_Object*); // A recursive helper for getptype
	public:

	inline ZZ_INSPECT (Station*, void*, char*, int, int);
};

class   Observer : public AI {

/* ------------------ */
/* The observer frame */
/* ------------------ */

#if	ZZ_NOS
	friend  class   RVariable;
#endif

	friend  class   ZZ_INSPECT;
	friend  class   ZZ_Object;
	friend  class   EObject;
	friend  class   ObserverService;
	friend  int     main (int, char *[]);
	friend  void    zz_timeout_rq (TIME, int);
	friend  void    zz_inspect_rq (Station*, void*, char*, int, int);
#ifdef  CDEBUG
	friend  void    zz_dumpTree (ZZ_Object*);
#endif
	friend  void zz_sendObjectMenu (ZZ_Object*);
	friend  ZZ_Object *zz_findAdvance (ZZ_Object*);
	friend  ZZ_Object *zz_findWAdvance (ZZ_Object*);

	private:

	// Besides belonging to its creator, an observer is kept on a
	// separate list, to speed up searching. Note: the following two
	// pointers must be the first attributes of Observer.

	Observer        *next, *prev;

	static Long sernum;             // Serial number

	ZZ_Object       *ChList;        // The objects owned

	// Hash masks -- to speed up things a little bit
	LONG            smask,          // Station mask
			pmask,          // Process mask
			nmask,          // Nickname mask
			amask;          // State mask

	ZZ_INSPECT      *inlistHead;    // The list of inspect requests

	ZZ_EVENT        *tevent;        // Timeout event pointer

	// Exposure functions

	void    exPrint0 (const char*);       // Global info
	void    exPrint1 (const char*);       // Inspect list

	void    exDisplay0 ();          // Global info
	void    exDisplay1 ();          // Inspect list

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	void    zz_start ();            // The constructor

	// The observer's code -- to be provided by the user
	virtual void zz_code () {
		excptn ("Observer: observer %s code undefined", getTName ());
	};

	// Returns the name of the observer state number i
	virtual const char *zz_sn (int);

	friend void terminate (Observer *o);

	inline void terminate () {
		::terminate (this);
	};

	inline  void    printAll (const char *hd = NULL) {
		// Information about all observers
		printOut (0, hd);
	};

	inline  void    printIns (const char *hd = NULL) {
		// Print inspect list of this observer
		printOut (1, hd);
	};
};

inline  void    zz_bld_Observer (char *nn = NULL) {
	register Observer *p;
	zz_COBJ [++zz_clv] = (void*) (p = new Observer);
	// Note: zz_start for Observers is called by smpp
	if (nn != NULL) {
		p->zz_nickname = new char [strlen (nn) + 1];
		strcpy (p->zz_nickname, nn);
	}
}

/* --------------------------------------------- */
/* Data structures used for displaying, stepping */
/* --------------------------------------------- */

/* ---------------- */
/* An active window */
/* ---------------- */

class   ZZ_WINDOW       {

	public:                         // This type is invisible anyway

	ZZ_WINDOW       *next, *prev;   // List pointers
	ZZ_Object       *obj;           // The displayable object
	short           mode,           // Display mode
			flag;           // E.g. starting, terminating

	int             stid,           // Station id
			scount,         // Item number limits
                        ecount;

	void            *frame;         // Associated object

	// Constructors

	ZZ_WINDOW (ZZ_Object*, int, int, int, int);
};

/* -------------------------------------- */
/* An element of the stepped windows list */
/* -------------------------------------- */

class   ZZ_SIT          {

	public:                         // Invisible anyway

	ZZ_SIT          *next, *prev;   // List pointers

	ZZ_WINDOW       *win;           // Window pointer
	Station         *station;       // Station to be awakened
	Process         *process;       // Process to be run
	AI              *ai;            // AI generating the event
	Observer        *obs;           // For stepping observers
};

/* ------------------------------------------------------- */
/* An element of the waiting list of windows to be stepped */
/* ------------------------------------------------------- */

class   ZZ_WSIT         {

	public:                         // Invisible anyway

	ZZ_WSIT         *next, *prev;   // List pointers

	Long            cevent;         // Starting event number
	TIME            ctime;          // Starting time

	ZZ_WINDOW       *win;           // Window pointer
	Station         *station;       // Station to be awakened
	Process         *process;       // Process to be run
	AI              *ai;            // AI generating the event
	Observer        *obs;           // For stepping observers
};

/* ---------------------------------------------------------- */
/* A  special  station type representing the system agent. It */
/* is a numberless station which is created before any  other */
/* station.                                                   */
/* ---------------------------------------------------------- */

class ZZ_SYSTEM : public Station {

	public:

	virtual const char *getTName () {
		return ("SYSTEM");
	};

	friend  class   Process;
	friend  void    buildNetwork ();

	private:

	// Exposure functions

	void    exPrint0 (const char*);       	// Network topology
	void    exPrint1 (const char*);       	// Abbreviated topology

#if	ZZ_NOL
	static void makeTopologyL ();    	// Topology cleanup
#endif
#if	ZZ_NOR
	static void makeTopologyR ();    	// Topology cleanup
#endif
	static void makeTopology ();

	public:

	void setup () {
		Id = NONE;
		sernum = 0;
		NStations = 0;
	};

	// For exposing special things that don't belong to any
	// specific AI's or other objects

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	inline  void    printTop (const char *hd = NULL) {
		// Print network topology
		printOut (0, hd);
	};

	inline  void    printATop (const char *hd = NULL) {
		// Print abbreviated topology
		printOut (1, hd);
	};
};

/* ------------------- */
/* System process type */
/* ------------------- */

class   ZZ_SProcess : public Process {

	public:
	const char **zz_sl;
	int     zz_ns;
	const   char *zz_sn (int);
};

/* ---------------------------------------------------------- */
/* A  special  process  type  to  represent  the dummy Kernel */
/* process acting as the  terminator  of  the  user's  driver */
/* process                                                    */
/* ---------------------------------------------------------- */

extern  int     zz_KERNEL_prcs;

class   ZZ_KERNEL : public ZZ_SProcess {

	public:

	virtual const char *getTName () {
		return ("KERNEL");
	};

	enum {Start};                   // Only one dummy state

	private:

	// EXposure functions

	void    exPrint0 (const char*, int); // Global request queue
	void    exPrint1 (const char*, int); // Abbreviated global request queue
	void    exPrint2 (const char*);      // Global simulation status

	void    exDisplay0 (int);       // Global request queue
	void    exDisplay1 (int);       // Abbreviated global request queue
	void    exDisplay2 ();          // Global simulation status
	void    exDisplay3 ();          // Last event

	public:

	// Exposable
	virtual void zz_expose (int, const char *h = NULL, Long s = NONE);

	void setup () {
		zz_typeid = (void*) (&zz_KERNEL_prcs);
		Id = NONE;
		sernum = 0;
		// Only one dummy state
		zz_ns = 1;
		zz_sl = new const char* [Start + 1];
		zz_sl [0] = "Start";
	};

	// For exposing special things that don't belong to any
	// specific AI's or other objects

	inline  void    printSts (const char *hd = NULL) {
		// Print global simulation status
		printOut (2, hd);
	};

	inline  void    printRqs (const char *hd = NULL, Long s = NONE) {
		// Request queue
		printOut (0, hd, s);
	};

	inline  void    printARqs (const char *hd = NULL, Long s = NONE) {
		// Abbreviated request queue
		printOut (1, hd, s);
	};

	void zz_code () {

	    switch (TheState) {

		case Start:
			break;
			// See? I told you -- it does nothing
	    }
	}
};

/* ------------------------ */
/* Global tracing functions */
/* ------------------------ */

#define	TRACE_OPTION_TIME		1
#define	TRACE_OPTION_ETIME		2
#define	TRACE_OPTION_STATID		4
#define	TRACE_OPTION_PROCESS		8
#define	TRACE_OPTION_STATE		16

void	trace (const char*, ...);

void settraceFlags (FLAGS);
void settraceTime (TIME fr, TIME upto = TIME_inf);
void settraceTime (double fr, double upto = Time_inf);
void settraceStation (Long st);

// These are void if not ZZ_DBG
void setTraceFlags (Boolean);
void setTraceTime (TIME fr, TIME upto = TIME_inf);
void setTraceTime (double fr, double upto = Time_inf);
void setTraceStation (Long st);

/* ---------------------------------------------------------- */
/* Inline   functions   removed   from   objects  to  resolve */
/* cross-reference conflicts                                  */
/* ---------------------------------------------------------- */

#if	ZZ_NOL

#if  ZZ_TAG
inline void Port::transmit (Packet *p, int s, LONG tag) {

	startTransfer (p);
	assert (TRate != 0, "Port->transmit: %s, transmission rate undefined",
		getSName ());
	Timer->wait ((TIME)TRate * (LONG)(p->TLength), s, tag);
};

inline void Port::transmit (Packet &p, int s, LONG tag) {

	startTransfer (&p);
	assert (TRate != 0, "Port->transmit: %s, transmission rate undefined",
		getSName ());
	Timer->wait ((TIME)TRate * (LONG)(p.TLength), s, tag);
};
#else
inline void Port::transmit (Packet *p, int s) {

	startTransfer (p);
	assert (TRate != 0, "Port->transmit: %s, transmission rate undefined",
		getSName ());
	Timer->wait ((TIME)TRate * (LONG)(p->TLength), s);
};

inline void Port::transmit (Packet &p, int s) {

	transmit (&p, s);
};
#endif

inline void Link::stdpfmMTR (Packet *p) {
	NTMessages++;
};

inline void Link::stdpfmMRC (Packet *p) {
	NRMessages++;
};

inline void Link::stdpfmPTR (Packet *p) {
	NTPackets++;
	NTBits += p->ILength;
};

inline void Link::stdpfmPRC (Packet *p) {
	NRPackets++;
	NRBits += p->ILength;
};

inline void Link::stdpfmPAB (Packet *p) {
	// Empty yet
};

#if ZZ_FLK
inline void Link::stdpfmPDM (Packet *p) {
	NDPackets++;
	if (flagSet (p->Flags, PF_hdamaged)) NHDPackets++;
	NDBits += p->ILength;
};
#endif

#endif	/* ZZ_NOL */

#if	ZZ_NOR

inline void RFChannel::stdpfmMTR (Packet *p) {
	NTMessages++;
};

inline void RFChannel::stdpfmMRC (Packet *p) {
	NRMessages++;
};

inline void RFChannel::stdpfmPTR (Packet *p) {
	NTPackets++;
	NTBits += p->ILength;
};

inline void RFChannel::stdpfmPRC (Packet *p) {
	NRPackets++;
	NRBits += p->ILength;
};

inline void RFChannel::stdpfmPAB (Packet *p) {
	// Empty
};

#if  ZZ_TAG
inline void Transceiver::transmit (Packet *p, int s, LONG tag) {

	// TRate verified by startTransfer
	startTransfer (p);
	Timer->wait (RFC->RFC_xmt (TRate, p->TLength) + getPreambleTime (),
		s, tag);
};

inline void Transceiver::transmit (Packet &p, int s, LONG tag) {

	transmit (&p, s, tag);
};
#else
inline void Transceiver::transmit (Packet *p, int s) {

	startTransfer (p);
	Timer->wait (RFC->RFC_xmt (TRate, p->TLength) + getPreambleTime (), s);
};

inline void Transceiver::transmit (Packet &p, int s) {

	transmit (&p, s);
};
#endif

inline TIME Transceiver::getXTime (Long bits) {
	return RFC->RFC_xmt (TRate, bits) + getPreambleTime ();
};

inline Boolean Transceiver::isFollowed (Packet *p) {
	return TracedActivity != NULL && p == &(TracedActivity->RFA->Pkt);
};

#endif	/* ZZ_NOR */


#if	ZZ_NOC

inline int zz_client::getPacket (Packet &p, Long mi, Long mx, Long fr) {

	return (getPacket (&p, mi, mx, fr));
};

inline int zz_client::getPacket (Packet &p, ZZ_MTTYPE tf, Long mi, Long mx,
	Long fr) {

	return (getPacket (&p, tf, mi, mx, fr));
};

/* -------------------------------------------------------- */
/* Releases a packet that has been successfully transmitted */
/* -------------------------------------------------------- */

inline void zz_client::release (Packet &p) {
	release (&p);
};

/* -------------------------- */
/* Traffic version of receive */
/* -------------------------- */
#if	ZZ_NOL
inline void Traffic::receive (Packet *p, Link *l) {
	assert (p->TP == Id, "Traffic->receive: %s, illegal Traffic Id %1d",
		getSName (), p->TP);
	Client->receive (p, l);
};
#else
inline void Traffic::receive (Packet *p) {
	assert (p->TP == Id, "Traffic->receive: %s, illegal Traffic Id %1d",
		getSName (), p->TP);
	Client->receive (p);
};
#endif /* NOL */

#if	ZZ_NOR
inline void Traffic::receive (Packet *p, RFChannel *l) {
	assert (p->TP == Id, "Traffic->receive: %s, illegal Traffic Id %1d",
		getSName (), p->TP);
	Client->receive (p, l);
};
#endif

/* --------------------------------------------------------- */
/* Traffic  member  version  of release (checks against type */
/* consistency)                                              */
/* --------------------------------------------------------- */

inline void Traffic::release (Packet *p) {
	assert (p->TP == Id, "Traffic->release: %s, illegal Traffic Id %1d",
		getSName (), p->TP);
	Client->release (p);
};

/* ------------------ */
/* Non-member receive */
/* ------------------ */
#if	ZZ_NOL
inline  void   receive (Packet &p, Link *l = NULL) {
	Client->receive (&p, l);
};
inline  void   receive (Packet &p, Link &l) {
	Client->receive (&p, &l);
};
#else
inline  void   receive (Packet &p) {
	Client->receive (&p);
};
#endif	/* NOL */

#if	ZZ_NOR
inline  void   receive (Packet &p, RFChannel *l) {
	Client->receive (&p, l);
};
inline  void   receive (Packet &p, RFChannel &l) {
	Client->receive (&p, &l);
};

#define	dBToLin(v)	pow (10.0, (double)(v) / 10.0)

inline	double linTodB (double v) {

	if (v <= 0.0)
		excptn ("linTodB: argument must be > 0.0, is %f", v);
	return log10 (v) * 10.0;
};

inline double dist (	double x0, double y0,
#if ZZ_R3D
  double z0,
#endif
			double x1, double y1
#if ZZ_R3D
, double z1
#endif
								) {
	return sqrt (
#if ZZ_R3D
			(z1 - z0) * (z1 - z0) +
#endif
			(y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0));
};

#endif	/* NOR */

inline Packet::~Packet () {
	if (isStandard ())
		idToTraffic (TP)->pfmPDE (this);
};

inline Message::~Message () {
	if (isStandard ())
		idToTraffic (TP)->pfmMDE (this);
};

#endif	/* NOC */

/* ------------------ */
/* INSPECT contructor */
/* ------------------ */

inline ZZ_INSPECT::ZZ_INSPECT (Station *s, void *pt, char *nik, int st, int os){

#if     ZZ_OBS

	station = s;
	typeidn = pt;
	nickname = nik;
	pstate = st;
	ostate = os;

	if (zz_inlist_tail == NULL) {
		// The first inspect
		zz_inlist_tail = zz_current_observer->inlistHead = this;
	} else {
		zz_inlist_tail -> next = this;
		zz_inlist_tail = this;
	}
	next = NULL;
#endif
};

/* ----------------------------------------------- */
/* Announcing one auxiliary function for observers */
/* ----------------------------------------------- */
void    zz_timeout_rq (TIME, int);

/* ---------- */
/* For create */
/* ---------- */

extern  Station *zz_CSTA [];
extern  int     zz_csv;

inline void zz_setths (Long i) { 
	zz_CSTA [zz_csv++] = TheStation;
	TheStation = idToStation (i);
}

#ifndef ZZ_Long_eq_int
inline void zz_setths (int i) {
	zz_CSTA [zz_csv++] = TheStation;
	TheStation = idToStation (i);
}
#endif

inline void zz_setths (Station *s) {
	zz_CSTA [zz_csv++] = TheStation;
	TheStation = s;
}

inline void zz_remths () {
	TheStation = zz_CSTA [--zz_csv];
}

#include        "inlines.h"
