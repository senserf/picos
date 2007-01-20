/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ------------------------------ */
/* Global variables (definitions) */
/* ------------------------------ */

#include        "global.h"
#include        "display.h"
#include	"../LIB/syshdr.h"

/* ------------------ */
/* Standard fixed AIs */
/* ------------------ */

zz_timer        zz_AI_timer;
zz_monitor	zz_AI_monitor;

#if	ZZ_NOC
zz_client       zz_AI_client;
unsigned int	zz_pvoffset = 0xffffffff;
#endif

ZZ_SYSTEM       *System = NULL;         // The system station
ZZ_KERNEL       *Kernel = NULL,         // The system process
		*ZZ_Kernel = NULL;      // Another copy of the above pointer
Process         *ZZ_Main = NULL;        // The user's root process

/* ----------- */
/* Environment */
/* ----------- */
#if  ZZ_NFP
#else

#if  ZZ_REA || ZZ_RSY
// When operating in real time, assume by default that 1 ITU = 1us
double          Itu = 0.000001, Etu = 1000000.0, Du = 1.0;   // Time units
double          MSecPerITU = 0.001,   // Milliseconds per ITU
                ITUPerMSec = 1000.0;  // ITUs per millisecond
#else
double          Itu = 1.0, Etu = 1.0, Du = 1.0;   // Time units
#endif

#endif	/* NFP */

TIME            Time = TIME_0;          // The simulated time
Station         *TheStation = NULL;     // The current station
Process         *TheProcess = NULL;     // The current process
int             TheState,               // The process state
		TheObserverState;       // The observer state

void            *Info01 = NULL,         // GP environment variables
		*Info02 = NULL,

		*zz_mbi;                // For mailbox outItem

ZZ_EVENT        *zz_CE;                 // Current event


/* ------ */
/* Counts */
/* ------ */
Long            NStations = 0;          // The total number of stations
Station         **zz_st = NULL;         // Station table

#if	ZZ_NOL
Long		NLinks = 0;             // The number of links
Link            **zz_lk = NULL;         // Link table
Port            *zz_ncport = NULL;	// Temporary head for port list
#endif

#if	ZZ_NOR
Long		NRFChannels = 0;	// The number of radio channels
RFChannel	**zz_rf = NULL;		// Channel table
Transceiver	*zz_nctrans = NULL;	// Temporary head for transceiver list
#endif

#if	ZZ_NOC
Long		NTraffics = 0;          // The number of message patterns
Traffic         **zz_tp = NULL;         // Array of Traffics
ZZ_PFItem       *zz_ncpframe = NULL;	// Temporary head for packet buffers

/* ------------------------------ */
/* Global counters for the Client */
/* ------------------------------ */
Long            zz_NQMessages,          // Number of queued messages
		zz_NTMessages,          // Number of transmitted messages
		zz_NRMessages,          // Number of received messages
		zz_NTPackets,           // Number of transmitted packets
		zz_NRPackets,           // Number of received packets
		zz_NGMessages;          // Number of generated messages

TIME            zz_GSMTime; // Start time for global Client measurements

BITCOUNT        zz_NQBits,              // Number of queued bits
		zz_NTBits,              // Number of transmitted bits
		zz_NRBits;              // Number of received bits
#endif	/* NOC */

Mailbox         *zz_ncmailbox = NULL;	// Temporary head for mailbox list

/* --------- */
/* Debugging */
/* --------- */
TIME            TracingTime;            // Trace flag (initially off)
Long            TracedStation = NONE;   // Tracing all stations
int             FullTracing = NO,       // Full trace flag
		EndOfData = NO;         // EOF flag

/* ------------------------------------- */
/* Boundary conditions of the simulation */
/* ------------------------------------- */
#if	ZZ_NOC
Long    zz_max_NMessages = MAX_Long;    // Default limits and flags
#endif
TIME    zz_max_Time;                    // Everything is by default
#if	ZZ_NFP
Long    zz_max_cpuTime   = MAX_Long;    // unlimited
#else
double  zz_max_cpuTime   = HUGE;        // unlimited
#endif
int     zz_exit_code     = EXIT_user;   // Termination code

/* ----------------------- */
/* Program call parameters */
/* ----------------------- */
char    **zz_callArgs;
int     zz_callArgc;                    // Argument count

	// Original RNG seeds to be included in error messages; initialized
	// in a standard way (changeable by a call parameter)
Long    zz_OrgSeeds [ZZ_N_SEEDS] = {1777, 98733, 111329};

#if     ZZ_R48

	// Working seeds

unsigned  short  zz_Seeds [ZZ_N_SEEDS] [ 3 ];

#else
Long    zz_Seeds [ZZ_N_SEEDS];
#if ZZ_NFP
#else
	// A divisor for rnd. This is the minimum double number greater than
	// MAX_Long such that (double)MAX_Long divided by it is less than 1.0
double  zz_rndd;
#endif
#endif

/* ----- */
/* Flags */
/* ----- */
int
#if	ZZ_NOC
	// A flag used to tell whether message queues should be
	// emptied after zz_max_NMessages have been generated
        zz_flg_emptyMQueues = NO,

	// Tells whether the standard Client is to be started. Can be
	// switched off by a program call parameter.
	zz_flg_stdClient = YES,
#endif

	// Tells whether definitions of standard objects should be
	// automatically echoed to the output file
	zz_flg_printDefs = NO,

	// Tells whether information about system events should be
	// suppressed from exposures (which is the default)
	zz_flg_nosysdisp = YES,

	// Tells whether the definition of stations is complete and the
	// protocol (the first protocol process) has been started.
	zz_flg_started = NO;

        // File with DSD templates or NULL
char    *zz_TemplateFile = NULL;

#if	ZZ_NOC
#if     ZZ_QSL
	// The global limit on the number of messages to be queued at stations
	// awaiting transmission. When this number is reached new generated
	// messages are ignored.
Long    zz_QSLimit = MAX_Long;
#endif
#endif

/* ---------------- */
/* Event processing */
/* ---------------- */
ZZ_EVENT   *zz_pe = NULL,               // Priority event
	   *zz_c_wait_event,            // Currently built event
	   zz_fsent,                    // Front sentinel event
	   zz_rsent,                    // Rear sentinel event
	   *zz_sentinel_event;          // Pointer to the rear sentinel

ZZ_REQUEST *zz_pr = NULL,               // Priority request
	   *zz_orphans = NULL;		// To queue orphaned requests

Long    zz_npee = 0,                    // Number of pending events
	zz_npre = 0;                    // Number of processed events

LPointer zz_event_id;                   // Current event id

int     zz_c_first_wait = YES,          // Wait issued for the first time
#if	ZZ_NOL || ZZ_NOR
	zz_temp_event_id = NONE,        // Last port/xceiver event id
#endif
	zz_chckint;                     // Time limit check interval
					// Also used as checkpoint rqst flag

ZZ_REQUEST      *zz_c_other,            // Next wait request pointer
		*zz_c_whead,            // Built wait list head
		*zz_nqrqs;              // Dummy head for some request lists

#if  ZZ_TAG
ZZ_TBIG zz_c_wait_tmin;                 // Earliest request so far
#else
TIME    zz_c_wait_tmin;                 // Earliest request so far
#endif

AI      *zz_ai;                         // The current AI pointer

#if     ZZ_OBS
/* ------------------- */
/* Observers internals */
/* ------------------- */
Observer   *zz_obslist = NULL,          // The list of all observers
	   *zz_current_observer;        // The observer currently running

int        zz_observer_running = NO,    // YES, if an observer is running
	   zz_jump_flag = NO,           // Resume in progress
	   zz_state_to_jump;            // The observer's state to jump to

ZZ_INSPECT *zz_inlist_tail;             // The tail of the created inlist

#endif
#if     ZZ_TOL
/* -------------------------- */
/* Clock tolerance parameters */
/* -------------------------- */
int    zz_quality   = 0;                // The default clock quality (perfect)
double zz_tolerance = 0.0;              // The default clock tolerance (none)
#endif

/* ------------------ */
/* Process type hooks */
/* ------------------ */
int zz_Process_prcs,
    zz_KERNEL_prcs,
#if	ZZ_NOC
    zz_ClientService_prcs,
#endif
#if	ZZ_NOL
    zz_LinkService_prcs,
#endif
#if	ZZ_NOR
    zz_RosterService_prcs,
#endif
    zz_ObserverService_prcs;

/* ---------------------- */
/* Display (visible part) */
/* ---------------------- */
volatile int    DisplayActive = NO;     // Display activity flag

int     DisplayInterval = DEFDINTVL,    // Display interval
	DisplayOpening = NO,            // A brand new window
	DisplayClosing = NO;            // Closing the window

/* ------------------------ */
/* Display (invisible part) */
/* ------------------------ */

ZZ_WINDOW  *zz_windows = NULL,          // Active windows
	   *zz_the_window = NULL;       // The current window
ZZ_SIT     *zz_steplist = NULL;         // Stepped windows
ZZ_WSIT    *zz_steplist_delayed = NULL; // Waiting to be stepped
Long       zz_commit_event;             // Starting event number
TIME       zz_commit_time;              // Starting time
volatile Long zz_events_to_display = 0; // Delay to next display refresh

int	   zz_drequest = NO,            // Display requested by '-d'
	   zz_send_step_phrase = NO,    // Flag: stepped window sent
	   zz_dispfound;                // Flag: exmode found

/* ---------------- */
/* Standard streams */
/* ---------------- */
istream *zz_ifpp = NULL;                // Input stream
ostream *zz_ofpp = NULL;                // Output file

char    *zz_ifn = NULL,                 // Pointer to input filename
	*zz_ofn = NULL;                 // Pointer to output filename

#if     ZZ_REA || ZZ_RSY
Mailbox *zz_socket_list = NULL;		// List of connected mailboxes
int     zz_nfdesc;                      // Max number of file descriptors
ZZ_Journal *ZZ_Journals = NULL;         // Journal list
char	*ZZ_OffsetArg = NULL;           // Time offset argument
Long    zz_max_sleep_time;              // Minimum time to next journal event
#endif

/* ---------------------------------------------------------- */
/* The constant below specifies the maximum nesting of create */
/* for the builder. It  is never checked whether this maximum */
/* is not exceeded. It is assumed  that nobody sane will ever */
/* get close to this number.                                  */
/* ---------------------------------------------------------- */
#define MXOCLEVEL               16
/* --------------------------------------- */
/* A dummy object pointer (used by create) */
/* --------------------------------------- */
void    *zz_COBJ [MXOCLEVEL];
int     zz_clv = 0;

/* ----------------------------------------------------------------------- */
/* This is used as a stack for Stations specified with create (...) tn ... */
/* We make sure to resume the previous station after such a create. As for */
/* COBJ, no overflow is ever checked.                                      */
/* ----------------------------------------------------------------------- */
Station	*zz_CSTA [MXOCLEVEL];
int	zz_csv = 0;

/* ----------------- */
/* Smurph process id */
/* ----------------- */
int     zz_processId;

/* ------------------- */
/* Initialized statics */
/* ------------------- */
#if	ZZ_NOS
Long    RVariable::sernum = 0;
#endif

#if	ZZ_NOL
Long    Link::sernum = 0;
#endif

#if	ZZ_NOR
Long    RFChannel::sernum = 0;
#endif

#if	ZZ_NOC
Long    Traffic::sernum = 0;
#endif

Long    EObject::sernum = 0;
Long    Station::sernum = 0;
Long    Process::sernum = 0;
Long    Observer::sernum = 0;
Long	Mailbox::sernum = 0;
