/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

/* --- */

#include        "system.h"
#include	"cdebug.h"

#define  openIStream(f)   zz_openIStream (f)

/* ---------------- */
/* System interface */
/* ---------------- */

char            ofnambuf [PATH_MAX];          // Output filename buffer

static  struct  tms     etimes;

Long                    times   ();

	// For properly calculating CPU execution time, also when smurph
	// is checkpointed
#if	ZZ_NFP
static  Long            base_time,
			time_so_far = 0;
#else
static  double          base_time,
			time_so_far = 0.0;
#endif

// ============================================================================

// Delayed initializers triggered by program call arguments - to be run when
// the network becomes "built".

class	DelInit {

	DelInit *next;
	void *Data;
	void (*Action)(void*);

	public:

	DelInit (void (*) (void*), void*);

	~DelInit ();
};

static	DelInit	*DelList = NULL;

DelInit::DelInit (void (*act) (void*), void *data) {

	Action = act;
	Data = data;

	next = DelList;
	DelList = this;
}

DelInit::~DelInit () {

	// Carry out the action
	(*Action) (Data);

	if (Data)
		delete Data;

	DelList = next;
}

void zz_run_delayed_initializers () {

	while (DelList)
		delete DelList;
}

typedef struct {

	Boolean	inEtu, Term, Full;

	Long *Stats;
	Long NStats;

	// If TIME is a structure, union doesn't like us; so we have to waste
	// some memory (the two are alternatives), but only before network
	// build
	TIME Start_t;
	double Start_d;

	// These two are alternatives, too
	TIME Stop_t;
	double Stop_d;

} del_trace_init_block;

static void del_init_tracing (void *d) {

	del_trace_init_block *D = (del_trace_init_block*) d;

	if (D->inEtu) {
		TracingTimeStart = etuToItu (D->Start_d);
		TracingTimeStop = etuToItu (D->Stop_d);
	} else {
		TracingTimeStart = D->Start_t;
		TracingTimeStop = D->Stop_t;
	}

	if (TracingStations)
		delete [] TracingStations;

	TracingStations = D->Stats;
	NTracingStations = D->NStats;

	if (D->Term) {
		// Stop on TracingTimeStop
		if (TracingTimeStop < zz_max_Time)
			zz_max_Time = TracingTimeStop;
		setFlag (zz_trace_options, TRACE_OPTION_TIME_LIMIT_SET);
	}
}

#if ZZ_DBG

static void del_debug_tracing (void *d) {

	del_trace_init_block *D = (del_trace_init_block*) d;

	if (D->inEtu) {
		DebugTracingTimeStart = etuToItu (D->Start_d);
		DebugTracingTimeStop = etuToItu (D->Stop_d);
	} else {
		DebugTracingTimeStart = D->Start_t;
		DebugTracingTimeStop = D->Stop_t;
	}

	if (DebugTracingStations)
		delete [] DebugTracingStations;

	DebugTracingStations = D->Stats;
	DebugNTracingStations = D->NStats;

	if (D->Term) {
		// Stop on TracingTimeStop
		if (DebugTracingTimeStop < zz_max_Time)
			zz_max_Time = DebugTracingTimeStop;
		setFlag (zz_trace_options, TRACE_OPTION_TIME_LIMIT_SET);
	}

	DebugTracingFull = D->Full;
}

#endif
		
// ============================================================================

static  int             ConMsg = NO;    // Console message flag

/* -------------------------------------- */
/* Signal servicing function (prototypes) */
/* -------------------------------------- */
static void  sigill (), sigfpx (), sigbus (), sigsegv (), sigint ();
static void sigxcpu ();

	// Note: non-standard memory allocator has been removed. Standard
	//       malloc will be used for the time being until I get around to
	//       program a new version

static  int     ecounter = 0;// Exception counter -- to avoid cascade exceptions

void    zz_closeMonitor ();

static void bad_arguments () {

/* ---------------------------------------------------------- */
/* Prints  info  about expected call arguments and aborts the */
/* program                                                    */
/* ---------------------------------------------------------- */

	cerr << "Usage:  " << *zz_callArgs << " 'options' 'files'\n";
	cerr << "    Options:\n";
	cerr << "     -r [n [n [n]]]  setting rnd seeds\n";
	cerr << "     -d              hang until display server connected\n";
	cerr << "     -t frm-to/s     trace on from frm until to for stat s\n";
#if ZZ_DBG
	cerr << "     -T frm-to/s[+]  debug trace on\n";
#endif
	cerr << "     -o              echo topology def to output file\n";
#if	ZZ_NOC
	cerr << "     -c              empty message queues\n";
	cerr << "     -u              disable standard client\n";
#endif
	cerr << "     -b [0|1]        run in background (as a daemon)\n";
	cerr << "     -e              implicit process termination illegal\n";
	cerr << "     -s              system event info to be exposed\n";
        cerr << "     -M fn           template file name\n";
#if     ZZ_TOL
	cerr << "     -k t q          define clock tolerance and quality\n";
#endif
#if     JOU
        cerr << "     -J mname        journal the specified mailbox\n";
        cerr << "     -I mname        input from journal input\n";
        cerr << "     -O mname        input from journal output\n";
        cerr << "     -D yy/mm/dd,hh:mm:ss\n";
        cerr << "     -D mm/dd,hh:mm:ss\n";
        cerr << "     -D dd,hh:mm:ss\n";
        cerr << "     -D hh:mm:ss\n";
        cerr << "     -D mm:ss\n";
        cerr << "     -D ss\n";
        cerr << "     -D yy/mm/dd\n";
        cerr << "     -D mm/dd        set effective date/time\n";
        cerr << "     -D J            set date/time from journal\n";
	cerr << "     -- model args\n";
#endif
// FIXME: add setResync parameter
	cerr << "    Files: [inp [out]]\n";
	cerr << "     inp             input file (missing or . -> stdin)\n";
	cerr << "     out             output file (missing or . -> stdout)\n";

	exit (21);
}

static void signal_service () {

/* ----------------------- */
/* Declares signal service */
/* ----------------------- */

#if ZZ_DBG < 2

#ifdef	SIGHUP
	signal (SIGHUP , (SIGARG) sigint );
#endif
#ifdef	SIGILL
	signal (SIGILL , (SIGARG) sigill );
#endif
#ifdef	SIGIOT
	signal (SIGIOT , (SIGARG) sigill );
#endif
#ifdef  SIGEMT
	signal (SIGEMT , (SIGARG) sigill );
#endif
#ifdef  SIGSYS
	signal (SIGSYS , (SIGARG) sigill );
#endif
#ifdef  SIGEMT
	signal (SIGEMT , (SIGARG) sigill );
#endif
#ifdef	SIGFPE
	signal (SIGFPE , (SIGARG) sigfpx );
#endif
#ifdef  SIGBUS
	signal (SIGBUS , (SIGARG) sigbus );
#endif
#ifdef	SIGABRT
	signal (SIGABRT, (SIGARG) sigsegv);
#endif
#ifdef	SIGSEGV
	signal (SIGSEGV, (SIGARG) sigsegv);
#endif
#ifdef	SIGXCPU
	signal (SIGXCPU, (SIGARG) sigxcpu);
#endif
	if (signal (SIGINT, SIG_IGN) != SIG_IGN)
		signal (SIGINT, (SIGARG) sigint);

#endif /* ZZ_DBG */

signal (SIGPIPE, SIG_IGN);

}

static void outmes (ostream &fiptr, char *txt) {

/* --------------------- */
/* Outputs error message */
/* --------------------- */

	int     k;
	char    tc [47], *tcp;

	btoa (Time, tc, 46);
	for (tcp = tc; *tcp == ' '; tcp++);

	fiptr << ">>> " << txt << '\n';
	fiptr << ">>> At virtual time          = " << tcp;
	if (Etu != 1.0)
		fiptr << form (" [%12.9f]", ituToEtu (Time));
	fiptr << '\n';
	fiptr << ">>> Input data file name     = ";
	if ((zz_ifn == NULL) || ((zz_ifn[0] == '.') && (zz_ifn[1] == '\0')))
		fiptr << "STDIN";
	else
		fiptr << zz_ifn;
	fiptr << '\n';
	fiptr << ">>> Rnd initialization seeds = ";
	for (k=0; k < N_SEEDS; k++) fiptr << zz_OrgSeeds [k] << ' ';
	fiptr << '\n';
	zz_print_event_info (fiptr);
}

void    zz_ierror (const char *t) {

/* ---------------------------------------------------------- */
/* Signals  a  fatal  error (that makes it impossible to even */
/* start the simulator) and aborts smurph                     */
/* ---------------------------------------------------------- */

	cerr << *zz_callArgs << ": " << t << '\n';
	exit (20);
}

static Boolean decode_tracing_arg (int argc, char *argv [],
						del_trace_init_block *dtib) {

// Decodes the trace parameters argument for 't' and 'T'

	int i, j;
	char *ap, *ep;
	char numc [2][65];

	dtib->Stats = NULL;
	dtib->NStats = 0;

	dtib->Full = dtib->inEtu = dtib->Term = NO;
	dtib->Start_t = TIME_0;
	dtib->Stop_t = TIME_inf;
	if (argc == 0 ||
	    (**argv != '/' && **argv != '+' && !isdigit (**argv))) {
		// The defaults
		return NO;
	}

	// We have something that looks like an argument
	ap = *argv;
	i = 0;
	do {
		// Decode the numbers
		for (j = 0; ; j++) {
			if (isdigit (*ap)) {
				if (j == 64)
					bad_arguments ();
				numc [i][j] = *ap++;
				continue;
			}
			if (*ap != '.')
				break;
			dtib->inEtu = YES;
			if (j == 64)
				bad_arguments ();
			numc [i][j] = *ap++;
		}
		if (j == 0)
			break;
		numc [i][j] = '\0';
		i++;
		if (i == 2 || *ap != '-')
			break;
		ap++;

	} while (1);

	if (i) {
		// There are numbers at all, at least one
		if (dtib->inEtu) {
			dtib->Start_d = strtod (numc [0], &ep);
			if (ep != numc [0] + strlen (numc [0]))
				bad_arguments ();
			// Make sure the second default in Etu looks right
			dtib->Stop_d = Time_inf;
		} else {
			// TIME
			dtib->Start_t = TIME_0;
			for (ep = numc [0]; *ep != '\0'; ep++)
				dtib->Start_t = dtib->Start_t * 10 +
					(LONG)(*ep - '0');
		}
		if (i == 2) {
			// Second one
			if (dtib->inEtu) {
				dtib->Stop_d = strtod (numc [1], &ep);
				if (ep != numc [1] + strlen (numc [1]))
					bad_arguments ();
			} else {
				// TIME
				dtib->Stop_t = TIME_0;
				for (ep = numc [1]; *ep != '\0'; ep++)
					dtib->Stop_t = dtib->Stop_t * 10 +
						(LONG)(*ep - '0');
			}
		}
	}

	// Done with the numbers

	if (*ap == '/') {
		// Station(s): calculate how many
		for (i = 1, ep = ++ap; isdigit (*ep) || *ep == ','; ep++)
			if (*ep == ',')
				i++;
		dtib->Stats = new Long [dtib->NStats = i];
		i = 0;
		while (1) {
			if (!isdigit(*ap))
				break;
			for (dtib->Stats [i] = 0; isdigit (*ap); ap++)
				dtib->Stats [i] = dtib->Stats [i] * 10 +
					(*ap - '0');
			if (*ap == ',')
				ap++;
			i++;
		}
	}

	// Trailing flags ('+' and/or 'q')
	while (*ap != '\0') {
		if (*ap == '+')
			dtib->Full = YES;
		else if (*ap == 'q' || *ap == 's')
			dtib->Term = YES;
		else
			bad_arguments ();
		ap++;
	}

	return YES;
}

void    zz_init_system  (int argc, char *argv []) {

/* ---------------------------------------------------------- */
/* Inteprets  parameters  from  the  call  line,  initializes */
/* things                                                     */
/* ---------------------------------------------------------- */

	int     		i, k, b;
	Long    		l;
	del_trace_init_block	*dtib;
        char    		param;

	zz_callArgs = argv;
	zz_callArgc = argc;

	Time = TIME_0;
	
	zz_processId = (int) getpid ();

	b = -1;				// Background flag

#if  ZZ_REA || ZZ_RSY
	zz_nfdesc = getdtablesize ();
#endif
	// Get filenames and other call parameters

	for (argc--, argv++; argc;) {

		if (**argv != '-') {
			if (zz_ifn == NULL) {
				// Input file name
				zz_ifn = *argv;
			} else {
				if (zz_ofn == NULL) {
					// Output file name; copy it to the
					// auxiliary buffer for checkpointing.
					// Note that argv will be destroyed at
					// restart.

					strcpy (ofnambuf, *argv);
					zz_ofn = ofnambuf;

				} else {
					bad_arguments ();
				}
			}

			argv++;
			argc--;
			continue;
		}

		argc--;

		if ((*argv)[2] != '\0') bad_arguments ();

		switch (param = (*(argv++))[1]) {

			case '-' : // End of SMURPH args

				goto EOA;

			case 'r' : // Random seeds

				for (i = 0; argc; i++) {

					if ((**argv < '0') || (**argv > '9'))
						break;

					for (l = k = 0; ((*argv)[k] >= '0') &&
						((*argv)[k] <= '9'); k++)

						l = l * 10 + (*argv)[k] - '0';

					if ((*argv)[k] != '\0')
						bad_arguments ();

					if (i < N_SEEDS)
						zz_OrgSeeds [i] = l;

					argv++;
					argc--;
				}

				if (i == 0) bad_arguments ();

				break;

			// Note: remote simulation options have been deleted

			case 'd' : // Display

				zz_drequest = YES;
				break;

			// Note: the 'h' option has been removed. Halting
			// will be taken care of by the display server

			case 't' : // Tracing

				// The argument may look like this:
				// -t starttime-stoptime/st, with shortcuts
				// -t starttime/st
				// -t starttime
				// -t 
				// -t /st

				dtib = new del_trace_init_block;
				if (decode_tracing_arg (argc, argv, dtib)) {
					argv++;
					argc--;
				}
				new DelInit (del_init_tracing, dtib);
				break;
#if ZZ_DBG
			case 'T' : // Debug tracing

				dtib = new del_trace_init_block;
				if (decode_tracing_arg (argc, argv, dtib)) {
					argv++;
					argc--;
				}
				new DelInit (del_debug_tracing, dtib);
				break;
#endif
                        case 'M' : // Template file name

				if (argc) {
                                  if (zz_TemplateFile) bad_arguments ();
                                  zz_TemplateFile = *argv++;
                                  argc--;
                                } else
                                  bad_arguments ();
                                break;
#if JOU
			case 'D' : // Set effective date/time

				if (!argc) bad_arguments ();
                                ZZ_OffsetArg = *argv;
                                argc--;
                                argv++;
                                break;

                        case 'J' : // Journal declaration
                        case 'I' : // Input from journal input
                        case 'O' : // Input from journal output

                                if (argc) {
                                  ZZ_Journal *jn =
                                                  new ZZ_Journal (*argv, param);
                                  argc--;
                                  argv++;
                                } else
                                  bad_arguments ();
                                break;
#endif
			case 'o' : // Echo definitions to the output file

				zz_flg_printDefs = YES;
				break;
#if	ZZ_NOC
			case 'c' : // Empty message queues

				zz_flg_emptyMQueues = YES;
				break;

			case 'u' : // Do not start standard Client

				zz_flg_stdClient = NO;
				break;
#endif	/* NOC */

			case 's' : // System event info to be exposed

				zz_flg_nosysdisp = NO;
				break;

			case 'e' : // Implicit process termination illegal

				zz_flg_impterm = NO;
				break;

			case 'b' : // Background, i.e., run as daemon

				if (b >= 0)
					bad_arguments ();

				if (argc < 2 || ((*argv)[0] != '0' &&
						 (*argv)[0] != '1')) {
					b = 1;
					break;
				}

				b = ((*argv) [0] != '0');

				argv++;
				argc--;

				break;
#if     ZZ_TOL
			case 'k' : // Standard tolerance parameters

				if (argc < 2) bad_arguments ();
				zz_tolerance = atof (*argv);
				if (zz_tolerance < 0.0 || zz_tolerance > 1.0)
							       bad_arguments ();
				argv++;
				argc--;

				for (l = k = 0; ((*argv)[k] >= '0') &&
						((*argv)[k] <= '9'); k++)

					l = l * 10 + (*argv)[k] - '0';

				if ((*argv)[k] != '\0') bad_arguments ();
				zz_quality = (int) l;
				if (zz_quality < 0 || zz_quality > 10)
					bad_arguments ();
				break;
#endif

			default: bad_arguments ();
		}
	}

EOA:	// Arguments visible to the model

	PCArgc = argc;
	PCArgv = (const char**) argv;

	// Initialize actual seeds
#if     ZZ_R48
	for (i = 0; i < N_SEEDS; i++) {
		zz_Seeds [i] [0] = (short) (zz_OrgSeeds [i] & 0177777);
		zz_Seeds [i] [1] = (short) ((zz_OrgSeeds [i] >> 16) & 0177777);
		zz_Seeds [i] [2] = zz_Seeds [i] [0];
	}
	srand48 (1);
#else
	for (i = 0; i < N_SEEDS; i++)
		zz_Seeds [i] = zz_OrgSeeds [i];
#endif
	// Open input file

	if ((zz_ifn == NULL) || ((zz_ifn[0] == '.') && (zz_ifn[1] == '\0'))) {
		// Reading from standard input
		zz_ifpp = &cin;
	} else {
		zz_ifpp = openIStream (zz_ifn);
		if (zz_ifpp == NULL)
			ierror ("cannot open input file");
	}

	// Output file

	if ((zz_ofn == NULL) ||
	    ((zz_ofn [0] == '.' || zz_ofn [0] == '+') &&
	     (zz_ofn [1] == '\0'))) {
		// Writing to standard output or standard error
		if (zz_ofn != NULL && zz_ofn [0] == '+')
			zz_ofpp = &cerr;
		else
			zz_ofpp = &cout;
	} else {
		i = (int) umask (OMASK);
		zz_ofpp = openOStream (zz_ofn, "w");
		umask (i);
		if (zz_ofpp == NULL)
			ierror ("cannot open output file");
	}

	// Declare signal service

	signal_service ();
	times (&etimes);
#if	ZZ_NFP
	base_time = etimes.tms_utime / ZZ_HZ;
#else
	base_time = (double) etimes.tms_utime / ZZ_HZ;
#endif
	signal (SIGCHKP, SIG_IGN);

	if (b >= 0)
		// Run as a daemon
		daemon (1, b);
}

void    zz_done () {

/* -------------------------------- */
/* Actual unconditional termination */
/* -------------------------------- */

	zz_dterminate ();  // Notify DSD
	if (zz_ofpp != NULL) {
		Ouf << "\n@@@ End of output\n";
		Ouf.flush ();
	}
	exit (0);
}

int excptn  (const char *sig, ...) {

/* ---------------- */
/* Abort simulation */
/* ---------------- */

	va_list pmts;
	char *mess;


	if (zz_ofpp != NULL) Ouf.flush ();

	if (ecounter++ > 1) {
		// Avoid cascade errors
		exit (3);
	}

	va_start (pmts, sig);

	mess = vform (sig, pmts);

#if ZZ_DBG > 1
	// Force abort on SEGV, so we can be caught in gdb
	write (2, mess, strlen (mess));
	write (2, "\n", 1);
	EXCPTN;
#else

	if (DisplayActive)
		displayNote (form ("*%s", mess));

#ifdef	SIGTSTP
	signal (SIGTSTP, SIG_DFL);
#endif
	outmes (cerr, mess);
	if (zz_ofpp != NULL) {
		outmes (Ouf, mess);
		Ouf << '\n';
	}
	zz_exit_code = EXIT_abort;
	if (ecounter > 1) zz_done ();   // Do not return on second error
	// Set up the abort flag
	Kernel->terminate ();
	return (0);

#endif	/* ZZ_GDB <= 1 */

}

/* ------------------------ */
/* Signal service functions */
/* ------------------------ */

static  void    sigill () {             // Service illegal instruction

#ifdef	SIGILL
	signal (SIGILL , SIG_DFL );
#endif
#ifdef	SIGIOT
	signal (SIGIOT , SIG_DFL );
#endif
#ifdef SIGEMT
	signal (SIGEMT , SIG_DFL );
#endif
#ifdef  SIGSYS
	signal (SIGSYS , SIG_DFL );
#endif
	excptn ("illegal machine instruction");
}

#if     ZZ_AER

int zz_aerror (const char *t) {

/* ------------------------------------ */
/* Print out arithmetic error diagnosis */
/* ------------------------------------ */

	return (excptn ("BIG arithmetic error: %s", t));
}

#endif

static  void    sigfpx () {             // Service floating point exception

	signal (SIGFPE , SIG_DFL);
	excptn ("floating point exception");
}

#ifdef  SIGBUS
static  void    sigbus () {             // Service bus error

	signal (SIGBUS , SIG_DFL);
	excptn ("bus error");
}
#endif

static  void    sigsegv () {            // Service segmentation violation

	signal (SIGSEGV, SIG_DFL);
	excptn ("segmentation violation");
}

#ifdef	SIGXCPU
static  void    sigxcpu () {            // CPU Time limit

	signal (SIGXCPU, SIG_DFL);
	excptn ("CPU Time limit");
}
#endif

static  void    sigint  () {            // User kill

	excptn ("Simulation aborted");
}

#if	ZZ_NFP
Long	cpuTime () {
#else
double  cpuTime () {
#endif

/* ------------------------------------------ */
/* Give the program execution time in seconds */
/* ------------------------------------------ */

	times (&etimes);
#if	ZZ_NFP
	return (etimes.tms_utime / ZZ_HZ - base_time + time_so_far);
#else
	return ((double) etimes.tms_utime / ZZ_HZ - base_time + time_so_far);
#endif
}

char    *tDate () {

/* ------------------------------------ */
/* Return date and time in textual form */
/* ------------------------------------ */

	char    *ct, *wt;
        time_t  atime;

	atime = getEffectiveTimeOfDay ();
	for (wt = ct = ctime (&atime); *ct != '\0'; ct++)
		if (*ct == '\n') {
			*ct = '\0';
			break;
		}
	return (wt);
}
