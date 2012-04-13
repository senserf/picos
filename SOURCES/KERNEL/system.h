/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */


/* -------------------------------------- */
/* User-invisible macros, constants, etc. */
/* -------------------------------------- */


#include        "global.h"

#include	"../LIB/syshdr.h"

#define	BII	INTBITS

#if	__WORDSIZE <= 32
#define	BIL	(LONGBITS + LONGBITS)
#else
#define	BIL	LONGBITS
#endif

#ifdef	ZZ_LONG_is_not_long
#define	LCS(a)	a ## LL
#else
#define	LCS(a)	a ## L
#endif

/* --------------------------------------- */
/* Global variables (external definitions) */
/* --------------------------------------- */

extern  ZZ_KERNEL       *Kernel,         // The system process
			*ZZ_Kernel;      // Another pointer to it

/* ----------- */
/* Environment */
/* ----------- */

#if	ZZ_NFP
// We are hardwired at 1 ITU = 1 us
#define	MILLISECOND	1000
#else
extern  double          Itu, Etu;
#if	ZZ_REA || ZZ_RSY
extern  double          MSecPerITU, ITUPerMSec;
#endif
#if	ZZ_RSY
extern	Long ZZ_ResyncMsec;
#endif
#endif

#if  ZZ_REA || ZZ_RSY
extern  int             zz_nfdesc; 
extern  Long		zz_max_sleep_time;
#if  ZZ_JOU
extern  ZZ_Journal      *ZZ_Journals;
extern  char		*ZZ_OffsetArg;
#endif
#endif

extern  int             TheObserverState;       // The observer state

extern  ZZ_EVENT        *zz_CE;                 // Current event

#if	ZZ_NOC
/* ------------------------------ */
/* Global counters for the Client */
/* ------------------------------ */
extern  Long            zz_NQMessages,          // Number of queued messages
			zz_NTMessages,          // Number of transmttd messages
			zz_NRMessages,          // Number of received messages
			zz_NTPackets,           // Number of transmitted packets
			zz_NRPackets,           // Number of received packets
			zz_NGMessages;          // Number of generated messages

extern  TIME            zz_GSMTime; // Start time for global Client measurements

extern  BITCOUNT        zz_NQBits,              // Number of queued bits
			zz_NTBits,              // Number of transmitted bits
			zz_NRBits;              // Number of received bits
#endif	/* NOC */

/* ---------------------------------------------------------- */
/* Temporary  list  heads  for creating station ports, packet */
/* buffers, and mailboxes                                     */
/* ---------------------------------------------------------- */
#if	ZZ_NOL
extern  Port            *zz_ncport;
#endif

#if	ZZ_NOR
extern	Transceiver	*zz_nctrans;

#define N_TRANSCEIVER_EVENTS ZZ_N_TRANSCEIVER_EVENTS
#endif

#if	ZZ_NOC
extern  ZZ_PFItem       *zz_ncpframe;
#endif
extern  Mailbox         *zz_ncmailbox;

/* --------- */
/* Debugging */
/* --------- */
#if	ZZ_DBG
extern	int		DebugTracingFull;
#endif

// To indicate that time limit cannot be reset by setLimit
#define	TRACE_OPTION_TIME_LIMIT_SET	31

extern	FLAGS		zz_trace_options;

extern  int 		EndOfData;              // EOF flag

/* ------------------------------------- */
/* Boundary conditions of the simulation */
/* ------------------------------------- */

#if	ZZ_NOC
extern  Long    zz_max_NMessages;               // Default limits and flags
#endif

extern  TIME    zz_max_Time;                    // Everything is by default
#if	ZZ_NFP
extern  Long    zz_max_cpuTime;                 // unlimited
#else
extern  double  zz_max_cpuTime;                 // unlimited
#endif
extern  int     zz_exit_code;                   // Termination code

/* ----------------------- */
/* Program call parameters */
/* ----------------------- */

extern  char    **zz_callArgs;
extern  int     zz_callArgc;                    // Argument count

		// Original seeds to be included in error messages; initialized
		// in a standard way (changeable by a call parameter)
extern  Long    zz_OrgSeeds [ZZ_N_SEEDS];

extern  int
#if	ZZ_NOC
		// A flag used to tell whether message queues should be
		// emptied after zz_max_NMessages have been generated
		zz_flg_emptyMQueues,

		// Tells whether the standard Client is to be started. Can be
		// switched off by a program call parameter.
		zz_flg_stdClient,
#endif

		// Tells whether definitions of standard objects should be
		// automatically echoed to the output file
		zz_flg_printDefs,

		// Tells whether information about system events should be
		// suppressed from exposures
		zz_flg_nosysdisp,

		// Tells wheter implicit process termination, occurring when
		// the process fails to issue at least one wait request in its
		// current state, is legit
		zz_flg_impterm,

		// Tells whether the definition of stations is complete
		zz_flg_started;

                // File with templates or null
extern  char    *zz_TemplateFile;

#if	ZZ_NOC
#if     ZZ_QSL
		// The global limit on the number of messages to be queued at
		// stations awaiting transmission. When this number is reached
		// new generated messages are ignored.
extern  Long    zz_QSLimit;
#endif
#endif
/* ---------------- */
/* Event processing */
/* ---------------- */

extern  ZZ_EVENT        *zz_c_wait_event;       // Currently built event

extern  Long    zz_npre;                        // Number of processed events

extern  LPointer zz_event_id;                   // Current event id

extern  int     zz_c_first_wait,                // Wait for the first time
#if	ZZ_NOL || ZZ_NOR
		zz_temp_event_id,               // Last port event id
#endif
		zz_chckint;                     // Time limit check interval
						// Also used as checkpoint flag

extern  ZZ_REQUEST *zz_c_whead;                 // Built wait list head

#if  ZZ_TAG
extern  ZZ_TBIG zz_c_wait_tmin;                 // Earliest request so far
#else
extern  TIME    zz_c_wait_tmin;                 // Earliest request so far
#endif

extern	ZZ_REQUEST *zz_orphans;			// Orphaned wait requests

extern  AI      *zz_ai;                         // The current AI pointer

#if     ZZ_OBS

/* ------------------- */
/* Observers internals */
/* ------------------- */

extern  Observer   *zz_obslist;                 // The list of all observers

#endif

#if     ZZ_TOL
/* -------------------------- */
/* Clock tolerance parameters */
/* -------------------------- */
extern  int    zz_quality;                      // The default clock quality
extern  double zz_tolerance;                    // The default clock tolerance
#endif

/* ---------------------- */
/* Display (visible part) */
/* ---------------------- */

extern  int          DisplayInterval,           // Display interval
		     DisplayOpening,            // Set for a new findow
		     DisplayClosing;            // The window is being closed

/* ------------------------ */
/* Display (invisible part) */
/* ------------------------ */

extern  ZZ_WINDOW       *zz_windows,            // Active windows
			*zz_the_window;         // Current window
extern  ZZ_SIT          *zz_steplist;           // Stepped windows
extern  ZZ_WSIT         *zz_steplist_delayed;   // Waiting to be stepped
extern  Long            zz_commit_event;        // Starting event number
extern  TIME            zz_commit_time;         // Starting time
extern  volatile Long   zz_events_to_display;   // Delay to next display refresh
extern  int             zz_drequest,            // Display requested by '-d'
			zz_send_step_phrase;    // Flag: step condition met

/* ---------------- */
/* Standard streams */
/* ---------------- */

extern  istream *zz_ifpp;                       // Input stream
extern  ostream *zz_ofpp;                       // Output file

extern  char    *zz_ifn,                        // Pointer to input filename
		*zz_ofn;                        // Pointer to output filename

/* ----------------- */
/* Smurph process id */
/* ----------------- */

extern  int     zz_processId;

/* -------------------------------------------- */
/* Plaintext versions for some hidden constants */
/* -------------------------------------------- */

#define         ACSSIZE                 ZZ_ACSSIZE
#define         EETSIZE                 ZZ_EETSIZE

#if     ZZ_ASR
#define         ASSERT                  1
#endif

#if     ZZ_OBS
#define         OBSERVERS               1
#endif

#if     ZZ_AER
#define         AERRORS                 1
#endif

#if     ZZ_TOL
#define         TOLERANCE               1
#endif

#if     ZZ_DBG
#define         DEBUGGING               1
#endif

#define		tohex(d)	((char)((d) > 9) ? (d) + 'a' - 10 : (d) + '0'))


/* ---------------------------------------------------------- */
/* Check  interval:  every  CHCKINT  events smurph will check */
/* against the CPU time limit                                 */
/* ---------------------------------------------------------- */

#define         CHCKINT                 5000

#if ZZ_REA || ZZ_RSY

// Asynchronous I/O check interval (in milliseconds) for real mode
#define         IOCHECKINT              1000   // One second
// For simulating select, if it doesn't exits: retry every SOCKCHKINT usec
#define		SOCKCHKINT              1000   // 0.001 sec

#if ZZ_JOU
// Journal magic
#define         JOURNALMAGIC            "SIDEJRNL"
// Connect header
#define         CONNECTMAGIC            "CNCT"
// Journal header length
#define		JHEADERLENGTH		12
// Creation time offset in journal header
#define		JHTIMEOFFSET		8
// Connection block magic. The length of the connect block must be the
// same as the length of the update header.
#define         BLOCKMAGIC              "NEWBL"
// Block header length
#define		JBHEADERLENGTH		13
// Length field offset in an update block
#define		JBLENGTHOFFSET		(JBHEADERLENGTH-4)
// Data block time offset
#define		JBTIMEOFFSET		1
// Dummy file descriptor representing a mailbox read from a journal
#define		JOURNAL                 32768
// Scratch buffer size (allocated on the stack)
#define		JBUFSIZE		4096
#endif	/* JOU */

#endif	/* REA, RSY */

#if	ZZ_NOC
#if	ZZ_NOS
/* --------------------------------------------------- */
/* Scratch non-displayable RVariable (used internally) */
/* --------------------------------------------------- */
class   ZZ_TRVariable : public RVariable {

	friend  class   RVariable;

	public:

	// No additional attributes -- just a separate destructor

	~ZZ_TRVariable ();
};
#endif
#endif

void	zz_run_delayed_initializers ();

/* ---------------------------------------------------------- */
/* Exposure   macros.  In  the  user  protocol  file  similar */
/* constructs are processed by smpp.                          */
/* ---------------------------------------------------------- */
#define sexposure(otp) void otp::zz_expose (int zz_emode, const char *Hdr, Long SId){\
	int     zz_em;

#define sonpaper ZZ_ONPP: if ((zz_em = zz_emode) >= ZZ_EMD_0) goto ZZ_ONSC;\
			  if (zz_em == ANY) return; zz_emode = ANY;\
			  switch (zz_em)

#define sonscreen ZZ_ONSC:if ((zz_em = zz_emode) < ZZ_EMD_0) goto ZZ_ONPP;\
			  zz_emode = ANY; zz_em -= ZZ_EMD_0;\
			  switch (zz_em)

#define sexmode(m)  break; case m: zz_dispfound = YES;
#define sfxmode(m)  case m: zz_dispfound = YES;

/* ------------------------------- */
/* User-invisible global functions */
/* ------------------------------- */
#if	ZZ_NOC
void    zz_start_client ();             // Starts the standard Client
#endif

void    zz_init_system (int, char*[]);  // Initialize files and options
void    zz_init_windows ();             // Initialize DDF
void    zz_checkpoint ();               // Checkpoint request
void    zz_check_asyn_io ();            // Simulates asynchronous i/o
#if     DEBUGGING
void    zz_print_debug_info ();         // One line of debugging information
#endif
void    zz_print_event_info (ostream&); // Prints out last event information
Long    zz_trunc (LONG, int);           // Truncate the number to int digits
istream *zz_openIStream (const char*);
ostream *zz_openOStream (const char*, const char*);

#define openIStream(f)   zz_openIStream (f)
#define openOStream(f,g)   zz_openOStream (f, g)

#define zz_tr32(a) zz_trunc (a, 10)

void    zz_closeMonitor ();
int     zz_msock_stat (int&);           // Monitor socket status
void    zz_dterminate ();               // Sends termination phrase to server
void    zz_ptime (TIME&, int);          // Prints out time values
void	zz_outth ();			// Time header for paper exposures
#if  ZZ_TAG
void    zz_ptime (ZZ_TBIG&, int);
#define pless(a,b) ((a).cmp (b) < 0)
#else
#define pless(a,b) ((a) < (b))
#endif

#define ptime(a,b)      zz_ptime (a,b)  // A system alias for the above

/* ------------------------------ */
/* A macro to display time values */
/* ------------------------------ */
#define dtime(a)        do { if (def (a)) display(a); else\
			display ("undefined"); } while (0)

/* -------------------------- */
/* The number of random seeds */
/* -------------------------- */
#define         N_SEEDS         ZZ_N_SEEDS

#if	ZZ_NOL
/* ------------------------------------ */
/* Number of (user-visible) link events */
/* ------------------------------------ */
#define         N_LINK_EVENTS           ZZ_N_LINK_EVENTS

/* ------------------------------ */
/* Protocol-invisible link events */
/* ------------------------------ */
#define         LNK_PURGE       N_LINK_EVENTS
#define         ARC_PURGE       (N_LINK_EVENTS+1)
#define         BOT_HEARD       (N_LINK_EVENTS+2)
#define         EOT_HEARD       (N_LINK_EVENTS+3)

/* ------------------------------------- */
/* Protocol-invisible bus activity types */
/* ------------------------------------- */
#define         TRANSFER_J              6       // Followed by a jam

#endif	/* NOL */

#if	ZZ_NOR || ZZ_NOL
#define         TRANSFER_A              5       // Transfer attempt
#endif

#if	ZZ_NOR
#define		N_RFCHANNEL_EVENTS	ZZ_N_RFCHANNEL_EVENTS
/* ----------------------------------- */
/* Protocol-invisible RFChannel events */
/* ----------------------------------- */
#define         DO_ROSTER       (N_RFCHANNEL_EVENTS+1)
#define         RACT_PURGE      (N_RFCHANNEL_EVENTS+2)
#define         BOT_TRIGGER     (N_RFCHANNEL_EVENTS+3)

#endif	/* NOR */

#if	ZZ_NOC
/* ------------- */
/* Client events */
/* ------------- */
#define         ARR_MSG                 -6
#define         ARR_BST                 -7
#endif	/* NOC */

/* ---------------------------------------------------------- */
/* A  virtual event generated by the process AI -- to start a */
/* process for the first time                                 */
/* ---------------------------------------------------------- */
#define         START                   (-3)

#if     OBSERVERS
/* ------------------------- */
/* Observer AI service event */
/* ------------------------- */
#define         OBS_TSERV               0
#endif

/* ---------------------------------------------------------- */
/* Special  request  'id'  attribute  value   to tell entries */
/* requiring special postprocessing upon deallocation         */
/* ---------------------------------------------------------- */
#define         RQTYPE_PRC              (-3)    // Waiting for process

/* ------------------------------------------------- */
/* Base port number for user accessible socket ports */
/* ------------------------------------------------- */
#define         SOCK_BASE       2000

/* --------------- */
/* Observer macros */
/* --------------- */
#if     ZZ_OBS
#define         if_not_from_observer(t) assert (zz_observer_running, t)
#define         if_from_observer(t)     assert (!zz_observer_running, t)
#else
#define         if_not_from_observer(t) NOP
#define         if_from_observer(t)     NOP
#endif


/* -------------------------------------------- */
/* Convenient aliases for user-invisible macros */
/* -------------------------------------------- */

#define         ierror(a)       zz_ierror(a)
#define         aerror(a)       zz_aerror(a)

#define narray(name,type,size)  type name [size];
#define darray(name)
#define STATIC
#define USESID
#define USEEMD
#define RWCAST

#if	ZZ_DET
#define	FLIP     0
#define TOSS(a)  0
#else
#define FLIP     (flip ())
#define TOSS(a)  (toss (a))
#endif

#define         SIGCHKP         SIGUSR1         // Checkpoint signal
#define         OMASK           0022            // Output file mask

// Create a compound ID
#define	MKCID(a,b)	(((a) << 16) | (b))
// Extracting sub IDs from Port, Transceiver, Mailbox IDs
#define	GSID(id)	(((id) >> 16) & 0xFFFF)
#define	GYID(id)	(((id)      ) & 0xFFFF)
#define	GSID_NONE	0xFFFF

// Trigger segmentation violation to abort the program for gdb
#define	EXCPTN		do { *((Long*)0) = 1; } while (1)
