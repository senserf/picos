/* ooooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-2016   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ----------------------------------------------------- */
/* This is a simple interactive setup program for SMURPH */
/* ----------------------------------------------------- */

#include        "version.h"

#include        <sys/param.h>
#include	<sys/stat.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include	<fcntl.h>
#include        <netinet/in.h>
#include	<netdb.h>
#include        <ctype.h>
#include	<string.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>

#include        "defaults.h"

#include	<iostream>
#include	<fstream>

#define	BFFSIZE		(1024*16)
#define	MAXINCDIRS	8

#define	FLUSH

char    is [BFFSIZE],
	cb [BFFSIZE];

char    wd [MAXPATHLEN];

char    *incdir [MAXINCDIRS], *xncdir [MAXINCDIRS], *ccomp, *ckomp, *jcomp,
	*srcdir, *libdir,
	*monhost, *mkname, *mksdir, *sokdir;
int     maxlib, monport, batch, nxncdirs;

#define	HOME getenv ("HOME")
#define notabsolute(s)   ((s)[0] != '/')
#define	getwd(a) getcwd (a, MAXPATHLEN)

using std::cin;
using std::cout;
using std::cerr;
using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;

ostream	*openOStream (const char *fn) {

	return (new ofstream (fn));

}

istream	*openIStream (const char *fn) {

	return (new ifstream (fn));
}

void    getstring () {

/* ----------------------------------------------- */
/* Reads a string of characters from 'cin' to 'is' */
/* ----------------------------------------------- */

	int     i;
	char    c;

	if (cin.eof ()) {

		// Keep returning empty string on eof
		is [0] = '\0';
		return;
	}

	while (1) {

		cin.get (c);
		if (!isspace (c)) break;
		if (c == '\n') {
			is [0] = '\0';
			return;
		}
	}

	for (is [0] = c, i = 1; i < 8192; i++) {

		cin.get (c);
		if (isspace (c)) break;
		is [i] = c;
	}

	is [i] = '\0';

	while (c != '\n' && ! cin.eof ()) cin.get (c);
}

void	encint (int v) {

/* ---------------------------------------------- */
/* Encodes a nonnegative integer number into 'is' */
/* ---------------------------------------------- */

	char  code [16];
	int   n;

	for (n = 14; ; n--) {
		code [n] = (v % 10) + '0';
		if ((v = v / 10) == 0) break;
	}
	code [15] = '\0';
	strcpy (is, & (code [n]));
}

void bad_usage (const char *pn) {

	fprintf (stderr, "Usage: %s [-b] [-r tag]\n", pn);
	exit (99);
}
	
int	main (int argc, const char *argv []) {

	char    c, *s;
	const char *pname, *rtag;
	istream *vfi;
	ostream	*vfo;
	int	i, m;

	pname = argv [0];
	rtag = NULL;

	while (1) {
		argc--;
		argv++;
		if (argc <= 0)
			break;
		if (strcmp (*argv, "-b") == 0) {
			if (batch)
				bad_usage (pname);
			batch = 1;
			continue;
		}
		if (strcmp (*argv, "-r") == 0) {
			if (argc < 1 || rtag != NULL)
				bad_usage (pname);
			argc--;
			argv++;
			rtag = *argv;
		}
	}

if (!batch) {
cout << "Hi, this is SMURPH/SIDE Version " << VERSION << '\n';
cout << "You will be asked a few simple questions.  By  hitting  RETURN you\n";
cout << "select the default answer indicated in the question.  Note that in\n";
cout << "the vast majority of cases the default answers are fine.\n\n";
}
	ccomp = new char [strlen (DEFCCOMP) + 1];
	ckomp = new char [strlen (DEFCKOMP) + 1];
	strcpy (ccomp, DEFCCOMP);
	strcpy (ckomp, DEFCKOMP);

if (!batch) {
cout << '\n';
cout << "If you want to recompile the DSD applet,  please  specify  now the\n";
cout << "name of your Java compiler.  If you accept the default (none), DSD\n";
cout << "will not be recompiled.  This usually makes perfect sense, because\n";
cout << "Java code is supposed to be platform-independent.\n\n";
cout << "So what is the name of your Java compiler? (" << DEFJCOMP << "): ";
}
	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << DEFJCOMP << '\n';
		strcpy (is, DEFJCOMP);
	} else
		cout << "The name of your Java compiler is " << is << '\n';

	jcomp = new char [strlen (is) + 1];
	strcpy (jcomp, is);

if (!batch) {
cout << '\n';
cout << "Give me the path to  SMURPH/SIDE  sources (at least from your home\n";
cout << "directory).  If the path starts with '/', it will be assumed to be\n";
cout << "absolute;  otherwise, it will be interpreted relative to your home\n";
cout << "directory. The default path is:\n\n";
}

	getwd (wd);

if (!batch) {
	cout << "        " << wd << "\n\n";

cout << "which is the current working directory.\n";
}

SP_RETRY:

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << wd << '\n';
		strcpy (is, wd);
	} else {
		// Check if the directory is reachable
		if (notabsolute (is)) {
			// Turn it into $HOME/is
			if ((s = HOME) == NULL) {
				if (batch) {
NoHome:
					cerr << "Cannot determine HOME\n";
					exit (1);
				}
				cout << "Cannot determine your home"
					    "directory;"
					    " please specify the full path\n";
				goto SP_RETRY;
			}
			strcpy (is+4096, is);
			strcpy (is, s);
			strcat (is, "/");
			strcat (is, is+4096);
		}

		// Execute a tentative chdir there

		if (chdir (is) < 0) {
			if (batch) {
DirAcc:
				cerr << "Directory " << is <<
					" is inaccessible\n";
				exit (1);
			}
			cout << "This directory is inaccessible;" <<
				" please specify another path\n";
			goto SP_RETRY;
		}
		// Move back to where you should be
		chdir (wd);
	}

	srcdir = new char [strlen (is) + 1];
	strcpy (srcdir, is);

if (!batch) {
cout << '\n';
cout << "Please give the path to the  directory  where  you  want  to  keep\n";
cout << "binary libraries.  This directory need not exist at present; if it\n";
cout << "exists, however, IT WILL BE CLEARED. The default path is:\n\n";
}

	for (s = wd + strlen (wd); s != wd && *(s-1) != '/'; s--);
	*s = '\0';
	strcat (wd, DEFLIBDR);

if (!batch) {
	cout << "        " << wd << "\n\n";

cout << "which is equivalent to: ../" << DEFLIBDR << '\n';
}

LB_RETRY:

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << wd << '\n';
		strcpy (is, wd);
	} else {
		if (notabsolute (is)) {
			// Turn it into $HOME/is
			if ((s = HOME) == NULL) {
				if (batch)
					goto NoHome;
				cout << "Cannot determine your home directory;"
					<< " please specify the full path\n";
				goto LB_RETRY;
			}
			strcpy (is+4096, is);
			strcpy (is, s);
			strcat (is, "/");
			strcat (is, is+4096);
		}
	}

	// Check if the directory exists
	getwd (wd);             // Save current wd
LB_CD:
	if (chdir (is) < 0) {

		// Assume that it does not exist and try to create it

		for (s = is+1; *s != '\0'; s++) {
			if (*s == '/' && *(s+1) != '\0') {
				c = *s;
				*s = '\0';
				mkdir (is, 0777);
				*s = c;
			}
		}

		if (mkdir (is, 0777) < 0) {
			if (batch)
				goto DirAcc;
			cout << "I have problems accessing this";
			cout << " directory; please speify another";
			cout << " path\n";
			goto LB_RETRY;
		}

	} else {
		chdir (wd);     // Move back
	}

	strcpy (cb, "rm -fr ");
	strcat (cb, is);
	strcat (cb, "/l* 2>/dev/null");
	system (cb);

	libdir = new char [strlen (is) + 1];
	strcpy (libdir, is);

if (!batch) {
cout << '\n';
cout << "Specify the maximum number of versions of the binary library to be\n";
cout << "kept simultaneously.  Whenever the configuration of options of mks\n";
cout << "requests a new library version to  be  created,  and  the  current\n";
cout << "number  of  library  versions  is  equal to the maximum, the least\n";
cout << "recently used library will be removed. The default limit is " <<
	DEFMAXLB << '\n';
}

ML_RETRY:

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << (maxlib = DEFMAXLB) << " libraries\n";
	} else {
		for (s = is; *s != '\0' && isdigit (*s); s++);
		if (*s != '\0') {
			if (batch) {
IllNum:
				cerr << "Illegal number " << is << "\n";
				exit (1);
			}
			cout << "This is not a legal number; try again\n";
			goto ML_RETRY;
		}
		maxlib = atoi (is);
		cout << "The maximum number of library versions is " <<
			maxlib << '\n';
	}

if (!batch) {
cout << '\n';
cout << "Now,  please enter the list of paths to source 'include' libraries\n";
cout << "(which can be absolute or relative to your home directory),   each\n";
cout << "path in a separate line. Enter an empty line when done. This path:\n";
cout << "\n";
}

	getwd (wd);
        for (s = wd + strlen (wd); s != wd && *(s-1) != '/'; s--);
        *s = '\0';
        strcat (wd, DEFINDIR);
if (!batch) {
        cout << "        " << wd << "\n\n";

cout << "is standard and need not be specified.  If you want to exclude the\n";
cout << "standard path, enter '-' as the only character of an input line.\n";
}
	// Put in the default
	incdir [0] = new char [strlen (wd) + 1];
	strcpy (incdir [0], wd);

	m = 1;

IN_RETRY:

	getstring ();

	if (is [0] == '\0') {
		i = m;
		if (incdir [0] == NULL)
			i--;
		if (i) {
			if (i == 1)
				cout << "One";
			else
				cout << i;
		} else {
			cout << "No";
		}
		cout << " include librar";
		cout << ((i == 1) ? "y" : "ies");
		cout << '\n';
		goto IN_DONE;
	}

	if (is [0] == '-' && is [1] == '\0') {
		// Exclude the standard library
		if (incdir [0] == NULL) {
			if (!batch)
				cout << "Standard library already excluded!\n";
		} else {
			cout << "Excluding standard library\n";
			delete incdir [0];
			incdir [0] = NULL;
		}
		goto IN_RETRY;
	}

	if (m == MAXINCDIRS) {
		if (batch) {
			cerr << "Too many libraries\n";
			exit (1);
		}
		cout << "Too many libraries!\n";
		goto IN_RETRY;
	}

	if (notabsolute (is)) {
		// Turn it into $HOME/is
		if ((s = HOME) == NULL) {
			if (batch)
				goto NoHome;
			cout << "Cannot determine your home directory;"
				<< " please specify the full path\n";
			goto IN_RETRY;
		}
		strcpy (is+4096, is);
		strcpy (is, s);
		strcat (is, "/");
		strcat (is, is+4096);
	}

	// Check if the directory exists
	getwd (wd);             // Save current wd
IN_CD:
	if (chdir (is) < 0) {
		// Assume that it does not exist and try to create it
		for (s = is+1; *s != '\0'; s++) {
			if (*s == '/' && *(s+1) != '\0') {
				c = *s;
				*s = '\0';
				mkdir (is, 0777);
				*s = c;
			}
		}

		if (mkdir (is, 0777) < 0) {
			if (batch)
				goto DirAcc;
			cout << "I have problems accessing this";
			cout << " directory; please specify another";
			cout << " path\n";
			goto IN_RETRY;
		}

	} else {
		chdir (wd);     // Move back
	}

	incdir [m] = new char [strlen (is) + 1];
	strcpy (incdir [m], is);
	m++;
	goto IN_RETRY;

IN_DONE:

if (!batch) {
cout << '\n';
cout << "Now,  please enter the list of paths to data 'include'  libraries,\n";
cout << "i.e.,  the directories that will be searched for <include>s in XML\n";
cout << "data. Each path must be specified in a separate line with an empty\n";
cout << "line ending the sequence. There is no default.\n";
}
	nxncdirs = 0;

XN_RETRY:

	getstring ();

	if (is [0] == '\0') {
		if (nxncdirs) {
			if (nxncdirs == 1)
				cout << "One";
			else
				cout << nxncdirs;
		} else {
			cout << "No";
		}
		cout << " data include librar";
		cout << ((nxncdirs == 1) ? "y" : "ies");
		cout << '\n';
		goto XN_DONE;
	}

	if (nxncdirs == MAXINCDIRS) {
		if (batch) {
			cerr << "Too many libraries\n";
			exit (1);
		}
		cout << "Too many libraries!\n";
		goto IN_RETRY;
	}

	if (notabsolute (is)) {
		// Turn it into $HOME/is
		if ((s = HOME) == NULL) {
			if (batch)
				goto NoHome;
			cout << "Cannot determine your home directory;"
				<< " please specify the full path\n";
			goto XN_RETRY;
		}
		strcpy (is+4096, is);
		strcpy (is, s);
		strcat (is, "/");
		strcat (is, is+4096);
	}

	// Check if the directory exists
	getwd (wd);             // Save current wd
XN_CD:
	if (chdir (is) < 0) {
		// Assume that it does not exist and try to create it
		for (s = is+1; *s != '\0'; s++) {
			if (*s == '/' && *(s+1) != '\0') {
				c = *s;
				*s = '\0';
				mkdir (is, 0777);
				*s = c;
			}
		}

		if (mkdir (is, 0777) < 0) {
			if (batch)
				goto DirAcc;
			cout << "I have problems accessing this";
			cout << " directory; please specify another";
			cout << " path\n";
			goto XN_RETRY;
		}

	} else {
		chdir (wd);     // Move back
	}

	xncdir [nxncdirs] = new char [strlen (is) + 1];
	strcpy (xncdir [nxncdirs], is);
	nxncdirs++;
	goto XN_RETRY;

XN_DONE:

if (!batch) {
cout << '\n';
cout << "Specify the host to run the monitor. By default, there is no host,\n";
cout << "which means that you don't  want to use the display applet.   Note\n";
cout << "that if you want to run everything on a single machine,  you still\n";
cout << "have to specify its Internet name here (localhost is fine).\n\n";
}

HS_RETRY:

	getstring ();

	if (is [0] != '\0') {
		// Check if the host is reachable
		if (gethostbyname (is) == NULL) {
			if (batch) {
				cerr << "Host unreachable\n";
				exit (1);
			}
			cout << "This host is unknown; please specify";
			cout << " another host\n";
			goto HS_RETRY;
		}
	}

	monhost = new char [strlen (is) + 1];
	strcpy (monhost, is);

	if (!batch) {
		cout << '\n';
	}

    if (monhost [0] != '\0') {

if (!batch) {
cout << "Specify  the  number  of  the monitor socket port on the host that\n";
cout << "will be running the monitor. The default port number is ";
cout << DEFMONPT << '\n';
}

PN_RETRY:

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << (monport = DEFMONPT) << '\n';
	} else {
		for (s = is; *s != '\0' && isdigit (*s); s++);
		if (*s != '\0') {
			goto IllNum;
			cout << "This is not a legal number; try again\n";
			goto PN_RETRY;
		}
		monport = atoi (is);
	}
    }

if (!batch) {
cout << '\n';
cout << "Specify the name of the make program (" << DEFMKNAM << "): ";
}

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << DEFMKNAM << '\n';
		strcpy (is, DEFMKNAM);
	}

	mkname = new char [strlen (is) + 1];
	strcpy (mkname, is);

if (!batch) {
cout << '\n';
cout << "Specify  the  path  to  the  directory where you keep your private\n";
cout << "executables. This is where I will put the mks program. The default\n";
cout << "path is:\n\n";
}
	if ((s = HOME) == NULL)
		wd [0] = '\0';
	else
		strcpy (wd, s);
	if ((i = strlen (wd) - 1) >= 0 && wd [i] != '/')
		strcat (wd, "/");
	strcat (wd, DEFMKDIR);

if (!batch) {
	cout << "        " << wd << "\n\n";
}

MD_RETRY:

	getstring ();

	if (is [0] == '\0') {
		cout << "Assuming " << wd << '\n';
		strcpy (is, wd);
	} else {
		if (notabsolute (is)) {
			// Turn it into $HOME/is
			if ((s = HOME) == NULL) {
				if (batch)
					goto NoHome;
				cout << "Cannot determine your home directory;"
					<< " please specify the full path\n";
				goto MD_RETRY;
			}
			strcpy (is+4096, is);
			strcpy (is, s);
			strcat (is, "/");
			strcat (is, is+4096);
		}
	}

	// Execute a tentative chdir there

	getwd (wd);
MD_CD:
	if (chdir (is) < 0) {
		if (!batch) {
			cout << "This directory does not exist;";
			cout << " should I create it? (y): ";
			strcpy (cb, is);
			getstring ();
			if (is [0] != 'y' && is [0] != 'Y' && is [0] != '\0') {
				cout << "Try another path\n";
				goto MD_RETRY;
			}
			strcpy (is, cb);
		}

		for (s = is+1; *s != '\0'; s++) {
			if (*s == '/' && *(s+1) != '\0') {
				c = *s;
				*s = '\0';
				mkdir (is, 0777);
				*s = c;
			}
		}

		if (mkdir (is, 0777) < 0) {
			if (batch)
				goto DirAcc;
			cout << "I have problems accessing this";
			cout << " directory; please specify another";
			cout << " path\n";
			goto MD_RETRY;
		}
	} else {
		chdir (wd);
	}

	mksdir = new char [strlen (is) + 1];
	strcpy (mksdir, is);

	// OK, Now update the version.h file

	if ((vfi = openIStream ("version.h")) == NULL) {
COVH:
		cerr << "Sorry, I can't open version.h\n";
		exit (1);
	}

	// Read everything from version.h up to the line that looks like
	// #define ZZ_SOURCES ... or to the end

	for (m = 0; m < BFFSIZE && !vfi->eof (); m++) vfi->get (cb[m]);

	if (m >= BFFSIZE) {
		cerr << "Sorry, the version.h file seems too long. Recompile\n";
		cerr << "maker with increased BFFSIZE and try again.\n";
		exit (1);
	}

	// Look up #define first

	for (i = 0; i < m; i++) {
		if (strncmp (&(cb [i]), "#define", 7) == 0) {
			i += 7;
			while (i < m && isspace (cb [i])) i++;
			if (i >= m) break;
			if (strncmp (&(cb [i]), "ZZ_SOURCES", 10) == 0) {
				// Find the first newline back
				while (i >= 0 && cb [i] != '\n') i--;
				i++;
				break;
			}
		}
	}

	// vfi->close ();
	delete (vfi);

	// Now rewrite version.h

	vfo = openOStream ("version.h");
	if (vfo == NULL) goto COVH;

	for (m = 0; m < i; m++) vfo->put (cb [m]);

	*vfo << "#define  ZZ_SOURCES      \"" << srcdir << "\"\n";
	*vfo << "#define  ZZ_LIBPATH      \"" << libdir << "\"\n";
	*vfo << "#define  ZZ_INCPATH      \"";

	for (i = m = 0; i < MAXINCDIRS; i++) {
		if (incdir [i]) {
			if (m++)
				*vfo << ' ';
			*vfo << "-I " << incdir [i];
		}
	}

	*vfo << "\"\n";

	if (nxncdirs) {
		*vfo << "#define  ZZ_XINCPAT      {";
		for (m = 0; m < nxncdirs; m++) {
			if (m)
				*vfo << ", ";
			*vfo << "\"" << xncdir [m] << "\"";
		}
		*vfo << ", NULL}\n";
	}

	if (monhost [0] != '\0') {
		*vfo << "#define  ZZ_MONHOST      \"" << monhost << "\"\n";
		*vfo << "#define  ZZ_MONSOCK      " << monport << '\n';
	}

	if (rtag != NULL)
		*vfo << "#define  ZZ_RTAG	\"" << rtag << "\"\n";

	// vfo->close ();
	vfo -> flush ();
	delete (vfo);

	getwd (wd);
	chdir ("LIB");

	// Check if Makefile is present, if not skip this step
	if ((i = open ("Makefile", O_RDONLY)) < 0) {
		cout << "Skipping library compilation\n";
	} else {
		close (i);
        	system ("rm -rf *.o *.a");
		cout << "Creating the library ...\n";
		strcpy (cb, "make COMP=\"");
		strcat (cb, ccomp);
	        strcat (cb, "\" CCOMP=\"");
	        strcat (cb, ckomp);
		strcat (cb, "\"");
		strcat (cb, " RANLIB=./ifranlib");

		if (system (cb)) {
			cerr << "Problems creating the library,";
			cerr << " procedure aborted\n" FLUSH;
			exit (1);
		}
	}

	chdir (wd);

	// Create mks

	cout << "Creating 'mks' ...\n" FLUSH;

	strcpy (cb, ccomp);
	// strcat (cb, " -w -o ");
	strcat (cb, " -o ");
	strcat (cb, mksdir);
	strcat (cb, "/");
	strcat (cb, mkname);
	strcat (cb, " mks.cc ");
	strcat (cb, "-LLIB -ltcpip ");
	strcat (cb, "-DCCOMP=\\\"");
	strcat (cb, ccomp);
	strcat (cb, "\\\" -DMAXLIB=");
	encint (maxlib);
	strcat (cb, is);
	if (system (cb)) {
		cerr << "Problems installing 'mks', procedure aborted\n" FLUSH;
		exit (1);
	}

	strcpy (cb, "strip ");
	strcat (cb, mksdir);
	strcat (cb, "/");
	strcat (cb, mkname);

#ifdef	ZZ_CYW
	strcat (cb, ".exe");
#endif
	system (cb);

	// Create the preprocessor

	getwd (wd);
	chdir ("SMPP");
	cout << "Creating 'smpp' ...\n" FLUSH;
	strcpy (cb, "make ");
	strcat (cb, " COMP=");
	strcat (cb, ccomp);
	// strcat (cb, " OPTI=-w");
	strcat (cb, " OPTI=-O");

	if (system (cb)) {
		cerr << "Problems creating smpp,";
		cerr << " procedure aborted\n" FLUSH;
		exit (1);
	}

#ifdef	ZZ_CYW
	system ("strip smpp.exe");
#else
	system ("strip smpp");
#endif

	chdir (wd);

    	if (monhost [0] != '\0') {

		if (!batch) {
			cout << "Should I create the monitor ? (y): ";
		}

		getstring ();

		if (is [0] == 'y' || is [0] == 'Y' || is [0] == '\0') {

			getwd (wd);
			chdir ("MONITOR");
			cout << "Creating 'monitor' ...\n" FLUSH;
			strcpy (cb, "make COMP=");
			strcat (cb, ccomp);
	                strcat (cb, " PROG=monsel.cc");
			// strcat (cb, " OPTI=-w");
			if (system (cb)) {
				cerr << "Problems creating monitor,";
				cerr << " procedure aborted\n" FLUSH;
				exit (1);
			}
#ifdef	ZZ_CYW
			system ("strip monitor.exe");
#else
			system ("strip monitor");
#endif
			chdir (wd);

			// Mini web server

			chdir ("NWEB");
			cout << "Creating nweb24 (the mini web server) ...\n"
				FLUSH;
			strcpy (cb, "make COMP=");
			strcat (cb, ckomp);
			if (system (cb)) {
				cerr << "Problems creating nweb24,";
				cerr << " procedure aborted\n" FLUSH;
				exit (1);
			}
#ifdef	ZZ_CYW
			system ("strip nweb24.exe");
#else
			system ("strip nweb24");
#endif
			chdir (wd);
if (!batch) {
cout << '\n';
cout << "The monitor is in file 'MONITOR/monitor'. If you want to run it on\n";
cout << "this machine, move to directory MONITOR and execute:\n\n";
cout << "                    monitor standard.t -b\n\n";
cout << "Make  sure  that  no other copy of the monitor is running already,\n";
cout << "because otherwise the monitor will complain about occupied port.\n\n";
cout << "The mini web server,  which you may need for  running  DSD,  is in\n";
cout << "file 'NWEB/nweb24'. Execute nweb24 -h for help.  Also, read README\n";
cout << "in SOURCES/DSD.\n\n";
}
		}

		getwd (wd);
		chdir ("DSD");
		cout << "Setting up 'dsd' ...\n" FLUSH;
		sprintf (cb,
           "sed -e \"s/hhhh/%s/\" -e \"s/pppp/%1d/\" < index.pat > index.html",
		monhost, monport);
                cout << cb << '\n';
                m = system (cb);

        	if (strcmp (jcomp, DEFJCOMP)) {
	                if (m == 0) {
				strcpy (cb, jcomp);
				strcat (cb, " -nowarn DSD.java");
	                	cout << cb << '\n';
	                	m = system (cb);
	                }
			if (m) {
				cerr << "Problems creating DSD,";
				cerr << " procedure aborted\n" FLUSH;
				exit (1);
			}
	        }

		if (!batch) {
			cout << '\n';
			cout << "The display applet is in directory 'DSD'.\n";
			cout << "Should I move it elsewhere? (n): ";
		}
		getstring ();
		if (is [0] == 'y' || is [0] == 'Y') {
			cout << '\n';
DS_RETRY:
		if (!batch) {
	   cout << "Specify the directory where the applet should be moved: ";
		}
  	          	getstring ();
	          	if (is [0] == '\0') goto DS_RETRY;
 		  	// Check if the directory is reachable
  		  	if (notabsolute (is)) {
				// Turn it into $HOME/is
				if ((s = HOME) == NULL) {
				 if (batch)
					goto NoHome;
				 cout << "Cannot determine your home directory;"
					<< " please specify the full path\n";
				 goto DS_RETRY;
				}
				strcpy (is+4096, is);
				strcpy (is, s);
				strcat (is, "/");
				strcat (is, is+4096);
		   	}
		   	// Execute a tentative chdir there
		   	if (chdir (is) < 0) {
				if (batch)
					goto DirAcc;
				cout << "This directory is inaccessible;" <<
					" please specify another path\n";
				goto DS_RETRY;
		    	}
		    	// Move back to where you should be
		    	chdir (wd);
                    	chdir ("DSD");
                    	strcpy (cb, "cp *.cla* *.htm* *.gif* ");
                    	strcat (cb, is);
                    	system (cb);
          	}
          	chdir (wd);

    	}

	cout << "Done.\n" FLUSH;
}
