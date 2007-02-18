/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

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
	cerr << "     -t [n] [s]      start tracing at n ITUs for station s\n";
	cerr << "     -f              full tracing (together with -t)\n";
	cerr << "     -o              echo topology def to output file\n";
#if	ZZ_NOC
	cerr << "     -c              empty message queues\n";
	cerr << "     -u              disable standard client\n";
#endif
	cerr << "     -s              system event info to be exposed\n";
        cerr << "     -T fn           template file name\n";
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
#ifdef	SIGFPE
	signal (SIGFPE , (SIGARG) sigfpx );
#endif
#ifdef  SIGBUS
	signal (SIGBUS , (SIGARG) sigbus );
#endif
#ifdef	SIGSEGV
	signal (SIGSEGV, (SIGARG) sigsegv);
#endif
#ifdef	SIGXCPU
	signal (SIGXCPU, (SIGARG) sigxcpu);
#endif
	if (signal (SIGINT, SIG_IGN) != SIG_IGN)
		signal (SIGINT, (SIGARG) sigint);
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
	fiptr << ">>> At virtual time          = " << tcp << '\n';
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

void    zz_ierror (char *t) {

/* ---------------------------------------------------------- */
/* Signals  a  fatal  error (that makes it impossible to even */
/* start the simulator) and aborts smurph                     */
/* ---------------------------------------------------------- */

	cerr << *zz_callArgs << ": " << t << '\n';
	exit (20);
}

void    zz_init_system  (int argc, char *argv []) {

/* ---------------------------------------------------------- */
/* Inteprets  parameters  from  the  call  line,  initializes */
/* things                                                     */
/* ---------------------------------------------------------- */

	int     i, k;
	Long    l;
        char    param;

	zz_callArgs = argv;
	zz_callArgc = argc;

	Time = TIME_0;
	
	zz_processId = (int) getpid ();

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

				if (argc && (**argv >= '0') &&
					(**argv <= '9')) {

					for (k = 0, TracingTime = TIME_0;
						((*argv)[k] >= '0') &&
							((*argv)[k] <= '9');
								k++)

						TracingTime = TracingTime
						      * 10 + ((*argv)[k] - '0');

					if ((*argv)[k] != '\0')
						bad_arguments ();

					argv++;
					argc--;
				} else {
					TracingTime = TIME_0;
				}
				
				if (argc && (**argv >= '0') &&
					(**argv <= '9')) {

					for (k = 0, TracedStation = 0;
						((*argv)[k] >= '0') &&
							((*argv)[k] <= '9');
								k++)

						TracedStation = TracedStation
						      * 10 + ((*argv)[k] - '0');

					if ((*argv)[k] != '\0')
						bad_arguments ();

					argv++;
					argc--;
				}

				break;

                        case 'T' : // Template file name

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
			case 'f' : // Full trace (links dumped)

				if (undef (TracingTime)) bad_arguments ();
				FullTracing = YES;
				break;

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

	if ((zz_ofn == NULL) || ((zz_ofn[0] == '.') && (zz_ofn[1] == '\0'))) {
		// Writing to standard output
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

int excptn  (char *sig, ...) {

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

int zz_aerror (char *t) {

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
        Long    atime;

	atime = getEffectiveTimeOfDay ();
	for (wt = ct = ctime ((time_t*)(&atime)); *ct != '\0'; ct++)
		if (*ct == '\n') {
			*ct = '\0';
			break;
		}
	return (wt);
}
