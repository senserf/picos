/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-07   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

// #define DEBUG 1

/* ----------------------- */
/* Make program for smurph */
/* ----------------------- */

#include	"version.h"
#include        "LIB/lib.h"
#include        "LIB/syshdr.h"
#include	<dirent.h>

#define	direct dirent

#if	PATH_MAX < 4096
#undef	PATH_MAX
#define	PATH_MAX 4096
#endif

#define	getwd(a) getcwd (a, PATH_MAX)

/* ---------------------------------------------------- */
/* The three constants below should be defined by maker */
/* ---------------------------------------------------- */
#ifndef CCOMP
#define CCOMP   "g++"
#endif

#ifndef MAXLIB
#define MAXLIB  5
#endif

#ifndef	ZZ_INCPATH
#define	ZZ_INCPATH	""
#endif

#define	LNAMESIZE	32	// Library name index size
#define	LNAMELENG	 9	// Library file name length (+1)

	//
	// Call format:
	//
	//  mks params filenames
	//
	//  where params may contain the following items
	//
	//      -f           -- optimization (default = no optimization)
	//      -a           -- assertions switched off (default = on)
	//      -v           -- observers switched off (default = on)
	//      -m           -- suppress error detection for MPA (default = on)
	//      -d           -- DISTANCE should be BIG (default = off)
	//      -i           -- BITCOUNT should be BIG (default = off)
	//      -r           -- RATE should be BIG (default = off)
	//      -n           -- clock errors switched off (default = on)
	//	-p           -- wait request are tagged with priorities
	//	-q	     -- message queue size can be limited
	//	-3           -- 3D geometry of radio channels
        //      -8           -- use drand48
	//      -g           -- debugging (default = off)
	//      -u           -- standard client disabled (default = enabled)
	//	-z           -- links can be made "faulty"
	//      -b n         -- TIME precision (default = 2)
	//      -o fname     -- send binaries to 'fname' (default 'side')
	//      -t           -- 'touch' the protocol source files
        //
	//	-W           -- vizualization mode
        //      -R           -- the real-time version of the simulator
	//	-J	     -- journaling compiled in
        //      -D           -- deterministic scheduler
        //      -L           -- no links
	//	-X           -- no radio
        //      -C           -- no client, implies -L -X
        //      -S           -- no rvariables, implies -C
        //      -F           -- no FP (embedded), implies -S, -D, -R
	//
	//		The following options are 'undocumented'. If you have
	//		managed to get this far, you can as well learn about
	//		them:
	//
	//	-E           -- leave the '.C' files after compilation
	//	-V	     -- 'verbose'
	//
	// The configuration of options, with exception of '-o', determines the
	// library subdirectory name.
	//
	// filenames describe the files containing the smurph code. Suffixes:
	// '.c' and '.cc' ('.cpp' and '.cc' in the Windows version) are legal
        // and treated in the same way.
        // Suffix '.C' ('.cxx' under Windows) is
	// reserved for the output file produced by smpp. If no files
	// are specified, all files in the current directory ending with the
	// proper suffix are assumed.
	//

#define	LIBX_OPT	0
#define	LIBX_ASR	(LIBX_OPT+1)
#define	LIBX_OBS	(LIBX_ASR+1)
#define	LIBX_AER	(LIBX_OBS+1)
#define	LIBX_DST	(LIBX_AER+1)
#define	LIBX_BTC	(LIBX_DST+1)
#define	LIBX_RTS	(LIBX_BTC+1)
#define	LIBX_TOL	(LIBX_RTS+1)
#define	LIBX_TAG	(LIBX_TOL+1)
#define	LIBX_FLK	(LIBX_TAG+1)
#define	LIBX_QSL	(LIBX_FLK+1)
#define	LIBX_R3D	(LIBX_QSL+1)
#define	LIBX_R48	(LIBX_R3D+1)
#define	LIBX_CLI	(LIBX_R48+1)
#define	LIBX_REA	(LIBX_CLI+1)
#define	LIBX_RSY	(LIBX_REA+1)
#define	LIBX_JOU	(LIBX_RSY+1)
#define	LIBX_DET	(LIBX_JOU+1)
#define	LIBX_NOC	(LIBX_DET+1)
#define	LIBX_NOL	(LIBX_NOC+1)
#define	LIBX_NOR	(LIBX_NOL+1)
#define	LIBX_NOS	(LIBX_NOR+1)
#define LIBX_NFP	(LIBX_NOS+1)
#define	LIBX_DBG	(LIBX_NFP+1)
#define	LIBX_PRC	(LIBX_DBG+1)

#define	NOPT		(LIBX_PRC+1)

IPointer	MemoryUsed;

char    PRC [24], OPT [16], ASR [16], OBS [16], AER [16], DST [16], BTC [16],
	RTS [16], TOL [16], TAG [16], QSL [16], R3D [16], R48 [16], DBG [16],
	CLI [16], REA [16], RSY [16], JOU [16], DET [16], NOC [16], NOL [16],
	NOR [16], NOS [16], NFP [16], FLK [16];

char    *options [32],
	*ofname, libindex [LNAMESIZE], libfname [LNAMELENG], **cfiles, *pname;

int     ncfiles = 0, cfsize = 16;

char    cd [PATH_MAX];

char    tmpfname [64], tmponame [64];

int	LeaveCFiles = 0,	// Flag == do not remove preprocessor '.C' files
	BeVerbose = 0,		// Inform about what exactly is being done
	TouchSources = 0;	// Flag == touch source files

#define	NOTHING	(-2)		// Special chars for reading the signature
#define	ENDFILE (-1)

class	Signature {

	char LIndex [LNAMESIZE];	// Library index
	int  NSFiles,			// Number of lists
	     MaxNSFiles,		// Maximum
	     LastChar;
        istream *Inp;
	char ***CL;			// The array of lists

	public:

	Signature (char*);
	int empty () { return LIndex [0] == '\0'; };
	void update (), close ();
	int pkc (), chr (), nbl ();
};

Signature *S;

#ifdef	DEBUG
void debug (char *t) {
	cerr << t << '\n';
};
#else
#define debug(a)
#endif

#define fnamecomp(s1,s2)  strcmp (s1, s2)

ostream	*openOStream (char *fn) {

	ostream *str = new std::ofstream (fn);

	if (str->good ())
		return str;

	delete str;
	return NULL;
}

istream	*openIStream (char *fn) {

	istream *str = new std::ifstream (fn);

	if (str->good ())
		return str;

	delete str;
	return NULL;
}

void    badArgs () {

	cerr << "Usage:  " << pname << " 'options' 'files'\n";
	cerr << "      Options:\n";
	cerr << "       -f         optimization\n";
	cerr << "       -a         assertions switched off\n";
	cerr << "       -v         observers deactivated\n";
	cerr << "       -m         error detection for BIG arith. suppressed\n";
	cerr << "       -d         distances are BIG\n";
	cerr << "       -i         bit counters are BIG\n";
	cerr << "       -r         port transmission rates are BIG\n";
	cerr << "       -n         clocks are absolutely accurate\n";
	cerr << "       -p         wait requests tagged with priorities\n";
	cerr << "       -q         message queue size can be limited\n";
	cerr << "       -3         3-d geometry of radio channels\n";
	cerr << "       -8         use the standard drand48 rnd gen family\n";
	cerr << "       -g         debugging\n";
	cerr << "       -G         like -g, no signals caught\n";
	cerr << "       -u         standard client permanently disabled\n";
	cerr << "       -z         links can be made faulty\n";
	cerr << "       -W         viewable version (emulate real time)\n";
        cerr << "       -R         real-time control program\n";
        cerr << "       -D         deterministic event scheduler, implies -n\n";
        cerr << "       -L         no link/port objects compiled\n";
        cerr << "       -X         no radio objects compiled\n";
        cerr << "       -C         no client objects compiled, implies -L -X\n";
        cerr << "       -S         no RVariables, implies -C\n";
        cerr << "       -F         no FP (embedded), implies -S, -D, -R\n";
	cerr << "       -b n       precision of BIG numbers (default is 2)\n";
	cerr << "       -o fname   direct binary simulator to 'fname'\n";
	cerr << "       -t         'touch' the protocol source files\n";
	cerr << "      Default: '-b 2 -o side' (other flags cleared)\n";
        cerr << "      Incompatibilities: -F and -8, -R and -V, -b 0 and -F\n";
        cerr << "                         -G and -g\n";
	exit (1);
}

void    sigint () {

	unlink (tmpfname);
	unlink (tmponame);
	exit (1);
}

int	excptn (char *t) {

	cerr << pname << ": " << t << '\n';
	sigint ();
	return (0);
}

int     processFName (char *fn) {

/* --------------------------- */
/* Processes filename argument */
/* --------------------------- */

	char    *cp, **scratch;
	int     i;

	for (cp = fn + strlen (fn) - 1; cp != fn; cp--)
		// Locate the suffix
		if (*cp == '.') break;

	if (cp == fn) return (-1);

	cp++;

	if (fnamecomp (cp, "c") && fnamecomp (cp, "cc"))
		// Illegal suffix
		return (-1);

	// Check if the file has not been specified already

	for (i = 0; i < ncfiles; i++)
		if (fnamecomp (cfiles [i], fn) == 0) return (-1);

	// Add the new file to the list

	if (ncfiles >= cfsize) {
		// Must increase the array size
		scratch = new char* [cfsize];
		for (i = 0; i < cfsize; i++) scratch [i] = cfiles [i];
		delete (cfiles);
		cfiles = new char* [cfsize += cfsize];
		for (i = 0; i < ncfiles; i++) cfiles [i] = scratch [i];
		delete (scratch);
	}

	cfiles [ncfiles] = new char [strlen (fn) + 1];
	strcpy (cfiles [ncfiles], fn);
	ncfiles++;
	return (0);
}

char    *toObject (char *fn) {

/* --------------------------------------------- */
/* Converts source file name to object file name */
/* --------------------------------------------- */

	char    *cp;
	int     i;

	for (cp = fn + strlen (fn) - 1; cp != fn; cp--)
		// Locate the suffix
		if (*cp == '.') break;

	for (i = 0; ; i++, fn++) {
		cd [i] = *fn;
		if (fn == cp) break;
	}

	cd [++i] = 'o';
	cd [++i] = '\0';
	return (cd);
}

char    *toSmpp (char *fn) {

/* ---------------------------------------------------------- */
/* Converts source file name to preprocessor output file name */
/* ---------------------------------------------------------- */

	char    *cp;
	int     i;

	for (cp = fn + strlen (fn) - 1; cp != fn; cp--)
		// Locate the suffix
		if (*cp == '.') break;

	for (i = 0; ; i++, fn++) {
		cd [i] = *fn;
		if (fn == cp) break;
	}
	cd [++i] = 'C';
	cd [++i] = '\0';
	return (cd);
}

void    getFNames () {

/* ---------------------------------------------------------- */
/* Searches  the  current  directory  for  filenames with the */
/* proper suffixes. Introduced primarily for DOS  --  to make */
/* sure that the library file name is no longer than 8 chars. */
/* ---------------------------------------------------------- */

	DIR     *cdir;
	struct  direct  *dentry;

	if ((cdir = opendir (".")) == NULL)
		excptn ("cannot open current directory");

	while ((dentry = readdir (cdir)) != NULL) {
		// Process next entry
		processFName (dentry->d_name);
	}

	closedir (cdir);
}

void compressLibName () {
  // Compress the library name
  int i, bit, dig;
  long long ix;

  ix = 0;
  for (i = 0; (dig = libindex [i]) != '\0'; i++) {
    if (dig == 'n' || dig == 'y') {
	// Binary
	ix = (ix << 1) | (dig == 'y');
	continue;
    }
    if (dig == '-' || dig == 'a' || dig == 'b') {
	// Alternative
	ix = (ix * 3) + (dig == '-' ? 0 : (dig == 'a' ? 1 : 2));
	continue;
    }
    // A digit
    ix = ix * 10 + (dig - '0');
  }
  libfname [0] = 'L';
  for (i = 1; ix != 0; i++) {
    if (i == LNAMELENG)
      excptn ("Library file name too long");
    libfname [i] = (char) ('a' + ix % 26);
    ix /= 26;
  }
  libfname [i] = '\0';
}

int     findLVer () {

/* ---------------------------------------------------------- */
/* Finds  and  possibly  creates  the  proper library version */
/* subdirectory                                               */
/* ---------------------------------------------------------- */

	DIR     *cdir;
	struct  direct  *dentry;
	struct  stat    statbuf;
	int     count, tfd;
	long    matime, loc, cloc;
	char    *cc;

	if ((cdir = opendir (".")) == NULL)
		excptn ("cannot open library directory");

	for (count = 0; (dentry = readdir (cdir)) != NULL; ) {
		if (*(dentry->d_name) == '.') continue;
		if (fnamecomp (dentry->d_name, libfname) == 0) break;
		count++;
	}

	if (dentry != NULL) {
		// Directory found
		closedir (cdir);
		cc = form ("%s/lockfile", libfname);
		if ((tfd = open (cc, O_WRONLY, 0)) < 0)
			tfd = open (cc, O_WRONLY+O_CREAT, 0777);
		return (tfd);
	}

	// Check if something should be removed

	while (count >= MAXLIB) {

		// Determine the least recently used directory
		rewinddir (cdir);
		for (matime = -1, cloc = telldir (cdir);
			(dentry = readdir (cdir)) != NULL;
				cloc = telldir (cdir)) {

			if (*(dentry->d_name) == '.') continue;
			cc = form ("%s/main.o", dentry->d_name);
			// main.o should always be present there
			if ((tfd = open (cc, O_RDONLY, 0)) < 0) {
				// Garbage -- feel free to remove it
				tfd = open (form ("%s/lockfile",dentry->d_name),
					O_WRONLY, 0);
				flock (tfd, LOCK_EX);
				cout << "Removing: " << dentry->d_name << '\n';
				system (form ("rm -fr %s 2>/dev/null",
							      dentry->d_name));
				flock (tfd, LOCK_UN);
				close (tfd);
				goto NEXT;
			}

			// Determine the status of main.o
			if (fstat (tfd, &statbuf) < 0)
				excptn (form ("problems accessing '%s'", cc));
			close (tfd);

			if (matime == -1 || statbuf.st_atime < matime) {
				matime = statbuf.st_atime;
				loc = cloc;
			}
		}

		if (matime == -1) {
PBMS:
			excptn ("problems determining victim library");
		}

		seekdir (cdir, loc);
		if ((dentry = readdir (cdir)) == NULL) goto PBMS;
		cout << "Removing: " << dentry->d_name << '\n';
		tfd = open (form ("%s/lockfile", dentry->d_name), O_WRONLY, 0);
		flock (tfd, LOCK_EX);
		system (form ("rm -fr %s 2> /dev/null", dentry->d_name));
		flock (tfd, LOCK_UN);
		close (tfd);

NEXT: 		count--;
	}
	closedir (cdir);

	// Now create the new library

	if (mkdir (libfname, 0777) < 0)
		excptn (form ("cannot create '%s'", libfname));

        // This automatically locks the new library under DOS
	tfd = open (form ("%s/lockfile", libfname), O_WRONLY+O_CREAT, 0777);
	return (tfd);
}

int Signature::pkc () {
	char c;
	if (LastChar == NOTHING) {
		if (Inp->eof ())
			LastChar = ENDFILE;
		else {
			Inp->get (c);
			LastChar = (int) c;
		}
	}
	return LastChar;
};

int Signature::chr () {
	int c;
	if (LastChar == NOTHING) pkc ();
	c = LastChar;
	LastChar = NOTHING;
	return c;
};

int Signature::nbl () {
	int c;
	while (1) {
		c = pkc ();
		if (c == ENDFILE || c == '\n' || !isspace (c)) break;
		chr ();
	}
	return c;
};

Signature::Signature (char *fn) {

/* ------------------------------------ */
/* Unpacks the signature list to memory */
/* ------------------------------------ */

	ostream *out;
	char name [PATH_MAX], ***cl, **tl, **ttl;
	int c, i, j, maxcom;

        Inp = openIStream ("side.sgn");
	LastChar = NOTHING;

	LIndex [0] = '\0';
	NSFiles = 0;
	CL = new char** [MaxNSFiles = 32];
	tl = NULL;

        if (Inp != NULL) {
		tl = new char* [maxcom = 1024];
		for (i = j = 0; pkc () != ENDFILE; i++) {
			if (nbl () == '\n') break;
			if (j < LNAMESIZE-1)
                          LIndex [j++] = chr ();
			else
			  chr ();
		}
		LIndex [j] = '\0';
	}
	// We are done with the library index
	if (fnamecomp (LIndex, fn)) {
		// Different libraries
		out = openOStream ("side.sgn");
		if (out == NULL)
			excptn ("cannot open side.sgn file");
		*out << fn << '\n';
		out->flush ();
		delete out;
		if (Inp != NULL) { delete Inp; Inp = NULL; }
		if (tl != NULL) { delete tl; tl = NULL; }
		LIndex [0] = '\0';
		return;
	}
	// The same library: read the common lists
	j = 0;
	while (1) {
	  while ((c = nbl ()) == '\n') chr ();
	  if (c == ENDFILE) break;
	  // A non-space character -> read a new word
	  if (j == maxcom) {
	    // Increase the size of tl
    	    ttl = new char* [maxcom + maxcom];
	    for (i = 0; i < maxcom; i++) ttl [i] = tl [i];
	    delete tl;
	    tl = ttl;
	    maxcom += maxcom;
	  }
          for (i = 0; (c = pkc ()) != ENDFILE && !isspace (c); i++)
	    if (i < PATH_MAX-1) name [i] = (char) chr (); else chr ();
	  name [i] = '\0';
	  tl [j] = new char [i + 1];
	  strcpy (tl [j], name);
	  j++;
	  // Examine the delimiter
	  if ((c = nbl ()) == ENDFILE || c == '\n') {
            // End of line
            if (NSFiles == MaxNSFiles) {
              // Increase the size
              cl = new char** [MaxNSFiles + MaxNSFiles];
              for (i = 0; i < MaxNSFiles; i++) cl [i] = CL [i];
              delete CL;
              CL = cl;
              MaxNSFiles += MaxNSFiles;
            }
            CL [NSFiles] = new char* [j+1];
            for (i = 0; i < j; i++) CL [NSFiles][i] = tl [i];
            CL [NSFiles][j] = NULL;
            NSFiles++;
            j = 0;
          }
	}
	delete Inp;
	delete tl;
};

void	Signature::update () {

/* --------------------------------------- */
/* Updates the signature list of "commons" */
/* --------------------------------------- */
        char name [PATH_MAX];
        int c, i, j;

	if (NSFiles == 0) return; // Nothing to do for an empty signature
	// On Cygwin, the tmp file, being created so late, causes clock skew
	// warnings
	system (form ("touch -t 200501010000 %s", tmpfname));
	// Get the list of user files to be re-compiled
        i = system (form ("make -n -f %s > %s", tmpfname, tmponame));
        Inp = openIStream (tmponame);
        if (Inp == NULL) excptn ("cannot open tmp file for reading");
	LastChar = NOTHING;

	while (1) {
	  while ((c = chr ()) != ENDFILE && c != '+');
	  if (c == ENDFILE) break;
	  if (chr () != 'C' || chr () != 'o' || chr () != 'm' || chr () != 'p'
           || chr () != 'i' || chr () != 'l' || chr () != 'i' || chr () != 'n'
           || chr () != 'g' || chr () != ' ')
	    continue;
	  // We are at the beginning of the program file name
	  debug ("Program file name found");
          for (i = 0; (c = pkc ()) != ENDFILE && c != '\"'; i++)
            if (i < PATH_MAX-1) name [i] = chr (); else chr ();
          name [i] = '\0';
	  debug (name);
	  // Now find this entry in the signature and remove it
	  for (i = 0; i < NSFiles; i++)
	    if (fnamecomp (CL [i][0], name) == 0) break;
	  if (i >= NSFiles) continue;	// Not found -> ignore
	  debug ("Name found - deleted");
	  delete CL [i];
	  for (j = i+1; j < NSFiles; j++) CL [j-1] = CL [j];
	  NSFiles--;
	}
	delete Inp;
};

void	Signature::close () {

/* --------------------------------------------------- */
/* Closes the signature and writes it back to the file */
/* --------------------------------------------------- */

	ostream *out;
	int i;
	char **t;

	if (empty ()) return;
        out = openOStream ("side.sgn");
        if (out == NULL) excptn ("cannot open side.sgn file");
	*out << LIndex << '\n';
	for (i = 0; i < NSFiles; i++) {
		t = CL [i];
		while (1) {
			*out << *t;
			if (*(++t) == NULL) break;
			*out << ' ';
		}
		*out << '\n';
	}
	out->flush ();
        delete out;
};

void    makeSmurph () {

/* ---------------------------- */
/* Makes the simulator instance */
/* ---------------------------- */

	char    fname [PATH_MAX], c, *cp;
	int     i, eraseo;

	istream *inp;
	ostream *out;

	// Read in the signature file
	S = new Signature (libfname);

	eraseo = S->empty ();
	if (eraseo || TouchSources) {
		// Erase old binaries
		for (i = 0; i < ncfiles; i++)
                        unlink (toObject (cfiles [i]));
	}

	strcpy (fname, ZZ_SOURCES);
	strcat (fname, "/");
	strcat (fname, "KERNEL/depend.txt");

	inp = openIStream (fname);
	if (inp == NULL)
		excptn ("cannot open 'depend.txt' file");

	// Now create the tmp files
	strcpy (tmpfname, "/tmp/smXXXXXX");
	mkstemp (tmpfname);
	system (form ("touch -r %s %s", fname, tmpfname));
	strcpy (tmponame, tmpfname);
	tmponame [6] = 'o';

	if (signal (SIGINT, SIG_IGN) != SIG_IGN)
		signal (SIGINT, (SIGARG) sigint);
	if (signal (SIGQUIT, SIG_IGN) != SIG_IGN)
		signal (SIGQUIT, (SIGARG) sigint);

	out = openOStream (tmpfname);
	if (out == NULL) {
		excptn ("cannot open temporary file");
        }

	*out << "VER= " << VERSION << '\n';
	*out << "SRC= " << ZZ_SOURCES << '\n';
	*out << "LIB= " << ZZ_LIBPATH << '/' << libfname << '\n';
	*out << "INC= " << ZZ_INCPATH << '\n';

	*out << "CMP= " << CCOMP;
	if (BeVerbose) *out << " -v";
	*out << '\n';
	*out << "OPT= " << OPT << '\n';
	*out << "ASR= " << ASR << '\n';
	*out << "OBS= " << OBS << '\n';
	*out << "AER= " << AER << '\n';
	*out << "DST= " << DST << '\n';
	*out << "BTC= " << BTC << '\n';
	*out << "RTS= " << RTS << '\n';
	*out << "TOL= " << TOL << '\n';
	*out << "TAG= " << TAG << '\n';
	*out << "NOC= " << NOC << '\n';
	*out << "NOL= " << NOL << '\n';
	*out << "NOR= " << NOR << '\n';
	*out << "NOS= " << NOS << '\n';
        *out << "NFP= " << NFP << '\n';
	*out << "FLK= " << FLK << '\n';
	*out << "QSL= " << QSL << '\n';
	*out << "R3D= " << R3D << '\n';
	*out << "R48= " << R48 << '\n';
	*out << "DBG= " << DBG << '\n';
	*out << "CLI= " << CLI << '\n';
        *out << "REA= " << REA << '\n';
        *out << "RSY= " << RSY << '\n';
        *out << "JOU= " << JOU << '\n';
        *out << "DET= " << DET << '\n';
	*out << "PRC= " << PRC << '\n';

	*out << "TRG= " << ofname << '\n';

	// The list of user object files

	*out << "USO=";
	for (i = 0; i < ncfiles; i++)
		*out << ' ' << toObject (cfiles [i]);
	*out << '\n';

	// Copy the rest

	while (! inp->eof ()) {

		inp->get (c);

		if (c == '?') {
			// Pattern
			if (libindex [LIBX_DBG] == '-') {
				// Strip the executable unless
				// -g has been selected
				*out << "\tstrip " << ofname <<
#ifdef	ZZ_CYW
					".exe" <<
#endif
							'\n';
			}
			for (i = 0; ; i++) {
				if (inp->eof () || i >= PATH_MAX-1)
			    excptn ("'depend.txt' file has illegal contents");
				inp->get (c);
				if (c == '?') break;
				fname [i] = c;
			}

			fname [i] = '\0';

			for (i = 0; i < ncfiles; i++) {
				for (cp = fname; *cp != '\0'; cp++) {
					if (*cp == '%')
						*out << cfiles [i];
					else if (*cp == '@')
						*out << toObject (cfiles [i]);
					else if (*cp == '^')
						*out << i;
					else if (*cp == '&')
						*out << toSmpp (cfiles [i]);
					else if (*cp == '!') {
						// SMPP params
						*out << i << ' ' << cfiles [i];
						if (libindex [LIBX_TAG] == 'y')
						  *out << " -p";
						if (libindex [LIBX_NOC] == 'n')
						  *out << " -c";
						if (libindex [LIBX_NOL] == 'n')
						  *out << " -l";
						if (libindex [LIBX_NOR] == 'n')
						  *out << " -r";
						if (libindex [LIBX_NOS] == 'n')
						  *out << " -s";
					} else
						out->put (*cp);
				}
				if (i < ncfiles-1) *out << '\n';
			}
		} else {
		
			out->put (c);
		}
	}

	delete inp;
	out->flush ();
	delete out;

	// Take care of the signature list
	S->update ();
	S->close ();

	unlink (tmponame);

	system (form ("touch -t 200501010000 %s", tmpfname));

	// Make the simulator
	eraseo = BeVerbose ? system (form ("make -f %s", tmpfname)) :
				system (form ("make -s -f %s", tmpfname));
	unlink (tmpfname);

	// Remove any leftover '*.C' files
	if (!LeaveCFiles)
		for (i = 0; i < ncfiles; i++) unlink (toSmpp (cfiles [i]));

	if (eraseo)
		excptn ("errors -- simulator not created");

	if (signal (SIGINT, SIG_IGN) != SIG_IGN) signal (SIGINT, SIG_DFL);
	if (signal (SIGQUIT, SIG_IGN) != SIG_IGN) signal (SIGQUIT, SIG_DFL);
}


main    (int argc, char *argv[]) {

	int     i, semfd, verfd;
	char	*str;

	// Initialize things for argument processing

	ofname = NULL;

	for (pname = *argv + strlen (*argv); pname != *argv; pname--)
		if (*(pname - 1) == '/') 
			break;

	argc--; argv++;

	strcpy (OPT, "-DZZ_OPT=0");             options [LIBX_OPT] = OPT;
	strcpy (ASR, "-DZZ_ASR=1");             options [LIBX_ASR] = ASR;
	strcpy (OBS, "-DZZ_OBS=1");             options [LIBX_OBS] = OBS;
	strcpy (AER, "-DZZ_AER=1");             options [LIBX_AER] = AER;
	strcpy (DST, "-DZZ_DST=0");             options [LIBX_DST] = DST;
	strcpy (BTC, "-DZZ_BTC=0");             options [LIBX_BTC] = BTC;
	strcpy (RTS, "-DZZ_RTS=0");             options [LIBX_RTS] = RTS;
	strcpy (TOL, "-DZZ_TOL=1");             options [LIBX_TOL] = TOL;
	strcpy (TAG, "-DZZ_TAG=0");		options [LIBX_TAG] = TAG;
	strcpy (FLK, "-DZZ_FLK=0");		options [LIBX_FLK] = FLK;
	strcpy (QSL, "-DZZ_QSL=0");             options [LIBX_QSL] = QSL;
	strcpy (R3D, "-DZZ_R3D=0");             options [LIBX_R3D] = R3D;
	strcpy (R48, "-DZZ_R48=0");		options [LIBX_R48] = R48;
	strcpy (DBG, "-DZZ_DBG=0");             options [LIBX_DBG] = DBG;
	strcpy (CLI, "-DZZ_CLI=1");             options [LIBX_CLI] = CLI;
        strcpy (REA, "-DZZ_REA=0");		options [LIBX_REA] = REA;
        strcpy (JOU, "-DZZ_JOU=0");		options [LIBX_JOU] = JOU;
        strcpy (DET, "-DZZ_DET=0");		options [LIBX_DET] = DET;
        strcpy (NOC, "-DZZ_NOC=1");		options [LIBX_NOC] = NOC;
        strcpy (NOR, "-DZZ_NOR=1");		options [LIBX_NOR] = NOR;
        strcpy (NOS, "-DZZ_NOS=1");		options [LIBX_NOS] = NOS;
        strcpy (NFP, "-DZZ_NFP=0");		options [LIBX_NFP] = NFP;
	// LONG is always at least 64 bits
	strcpy (PRC, "-DBIG_precision=1");      options [LIBX_PRC] = PRC;

	if (strcmp (pname, "vuee") == 0 ||
	    strcmp (pname, "vue2") == 0  ) {
		// VUEE options selection
        	strcpy (RSY, "-DZZ_RSY=1");
        	strcpy (NOL, "-DZZ_NOL=0");
	} else {
        	strcpy (RSY, "-DZZ_RSY=0");
        	strcpy (NOL, "-DZZ_NOL=1");
	}

        options [LIBX_RSY] = RSY;
        options [LIBX_NOL] = NOL;

	options [NOPT] = NULL;

	strcpy (libindex, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	cfiles = new char* [cfsize];

	while (argc > 0) {

		// Process next argument

		if (**argv != '-' || (*argv)[1] == '\0' || (*argv)[2] != '\0') {
			if (processFName (*argv)) badArgs ();
			argv++;
			argc--;
			continue;
		}

		switch ((*argv)[1]) {

		  case 'f' :

			if (libindex [LIBX_OPT] != 'x') badArgs ();
			strcpy (&(OPT [9]), "1 -O");
			libindex [LIBX_OPT] = 'y';
			break;

		  case 'a' :

			if (libindex [LIBX_ASR] != 'x') badArgs ();
			ASR [9] = '0';
			libindex [LIBX_ASR] = 'n';
			break;

		  case 'v' :

			if (libindex [LIBX_OBS] != 'x') badArgs ();
			OBS [9] = '0';
			libindex [LIBX_OBS] = 'n';
			break;

		  case 'm' :

			if (libindex [LIBX_AER] != 'x') badArgs ();
			AER [9] = '0';
			libindex [LIBX_AER] = 'n';
			break;

		  case 'd' :

			if (libindex [LIBX_DST] != 'x') badArgs ();
			DST [9] = '1';
			libindex [LIBX_DST] = 'y';
			break;

		  case 'i' :

			if (libindex [LIBX_BTC] != 'x') badArgs ();
			BTC [9] = '1';
			libindex [LIBX_BTC] = 'y';
			break;

		  case 'r' :

			if (libindex [LIBX_RTS] != 'x') badArgs ();
			RTS [9] = '1';
			libindex [LIBX_RTS] = 'y';
			break;

		  case 'n' :

			if (libindex [LIBX_TOL] != 'x') badArgs ();
			TOL [9] = '0';
			libindex [LIBX_TOL] = 'n';
			break;

		  case 'p' :

			if (libindex [LIBX_TAG] != 'x') badArgs ();
			TAG [9] = '1';
			libindex [LIBX_TAG] = 'y';
			break;

		  case 'z' :

			if (libindex [LIBX_FLK] != 'x') badArgs ();
			FLK [9] = '1';
			libindex [LIBX_FLK] = 'y';
			break;

		  case 'q' :

			if (libindex [LIBX_QSL] != 'x') badArgs ();
			QSL [9] = '1';
			libindex [LIBX_QSL] = 'y';
			break;

		  case '3' :

			if (libindex [LIBX_R3D] != 'x') badArgs ();
			R3D [9] = '1';
			libindex [LIBX_R3D] = 'y';
			break;

		  case '8' :

			if (libindex [LIBX_R48] != 'x') badArgs ();
			R48 [9] = '1';
			libindex [LIBX_R48] = 'y';
			if (libindex [LIBX_NFP] != 'x') badArgs ();
			break;

		  case 'g' :

			if (libindex [LIBX_DBG] != 'x') badArgs ();
			strcpy (&(DBG [9]), "1 -g");
			libindex [LIBX_DBG] = 'y';
			break;

		  case 'G' :

			if (libindex [LIBX_DBG] != 'x') badArgs ();
			strcpy (&(DBG [9]), "2 -g");
			libindex [LIBX_DBG] = 'y';
			break;

		  case 'u' :

			if (libindex [LIBX_CLI] != 'x') badArgs ();
			CLI [9] = '0';
			libindex [LIBX_CLI] = 'n';
			break;

		  case 'W' :

                        if (libindex [LIBX_REA] != 'x') badArgs ();
			if (libindex [LIBX_RSY] != 'x') badArgs ();
			RSY [9] = '1';
			libindex [LIBX_RSY] = 'y';
			break;

                  case 'R' :

                        if (libindex [LIBX_REA] != 'x') badArgs ();
			if (libindex [LIBX_RSY] != 'x') badArgs ();
                        REA [9] = '1';
                        libindex [LIBX_REA] = 'y';
                        break;


                  case 'J' :

                        if (libindex [LIBX_JOU] != 'x') badArgs ();
                        JOU [9] = '1';
                        libindex [LIBX_JOU] = 'y';
                        break;

                  case 'D' :

                        if (libindex [LIBX_DET] != 'x') badArgs ();
                        DET [9] = '1';
                        libindex [LIBX_DET] = 'y';
                        // Force '-n', if it isn't there already
			TOL [9] = '0';
                        break;

		  case 'C' :

			if (libindex [LIBX_NOC] != 'x') badArgs ();
			NOC [9] = '0';
			libindex [LIBX_NOC] = 'n';
                        NOL [9] = '0';	/* Force -L */
                        NOR [9] = '0';	/* Force -X */
			break;

		  case 'L' :

			if (libindex [LIBX_NOL] != 'x') badArgs ();
			NOL [9] = '0';
			libindex [LIBX_NOL] = 'n';
			break;

		  case 'X' :

			if (libindex [LIBX_NOR] != 'x') badArgs ();
			NOR [9] = '0';
			libindex [LIBX_NOR] = 'n';
			break;

		  case 'S' :

			if (libindex [LIBX_NOS] != 'x') badArgs ();
			NOS [9] = '0';
			libindex [LIBX_NOS] = 'n';
                        NOC [9] = '0';  /* Force -C */
                        NOL [9] = '0';	/* Force -L */
                        NOR [9] = '0';	/* Force -X */
			break;

		  case 'F' :

			if (libindex [LIBX_NFP] != 'x') badArgs ();
			NFP [9] = '1';
			libindex [LIBX_NFP] = 'y';
                        REA [9] = '1';  /* Force -R */
                        NOS [9] = '0';  /* Force -S */
			NOC [9] = '0';  /* Force -C */
			NOL [9] = '0';  /* Force -L */
                        NOR [9] = '0';	/* Force -X */
                        DET [9] = '1';  /* Force -D */
			TOL [9] = '0';  /* No clock tolerance */
                        if (libindex [LIBX_R48] != 'x') badArgs ();
			if (libindex [LIBX_RSY] != 'x') badArgs ();
			if (PRC [16] == '-') badArgs ();
			break;

		  case 'b' :

			if (argc < 2 || libindex [LIBX_PRC] != 'x') badArgs ();
			argv++;
			argc--;

			i = **argv - '0';
			if (i < 0 || i > 9 || (*argv)[1] != '\0') badArgs ();
			libindex [LIBX_PRC] = **argv;
			if (**argv == '0') {
                                // Don't use double at ALL (LONG is 64 bits)
                                PRC [16] = '1';
			} else {
				// Translate into 32-bit portions
				PRC [16] = ((int)((i + 1) / 2)) + '0';
			}
			break;

		  case 'o' :

			if (argc < 2 || ofname != NULL) badArgs ();
			argv++;
			argc--;

			ofname = new char [strlen (*argv) + 1];
			strcpy (ofname, *argv);
			break;

 		  case 't' :

			if (TouchSources++) badArgs ();
			break;

		  case 'E' :

			LeaveCFiles = 1;
			break;

		  case 'V' :

			BeVerbose = 1;
			break;

		  default :

			if (processFName (*argv)) badArgs ();;
		}

		argv++;
		argc--;
	}

	// Adjustments ...
	if (REA [9] != '1' && RSY [9] != '1')
		// Make sure that journaling is only present when it makes sense
		JOU [9] = '0';

	if (ofname == NULL) {
		ofname = new char [16];
		strcpy (ofname, "side");
	}

	// Prepare the library name

	for (i = 0; i < NOPT-2; i++) {

		if ((options [i])[9] == '1')
			libindex [i] = 'y';
		else
			libindex [i] = 'n';
	}

	// Debug
	if ((options [NOPT-2])[9] == '1')
		libindex [i] = 'a';
	else if ((options [NOPT-2])[9] == '2')
		libindex [i] = 'b';
	else
		libindex [i] = '-';

	// Now the precision of time
	libindex [NOPT-1] = PRC [16];
	libindex [NOPT] = '\0';

        // Turn the index into a library name
        compressLibName ();

	if (ncfiles == 0) getFNames ();

	if (ncfiles == 0) excptn ("no files to compile");

	getwd (cd);     // Save the current directory

	if (chdir (ZZ_LIBPATH) < 0)
		excptn (form ("cannot access '%s'", ZZ_LIBPATH));

	str = form ("%s/liblock", ZZ_LIBPATH);

	// Now open the global lock file to be used as a semaphore
	if ((semfd = open (str, O_RDONLY, 0)) < 0) {
		if ((semfd = open (str, O_WRONLY+O_CREAT, 0660)) < 0)
			excptn ("cannot open liblock file");
	}

	// Hard lock: make sure nothing changes in the library for a while
	flock (semfd, LOCK_EX);

	// Check if the library exists
	if ((verfd = findLVer ()) < 0)
		excptn ("cannot open library version subdirectory");

	// Lock the library to make sure that another copy of mks doesn't
	// attempt to modify/remove it

	if (flock (verfd, LOCK_EX | LOCK_NB) < 0) {
		cout <<
	        "Waiting for a lock, conflicting compilation in progress ...\n";
		if (flock (verfd, LOCK_EX) < 0)
			cerr << pname << ": warning -- cannot lock '"
				<< ZZ_LIBPATH << '/' << libfname << "'\n";
		cout << "Lock acquired, proceeding ...\n";
	}

	flock (semfd, LOCK_UN);         // Others can proceed (as long as they
	close (semfd);                  // don't try to remove our version)

	// That's it. Now prepare the makefile for the library

	chdir (cd);                     // Resume original directory
	makeSmurph ();
}
