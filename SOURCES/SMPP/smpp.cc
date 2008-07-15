/* ooooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-2008   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ---------------------------------------------------------- */
/* This  is  a  preprocessor running after GNU cpp and before */
/* the C++ compiler whose purpose  is  to  turn  some  SMURPH */
/* constructs into C++ statements                             */
/* ---------------------------------------------------------- */

#include        "smpp.h"

#define INLINE  inline

void emitBuilder (SymDesc *co = NULL);

#ifdef DEBUG
void    debug (char *t) {

/* ----------------------- */
/* Print out debug message */
/* ----------------------- */

	char    *s;

	write (STDERR, CFName, strlen (CFName));
	s = form (":%1d: %s\n", LineNumber, t);
	write (STDERR, s, strlen (s));
}
#else
#define debug(t)
#endif

int Signature::pkc () {
        int nc;
        if (LastChar == NOTHING) {
		if (EndFF)
			LastChar = ENDFILE;
                else {
			if (BufPtr >= BufSize) {
			  BufPtr = 0;
			  nc = read (FilD, Buffer, IBSIZE);
			  if (nc < 0) {
                            perror ("cannot read from signature file");
                            exit (5);
			  }
			  if ((BufSize = nc) == 0) {
			    EndFF = YES;
			    return LastChar = ENDFILE;
			  }
			}
			LastChar = (int) (Buffer [BufPtr++]);
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

void Signature::flsig () {
	if (BufPtr) {
          if (write (FilD, Buffer, BufPtr) < BufPtr) {
            perror ("cannot write to signature file");
            exit (7);
	  }
	  BufPtr = 0;
	}
};

void Signature::outsig (char *t) {
	while (*t != '\0') {
	  if (BufPtr >= IBSIZE) flsig ();
	  Buffer [BufPtr++] = *t++;
	}
};

void Signature::outsig (char c) {
	if (BufPtr >= IBSIZE) flsig ();
	Buffer [BufPtr++] = c;
};

Signature::Signature () {

/* ------------------------------------ */
/* Unpacks the signature list to memory */
/* ------------------------------------ */

        char name [MAXKWDLEN], ***cl, **tl, **ttl;
	const char *s;
        int c, i, j, maxcom;

	debug ("Unpacking signature");
        LastChar = NOTHING;
	BufPtr = BufSize = 0;
        NSFiles = 0;
	if (SFName != NULL) {
	  FilD = open ("side.sgn", O_RDONLY);
	  if (FilD < 0) {
            perror ("cannot open signature file");
            exit (2);
	  }
	  EndFF = NO;
	  Buffer = new char [IBSIZE];
          CL = new char** [MaxNSFiles = 32];
	} else {
	  EndFF = YES;
          CL = NULL;
	  Buffer = NULL;
	}
	debug ("Stage 1");
        if (CL != NULL) {
	  debug (SFName);
	  // The signature is active
          tl = new char* [maxcom = 1024];
          for (i = j = 0; pkc () != ENDFILE; i++) {
	    // The library index
            if (nbl () == '\n') break;
            if (j < LNAMESIZE-1) LIndex [j++] = chr (); else chr ();
          }
          LIndex [j] = '\0';
	  debug ("Stage 2");
	  // Read the common list
          j = 0;
	  debug (form ("Before unpacking: %1d", NSFiles));
          while (1) {
            while ((c = nbl ()) == '\n') chr ();
            if (c == ENDFILE) break;
	    debug (form ("Nonblank: %c", c));
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
              if (i < MAXKWDLEN-1) name [i] = (char) chr (); else chr ();
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
	      debug ("Incremented");
	      debug (CL [NSFiles-1][0]);
              j = 0;
            }
          }
	  debug ("Closing FilD");
          ::close (FilD);
          delete tl;
	  // Check if the current file has an entry -- it shouldn't
          for (i = 0; i < NSFiles; i++)
            if (strcmp (CL [i][0], SFName) == 0) break;
          if (i < NSFiles) {
	    s = "Corrupted signature file\n";
	    write (STDERR, s, strlen (s));
	    exit (4);
	  }
	  debug ("Making room for new entry");
	  // Prepare room for the new entry at the end
	  if (NSFiles == MaxNSFiles) {
            // Increase the size
            cl = new char** [MaxNSFiles + 1]; // One more will do
            for (i = 0; i < MaxNSFiles; i++) cl [i] = CL [i];
            delete CL;
            CL = cl;
            MaxNSFiles++;
	  }
	  CL [NSFiles] = TL = new char* [TLMaxLength = 1024];
	  TL [0] = new char [strlen (SFName) + 1];
	  strcpy (TL [0], SFName);
	  TL [1] = NULL;
	  TLLength = 1;
	  NSFiles++;
        }
	debug ("Signature unpacked");
};

int Signature::update (const char *sym) {

/* --------------------------------------------------------------------- */
/* Adds a new symbol to the signature, returns YES if the symbol must be */
/* defined in the current module.                                        */
/* --------------------------------------------------------------------- */

	int i;
	char **tf;

	debug ("Updating signature");
	debug (sym);
	if (empty ()) return YES;
	debug ("Nonempty");
	for (i = 0; i < NSFiles; i++) {
	  tf = CL [i] + 1;  // Skip the module name
	  while (*tf != NULL) {
	    if (strcmp (*tf, sym) == 0) {
	      debug ("Symbol found");
	      return NO;
	    }
	    tf++;
	  }
	}
	debug ("Not found");
	// When we get here, the symbol hasn't been found
	if (TLLength == TLMaxLength - 1) {
	  // Increase the size of TL
	  tf = new char* [TLMaxLength + TLMaxLength];
	  for (i = 0; i < TLMaxLength; i++) tf [i] = TL [i];
	  delete TL;
	  CL [NSFiles-1] = TL = tf;
	  TLMaxLength += TLMaxLength;
	}
	TL [TLLength] = new char [strlen (sym) + 1];
	strcpy (TL [TLLength], sym);
	TL [++TLLength] = NULL;
	debug ("Added");
	return YES;
};

void    Signature::close () {

/* --------------------------------------------------- */
/* Closes the signature and writes it back to the file */
/* --------------------------------------------------- */

        int i;
        char **t;

	debug (form ("Close %1d", NSFiles));
        if (empty ()) return;
        FilD = open ("side.sgn", O_WRONLY+O_TRUNC);
        if (FilD < 0) {
          perror ("cannot open signature file for writing");
          exit (6);
        }
	BufPtr = 0;
	outsig (LIndex);
	debug (LIndex);
	outsig ('\n');
        for (i = 0; i < NSFiles; i++) {
                t = CL [i];
		debug ("NEW FILE");
                while (1) {
			outsig (*t);
			debug (*t);
                        if (*(++t) == NULL) break;
			outsig (' ');
                }
		debug ("End Line");
		outsig ('\n');
        }
	debug ("Flushing");
	flsig ();
        ::close (FilD);
	debug ("End close");
};
	
static INLINE int hash (const char *s) {

/* -------------------------------------------- */
/* Calculates the hash code of a symbol/keyword */
/* -------------------------------------------- */

	register int    res;

	res = *s++;
	if (*s == '\0') return (res & (HASHTS-1));
	res += *s++;
	if (*s == '\0') return (res & (HASHTS-1));
	return ((res + *s) & (HASHTS-1));
}

void    ioerror (int fd) {

/* ------------------------- */
/* Process i/o error (fatal) */
/* ------------------------- */

	char    *s;

	s = form ("i/o error on %s", fd == STDIN ? "stdin" : "stdout");
	perror (s);
	exit (2);
}

void    excptn (char *t) {

/* ----------------------- */
/* Memory allocation error */
/* ----------------------- */

	char    *s;

	s = form ("Memory allocation error: %s\n", t);
	write (STDERR, s, strlen (s));
	exit (3);
}

SymDesc::SymDesc (const char *sym, int hsh, int qual, int stts, int mode,
	int estat) {

/* ----------------------------------------- */
/* Constructor for a symbol dictionary entry */
/* ----------------------------------------- */

	next = SymTab [hsh];
	SymTab [hsh] = this;

	Name = new char [strlen (sym) + 1];
	strcpy (Name, sym);
	Type = qual;
	Status = stts;
	EStat = estat;
	PAnn = NO;
	Mode = mode;
	States = NULL;
	Parent = NULL;
}

SymDesc *addSym (const char *sym, int qual, int stts, int mode, int estat) {

/* --------------------------------- */
/* Adds a symbol to the symbol table */
/* --------------------------------- */

	SymDesc *cs;
	int     h;

	for (cs = SymTab [h = hash (sym)]; cs != NULL; cs = cs->next) {
	    if (strcmp (cs->Name, sym) == 0) {
	      if (cs->Type != qual) {
		xerror ("'%s' conflicitng declarations", sym);
		return (NULL);
	      }
	      if (cs->Status == DEFINED) {
		if (stts == DEFINED) {
		  xerror ("'%s' defined more than once", sym);
		  return (NULL);
		}
	      } else {
		cs->Status = stts;
	      }
	      if (cs->Mode != FREE) {
		if (mode != FREE && mode != cs->Mode) {
		  xerror ("'%s' mode conflicts with previous declaration", sym);
		  return (NULL);
		}
	      } else {
		cs->Mode = mode;
	      }
	      if (cs->EStat != estat) {
		if (cs->EStat == UNEXPOSABLE) {
		  xerror ("'%s' is unexposable", cs->Name);
		  return (NULL);
		}
		if (estat == ANNOUNCED) cs->EStat = ANNOUNCED;
	      }
	      return (cs);
	    }
	}

	return new SymDesc (sym, h, qual, stts, mode, estat);
}

INLINE SymDesc *getSym (const char *sym) {

/* ------------------------------------------------- */
/* Gets the symbol description from the symbol table */
/* ------------------------------------------------- */

	SymDesc *cs;
	int     h;

	for (cs = SymTab [h = hash (sym)]; cs != NULL; cs = cs->next)
	  if (strcmp (cs->Name, sym) == 0) return (cs);
	return (NULL);
}

inline  int     isDefined (SymDesc *ob, int qual, int stts) {

/* ---------------------------------------------- */
/* Checks whether the symbol is defined as needed */
/* ---------------------------------------------- */

	return (ob->Type == qual && (ob->Status == DEFINED ||
		stts == ANNOUNCED));
}

KeyDesc::KeyDesc (const char *s, KFUNC f, int arr) {

/* ---------------------------------------------------------- */
/* Adds   a  new  keyword  to  the  keyword  table  (used  at */
/* initialization)                                            */
/* ---------------------------------------------------------- */

	int     h;

	PFunct = f;
	Name = new char [strlen (s) + 1];
	Arrow = arr;
	strcpy (Name, s);
	h = hash (s);
	next = KeyTab [h];
	KeyTab [h] = this;
}

INLINE KFUNC getKey (char *s) {

/* ---------------------------------------- */
/* Gets the function for processing the key */
/* ---------------------------------------- */

	register int     h;
	register KeyDesc *ck;

	ck = KeyTab [h = hash (s)];
	for (; ck != NULL; ck = ck->next)
		if ((ArrowInFront == 2) == ck->Arrow && strcmp (ck->Name, s)
			== 0) return (ck->PFunct);
	return (NULL);
}
	
void    readBuf () {

/* ------------------------------------ */
/* Fills the input buffer with new data */
/* ------------------------------------ */

	int     n, m;

	if (Eof) return;                        // Don't read past EOF
	if (InP < OutP) {
		if (InP + 1 == OutP)            // The buffer is full
			return;
		if ((m = read (STDIN, InP, n=OutP-InP-1)) < 0) ioerror (STDIN);
		InP += m;
		if (m == 0) Eof = YES;
		return;
	}
	n = LimitP - InP;
	if (OutP == FirstP) {
		if (--n <= 0)           // The buffer is full
			return;
		if ((m = read (STDIN, InP, n)) < 0) ioerror (STDIN);
		InP += m;
		if (m == 0) Eof = YES;
		return;
	}
	if ((m = read (STDIN, InP, n)) < 0) ioerror (STDIN);
	if (m < n) {
		InP += m;
		if (m == 0) Eof = YES;
		return;
	}
	if (OutP == FirstP+1) {         // No second part
		InP = FirstP;
		return;
	}
	if ((m = read (STDIN, FirstP, n=OutP-FirstP-1)) < 0) ioerror (STDIN);
	InP = FirstP + m;
	return;
}

static INLINE int getC () {

/* ------------------------------------- */
/* Reads a new character from the buffer */
/* ------------------------------------- */

	if (BOLine = (CurrChar == '\n')) LineNumber++;;
	if (OutP == InP) {              // The buffer is empty
		readBuf ();
		if (Eof) return (END);
	}
	CurrChar = *OutP++;
	if (OutP >= LimitP) OutP = FirstP;
	return (CurrChar);
}

static INLINE int peekC () {

/* ----------------------------------------- */
/* Peeks at the next character in the buffer */
/* ----------------------------------------- */

	if (OutP == InP) {              // The buffer is empty
		readBuf ();
		if (Eof) return (END);
	}
	return (*OutP);
}

void    xerror (const char *t, ...) {

/* ----------------------- */
/* Print out error message */
/* ----------------------- */

	va_list	ap;
	char    *s;

	va_start (ap, t);

	ErrorsFound++;
	write (STDERR, CFName, strlen (CFName));
	s = form (":%1d: ", LineNumber);
	write (STDERR, s, strlen (s));
	s = vform (t, ap);
	write (STDERR, s, strlen (s));
	write (STDERR, "\n", 1);

	if (ErrorsFound < MAXERRCNT) return;
	// That's it: ignore the rest of the program
	write (STDERR, CFName, strlen (CFName));
	s = form (":%1d: too many errors\n", LineNumber);
	write (STDERR, s, strlen (s));
	while (getC () != END);
	exit (1);
}

int     startLkp () {

/* ------------------------------------ */
/* Starts advance lookup for characters */
/* ------------------------------------ */

	if (OutP == InP) {              // The buffer is empty
		readBuf ();
		if (Eof) return (END);
	}
	EnLkpPtr = OutP;
	return (YES);
}

static INLINE int lkpC () {

/* --------------------------------------- */
/* Lookup the next character in the buffer */
/* --------------------------------------- */

	char    c;

	if (EnLkpPtr == InP) {          // End of buffer reached
		if (Eof) return (END);
		readBuf ();
		if (EnLkpPtr == InP) {
			if (Eof) return (END);
			// Lookup exceeds buffer capacity
			xerror ("construct too long");
			return (ERROR);
		}
	}
	c = *EnLkpPtr++;
	if (EnLkpPtr >= LimitP) EnLkpPtr = FirstP;
	if (c == '\n') SkLineCnt++;
	return (c);
}

void    catchUp () {

/* ------------------------------------------------ */
/* Advance the out pointer to the lookahead pointer */
/* ------------------------------------------------ */

	while (OutP != EnLkpPtr) getC ();
}

void    flushBuf () {

/* ----------------------------------------------- */
/* Writes the output buffer to the standard output */
/* ----------------------------------------------- */

	if (FreeP == StartP) return;    // Nothing to write
	if (write (STDOUT, StartP, FreeP - StartP) < 0) ioerror (STDOUT);
	FreeP = StartP;
}

static inline void putC (char c) {

/* --------------------------------------- */
/* Writes one character to the output file */
/* --------------------------------------- */

	if (FreeP >= EndP) flushBuf ();
	*FreeP++ = c;
}

void    putC (const char *s) {

/* ---------------------------------- */
/* Writes a string to the output file */
/* ---------------------------------- */

	while (*s != '\0') putC (*s++);
}

void    finish () {

/* ----------------------- */
/* Completes the execution */
/* ----------------------- */

	SymDesc *qr;

	// Check if should append the starter function
	if ((qr = getSym ("Root")) != NULL && qr->Type == PROCESS &&
	    qr->Status == DEFINED) {
		putC ("void ZZRoot () { ZZ_Main = (Process*) new Root; ");
		putC ("((Process*)ZZ_Main)->zz_start (); ");
		putC ("((Root*)ZZ_Main)->setup (); }\n");
	}

	// Close the signature (updated copy gets written to disk)
	S->close ();
		
	flushBuf ();
	exit (ErrorsFound != 0);
}

static inline void genDCons (char *t) {

/* ------------------------- */
/* Emits a dummy constructor */
/* ------------------------- */

	putC (t); putC (" () {}; ");
}

int     pushDecl (SymDesc *co) {

/* ------------------------------------------ */
/* Push a new object declaration on the stack */
/* ------------------------------------------ */

	if (DeclStackDepth >= MSTACK) {
		xerror ("declarations overnested");
		return (NO);
	}

	DeclStack [DeclStackDepth] . CObject = CObject;
	DeclStack [DeclStackDepth] . CObjectBC = CObjectBC;

	DeclStackDepth ++;

	CObject = co;
	CObjectBC = BraceLevel;
	return (YES);
}

void    processLineCommand () {

/* ---------------------------------------------------------- */
/* Processes the line command of the form # ddd "sssss" [nnn] */
/* ---------------------------------------------------------- */

	int     nln,    // New line number
		bpt,    // File name buffer pointer
	        c;

	// Copy the '#' character
	putC ('#');
	if (startLkp () == END) {
		xerror ("input file ends prematurely");
		finish ();
	}

	// Line number

	while ((c = lkpC ()) != '\n') if (!isspace (c)) break;
	if (c == '\n' || ! isdigit (c)) goto Copy;
	for (nln = c - '0'; (c = lkpC ()) && isdigit (c); nln = nln*10+(c-'0'));

	// File name

	while (isspace (c) && c != '\n') c = lkpC ();
	if (c != '"') goto Copy;

	for (bpt = 0; ;) {
		c = lkpC ();
		if (c == '\n' || c == END || c == ERROR) goto Copy;
		if (c == '"') break;
		if (bpt < MFNLEN-1) CFName [bpt++] = c;
	}
	CFName [bpt] = '\0';

	// We don't care about anything else

	LineNumber = nln-1;
	// This number will be incremented properly when we process the
	// end of line

Copy:   // Copy the rest of the line until EOL

	while ((c = getC ()) != '\n') {
		if (c == END) finish ();
		putC (c);
	}
	putC (c);
}

int     getKeyword (char *kbuf, int fc = 0, int mswd = 0) {

/* ---------------------------------------------------------- */
/* Gets  a  keyword  into  buffer kbuf. Returns the non-blank */
/* delimiter code, ERROR or END.                              */
/* ---------------------------------------------------------- */

	int     i, c;

	// fc is the first character. If it is 0, we assume that we have to
	// start from scratch

	if (fc == 0) {
		// Skip white spaces
		while (1) {
			fc = lkpC ();
			if (!isspace (fc)) break;
		}
	}

	if (fc == END || fc == ERROR) return (fc);

	if (!isalpha(fc) && fc != '_' && fc != '$'
		// This one solves a problem with constructs like ::*...
		// which, of course, are legitimate
	  && fc != '*'
	    && (!mswd || !isdigit(fc))) {

		xerror ("identifier expected");
		return (ERROR);
	}

	kbuf [0] = fc;

	for (i = 1; ; i++) {
		c = lkpC ();
		if (!isalnum (c) && c != '_' && c != '$') break;
		if (i > MAXKWDLEN) {
			xerror ("identifier too long");
			return (ERROR);
		}
		kbuf [i] = c;
	}

	kbuf [i] = '\0';
	while (isspace (c)) c = lkpC ();
	return (c);
}

int     getArg (char *kbuf) {

/* ---------------------------------------------------------- */
/* Gets  an argument (which can be an expression) into buffer */
/* kbuf. Returns the non-blank delimiter code, ERROR or END.  */
/* ---------------------------------------------------------- */

	int     i, c, pc, ws;

	// Skip white spaces
	while (1) {
		c = lkpC ();
		if (!isspace (c)) break;
	}

	if (c == END || c == ERROR) return (c);

	if (c == ',' || c == ')' || c == ';') {
		xerror ("argument missing");
		return (ERROR);
	}

	kbuf [0] = c;
	if (c == '"' || c == '\'')
		ws = c;                 // Within string
	else
		ws = 0;

	if (c == '(')
		pc = 1;                 // Parentheses count
	else
		pc = 0;
	
	for (i = 1; ; ) {
		c = lkpC ();
		if (ws) {
			if (c == '\\') {
				if (i > MAXKWDLEN) {
					xerror ("argument too long");
					return (ERROR);
				}
				kbuf [i++] = c;
				c = lkpC ();
			} else {
				if (c == ws) ws = 0;
			}
		} else {
			if (c == ',' || c == ';') {
				if (pc == 0) break;
			} else if (c == ')') {
				if (pc-- == 0) break;
			} else if (c == '(') {
				pc++;
			} else if (c == '\'' || c == '"') {
				ws = c;
			}
		}
			
		if (c == ERROR) return (NO);
		if (c == END) return (c);
		if (i > MAXKWDLEN) {
			xerror ("argument too long");
			return (ERROR);
		}
		kbuf [i++] = c;
	}

	kbuf [i] = '\0';
	while (isspace (c)) c = lkpC ();
	return (c);
}

int     isState (char *st) {

/* --------------------------------------------------------- */
/* Checks if the state 'st' occurs on the process state list */
/* --------------------------------------------------------- */

	char    **sl;

	if ((sl = CodeObject->States) == NULL) {
		xerror ("no states declared for '%s'", CodeObject->Name);
		return (NO);
	}

	for ( ; *sl != NULL; sl++)
		if (strcmp (*sl, st) == 0) break;

	if (*sl == NULL) {
	  xerror ("'%s' is not a state for '%s'", st, CodeObject->Name);
	  return (NO);
	}

	return (YES);
}

char    *isStateP (char *st) {

/* ------------------------------------------------------------ */
/* Checks if the state 'st' occurs on the process state list or */
/* on the list of its closest parent  that has a nonempty state */
/* list. Returns the process (parent) type name or NULL.        */
/* ------------------------------------------------------------ */

	SymDesc *p;
	char    **sl;

	for (p = CodeObject; p != NULL && (sl = p->States) == NULL;
          p = p->Parent);
	if (sl == NULL) {
	  xerror ("no states visible from '%s'", CodeObject->Name);
	  return (NULL);
	}

	for ( ; *sl != NULL; sl++)
		if (strcmp (*sl, st) == 0) break;

	if (*sl == NULL) {
          xerror ("'%s' is not a legal state for '%s'", st, CodeObject->Name);
	  return (NULL);
	}

	return (p->Name);
}

int     processWait (int del) {

/* --------------------------------------- */
/* Expands the ...wait (a, b) ... sequence */
/* --------------------------------------- */

	// wait (a, b) expands into 'wait (a, b)' (stupid, isn't it?)

	char    arg [MAXKWDLEN+1];
	int     lc;

	if (CodeType != PROCESS) return (NO);
	if (del != '(') return (NO);

	// Get the first argument

	lc = getArg (arg);
	if (lc == END || lc == ERROR) return (NO);
	if (lc != ',') {
		xerror ("event identifier missing");
		return (NO);
	}

	putC ("wait (");
	putC (arg);
	putC (", ");

	// The second argument should be an identifier

	lc = getKeyword (arg);
	if (lc == ERROR) return (NO);
	if (lc == END) {
WaitErr:
		xerror ("file ends in the middle of 'wait'");
		return (NO);
	}

	if (!isState (arg)) return (NO);
	putC (arg);

	if (lc == ',') {
		if (! Tagging) {
			xerror ("three-argument wait requires '-p'");
			return (NO);
		}
		// The third argument, if present, can be an expression
		lc = getArg (arg);
		if (lc == ERROR) return (NO);
		if (lc == END) goto WaitErr;
		putC (',');
		putC (arg);
	}
		
	if (lc != ')') {
		xerror ("too many arguments to 'wait'");
		return (NO);
	}

	putC (')');

	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processTerminate (int del) {

/* -------------------- */
/* Expands 'terminate;' */
/* -------------------- */

	// terminate; and terminate (); expands into 'terminate (); sleep;'

	if (!CodeType) return (NO);
	if (del != ';') {
		if (del != '(') return (NO);
		for (del = lkpC (); isspace (del); del = lkpC ());
		if (del != ')') return (NO);
		for (del = lkpC (); isspace (del); del = lkpC ());
		if (del != ';') return (NO);
	}

	putC ("do { terminate (); return; } while (0);");
	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processGetDelay (int del) {

/* --------------------------------------------- */
/* Expands the ...->getDelay (a, b) ... sequence */
/* --------------------------------------------- */

	// ->getDelay (a, b) expands into:
        // ->zz_getdelay (a, ((typeof (a))(0))->b)

	char    arg [MAXKWDLEN+1], sta [MAXKWDLEN+1];
	int     lc;

	if (del != '(') return (NO);

	// Get the first argument (this should be a process pointer)

	lc = getArg (arg);
	if (lc == END || lc == ERROR) return (NO);
	if (lc != ',') {
		return (NO);
	}

	putC ("zz_getdelay (");
	putC (arg);
	putC (", ");

	// The second argument should be an identifier

	lc = getKeyword (sta);
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'getDelay'");
		return (NO);
	}
        if (lc != ')') {
                xerror ("too many arguments to 'getDelay'");
                return (NO);
        }
        putC ("((typeof (");
	putC (arg);
        putC ("))(0))->");
	putC (sta);

	putC (')');

	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processSetDelay (int del) {

/* ------------------------------------------------ */
/* Expands the ...->setDelay (a, b, c) ... sequence */
/* ------------------------------------------------ */

	// ->getSelay (a, b, c) expands into:
        // ->zz_setdelay (a, ((typeof (a))(0))->b, c)

	char    arg [MAXKWDLEN+1], sta [MAXKWDLEN+1];
	int     lc;

	if (del != '(') return (NO);

	// Get the first argument (this should be a process pointer)

	lc = getArg (arg);
	if (lc == END || lc == ERROR) return (NO);
	if (lc != ',') {
		return (NO);
	}

	putC ("zz_setdelay (");
	putC (arg);
	putC (", ");

	// The second argument should be an identifier

	lc = getKeyword (sta);
	if (lc == ERROR) return (NO);
	if (lc == END) {
SetdErr:
		xerror ("file ends in the middle of 'setDelay'");
		return (NO);
	}
        if (lc != ',') {
                xerror ("too few arguments to 'setDelay'");
                return (NO);
        }
        putC ("((typeof (");
	putC (arg);
        putC ("))(0))->");
	putC (sta);
        putC (", ");

        lc = getArg (arg);
	if (lc == ERROR) return (NO);
	if (lc == END) goto SetdErr;
        if (lc != ')') {
                xerror ("too many arguments to 'setDelay'");
                return (NO);
        }
        putC (arg);

	putC (')');

	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processIdentify (int del) {

/* ---------------------------------- */
/* Expands the `identify t;' sequence */
/* ---------------------------------- */

	// identify (....);
	// identify ...;
	// identify "...";
	//
	// expands into: 'ProtocolId = "...";'

	int     lc, pc;

	if (CodeType) return (NO);

	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of 'identify'");
		return (NO);
	}

	if (Identified) {
		xerror ("'identify' may occur only once in a protocol file");
		return (NO);
	}

        if (!(S->update ("ProtocolId"))) {
                xerror ("'identify' occurred in a previous protocol file");
                return (NO);
        }

	Identified = YES;

	putC ("const char *ProtocolId = \"");
	if (lc == '(') {
		pc = 1;
	} else {
		pc = 0;
		if (lc == '"') {
			pc = -1;
		} else {
			putC (lc);
		}
	}

	while (1) {
	    switch (lc = lkpC ()) {
		case ';' :
			if (pc) {
				putC (lc);
				continue;
			}
			goto Done;
		case '(' :
			if (pc > 0) {
				pc++;
				continue;
			}
			putC (lc);
			continue;
		case ')' :
			if (pc > 0) {
				if (--pc == 0) {
					lc = lkpC ();
					goto Complete;
				}
				continue;
			}
			putC (lc);
			continue;
		case ' ' :
			if (pc) {
				putC (lc);
				continue;
			}
			goto Complete;
		case '\\' :
			lc = lkpC ();
			if (lc == '"') putC ('\\');
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			putC (lc);
			continue;
		case '"' :
			if (pc < 0) {
				lc = lkpC ();
				goto Complete;
			}
			putC ('\\');
			putC ('"');
			continue;
		case ERROR :
			return (NO);
		case END :
			goto Endfile;
		default:
			putC (lc);
			continue;
	    }
	}

Complete:       // Expect only spaces terminated by ';'

	while (isspace (lc)) lc = lkpC ();
	if (lc == ERROR) return (NO);
	if (lc != ';') {
		xerror ("identify syntax error");
		return (NO);
	}

Done:   putC ("\";");
	catchUp ();
	return (YES);
}

int     processState (int del) {

/* ------------------------------------- */
/* Expands the `state nnnnnn :' sequence */
/* ------------------------------------- */

	// state sym :
	//
	// expands into: 'break; case sym :'

	char    arg [MAXKWDLEN+1];
	int     lc;

	if (! CodeType) return (NO);

	// Get the state identifier

	lc = getKeyword (arg, del);
	if (lc == END || lc == ERROR) return (NO);
	if (lc != ':') {
		xerror ("state syntax error");
		return (NO);
	}

	if (!isState (arg)) return (NO);

	if (CodeWasState) {
		putC ("break; ");
	} else {
		if (CodeType == PROCESS)
			putC ("switch (TheState) { ");
		else
			putC ("switch (TheObserverState) { ");
			CodeWasState = YES;
	}
	putC ("case ");
	putC (arg);
	putC (":");

	putC (" __state_label_");
	putC (arg);
	putC (":");

	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processTransient (int del) {

/* ------------------------------------------------------- */
/* Expands the controversial `transient nnnnnn :' sequence */
/* ------------------------------------------------------- */

	char    arg [MAXKWDLEN+1];
	int     lc;

	// transient state :
	//
	// expands into: 'case state :'

	if (! CodeType) return (NO);
	if (!CodeWasState)
		return processState (del);

	// Get the state identifier
	lc = getKeyword (arg, del);
	if (lc == END || lc == ERROR) return (NO);
	if (lc != ':') {
		xerror ("state syntax error");
		return (NO);
	}

	if (!isState (arg)) return (NO);

	putC ("case ");
	putC (arg);
	putC (":");

	putC (" __state_label_");
	putC (arg);
	putC (":");

	catchUp ();     // Catch up with the lookahead pointer
	return (YES);
}

int     processStates (int del) {

/* ------------------------------- */
/* Expands `states {...}' sequence */
/* ------------------------------- */

	// states {s0, ..., sn};
	//
	// expands into: 'enum {s0, ..., sn};'

	char    arg [MAXKWDLEN+1];
	int     i, lc, ns, sls;
	char    **sl, **tmp;
	
	lc = del;
	if (lc != '{') return (NO);
	if (CObject == NULL || (CObject->Type != PROCESS && CObject->Type !=
	  OBSERVER) || BraceLevel != CObjectBC+1) {
	    xerror ("'states' declaration outside of process/observer scope");
	    return (NO);
	}

	if (CObject->States != NULL) {
		xerror ("duplicate 'states' declaration for '%s'",
			CObject->Name);
		return (NO);
	}

	sls = 16;       // Size of the state table
	ns = 0;         // The number of states

	for (sls = 15, ns = 0, sl = new char* [sls+1]; ; ) {
		lc = getKeyword (arg);
		if (lc == ERROR) { delete (sl); return (NO); }
		if (lc == END) {
		    xerror ("file ends in the middle of 'states' declaration");
		    delete (sl);
		    return (NO);
		}

		// Check against double occurrence
		for (i = 0; i < ns; i++) {
		  if (strcmp (sl [i], arg) == 0) {
		    xerror ("state '%s' multiply defined", arg);
		    delete (sl);
		    return (NO);
		  }
		}
		
		if (ns >= sls) {
			// Reallocate the state array
			tmp = new char* [ns];
			for (i = 0; i < ns; i++) tmp [i] = sl [i];
			delete (sl);
			sl = new char* [(sls = sls+sls+1) + 1];
			for (i = 0; i < ns; i++) sl [i] = tmp [i];
		}

		sl [ns] = new char [strlen (arg) + 1];
		strcpy (sl [ns], arg);
		ns++;

		if (lc == '}') break;
		if (lc != ',') {
			xerror ("illegal delimiter in 'states' declaration");
			delete (sl);
			return (NO);
		}
	}
	sl [ns] = NULL; // Sentinel
	// Skip to the semicolon which must be the next nonspace character
	del = lkpC ();
	while (isspace (del)) del = lkpC ();
	if (del != ';') {
		xerror ("states declaration not terminated with ';'");
		delete (sl);
		return (NO);
	}

	// Now expand the declaration
	CObject->States = sl;
	if (ns) {
		putC ("enum {");
		for (tmp = sl; ;) {
			putC (*tmp);
			tmp++;
			if (*tmp == NULL) {
				putC ("}; ");
				break;
			} else {
				putC (", ");
			}
		}
	}

	putC ("static const int zz_ns");
	putC (SPref);
	putC ("; ");
	putC ("static const char *zz_sl");
	putC (SPref);
	putC (form ("[%1d];", ns));
	putC ("virtual const char *zz_sn (int n) { return ((n > zz_ns");
	putC (SPref);
	putC (") ? \"undefined\" : zz_sl");
	putC (SPref);
	putC (" [n]); }; ");

	catchUp ();
	return (YES);
}

int     processSleep (int del) {

/* ------------- */
/* Expands sleep */
/* ------------- */

	// sleep;
	//
	// expands into 'return;'

	if (!CodeType) {
		// Out of process, use longjmp
		if (del != ';')
			// This may be something weird
			return NO;
		putC ("longjmp (zz_waker, 1);");
	} else {
		if (del != ';') {
			xerror ("'sleep' syntax error");
			return (NO);
		}

		putC ("return;");
	}

	catchUp ();
	return (YES);
}

int     processProceed (int del) {

/* --------------------------------- */
/* Expands the `proceed s;' sequence */
/* --------------------------------- */

	// proceed state;
	// proceed (state);
	//
	// within a process expands into:
	//    do { zz_AI_timer.zz_proceed (state); return; } while (0);
	//
	// within an observer expands into:
	//    do { TheObserverState = state; goto __state_label_state; }
	//	while (0);

	char    arg [MAXKWDLEN+1], sarg [MAXKWDLEN+1];
	int     lc, pc;

	if (! CodeType) return (NO);

	lc = del;
	sarg [0] = '\0';

	if (lc == '(') {
		pc = 1;
		while (1) {
			if ((lc = lkpC ()) == '(') pc++;
			if (!isspace (lc)) break;
		}
	} else {
		pc = 0;
	}

	if (lc == ERROR) return (NO);
	if (lc == END) {
ProcErr:
		xerror ("file ends in the middle of 'proceed'");
		return (NO);
	}

	lc = getKeyword (arg, lc);

	if (lc == ',') {
		// Two argument proceed
		if (CodeType != PROCESS) {
			xerror ("two-argument proceed in an observer");
			return (NO);
		}
		if (!Tagging) {
			xerror ("two-argument proceed requires '-p'");
			return (NO);
		}
		lc = getArg (sarg);
		if (lc == ERROR) return (NO);
		if (lc == END) goto ProcErr;
	}

	while (pc) {
		if (lc != ')') {
			xerror ("'proceed' syntax error");
			return (NO);
		}
		pc--;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
	}

	if (lc != ';') {
		xerror ("semicolon missing after 'proceed'");
		return (NO);
	}

	if (!isState (arg)) return (NO);

	putC ("do { ");

	if (CodeType == PROCESS) {
		putC ("zz_AI_timer.zz_proceed (");
		putC (arg);
		if (sarg [0] != '\0') {
			putC (',');
			putC (sarg);
		}
		putC ("); return;");
	} else {
		// The observer version
		putC ("TheObserverState = ");
		putC (arg);
		putC ("; goto __state_label_");
		putC (arg);
		putC (";");
	}

	putC (" } while (0);");

	catchUp ();
	return (YES);
}

int     processSkipto (int del) {

/* -------------------------------- */
/* Expands the `skipto s;' sequence */
/* -------------------------------- */

	// skipto state;
	// skipto (state);
	//
	// within a process expands into:
	//     do { zz_AI_timer.zz_skipto (state); return; } while (0);

	char    arg [MAXKWDLEN+1];
	int     lc, pc;

	if (CodeType != PROCESS) return (NO);

	lc = del;

	if (lc == '(') {
		pc = 1;
		while (1) {
			if ((lc = lkpC ()) == '(') pc++;
			if (!isspace (lc)) break;
		}
	} else {
		pc = 0;
	}

	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'skipto'");
		return (NO);
	}

	lc = getKeyword (arg, lc);

	if (!isState (arg)) return (NO);
	putC ("do { zz_AI_timer.zz_skipto (");
	putC (arg);
	if (lc == ',') {
		if (!Tagging) {
			xerror ("two-argument skip requires '-p'");
			return (NO);
		}
		lc = getArg (arg);
		putC (',');
		putC (arg);
	}

	while (pc) {
		if (lc != ')') {
			xerror ("'skipto' syntax error");
			return (NO);
		}
		pc--;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
	}

	if (lc != ';') {
		xerror ("semicolon missing after 'skipto'");
		return (NO);
	}

	putC ("); return; } while (0);");

	catchUp ();
	return (YES);
}

int     processSameas (int del) {

/* -------------------------------- */
/* Expands the `sameas s;' sequence */
/* -------------------------------- */

	// sameas state;
	// sameas (state);
	//
	// expands into:
	//    do {
	//	The[Observer]State = state;
	//	unwait (); // Process only
	//	goto __state_label_state;
	//    }	while (0);

	char    arg [MAXKWDLEN+1], sarg [MAXKWDLEN+1];
	int     lc, pc;

	if (! CodeType) return (NO);

	lc = del;
	sarg [0] = '\0';

	if (lc == '(') {
		pc = 1;
		while (1) {
			if ((lc = lkpC ()) == '(') pc++;
			if (!isspace (lc)) break;
		}
	} else {
		pc = 0;
	}

	if (lc == ERROR) return (NO);
	if (lc == END) {
ProcErr:
		xerror ("file ends in the middle of 'sameas'");
		return (NO);
	}

	lc = getKeyword (arg, lc);

	while (pc) {
		if (lc != ')') {
			xerror ("'sameas' syntax error");
			return (NO);
		}
		pc--;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
	}

	if (lc != ';') {
		xerror ("semicolon missing after 'sameas'");
		return (NO);
	}

	if (!isState (arg)) return (NO);

	putC ("do { The");

	if (CodeType != PROCESS)
		putC ("Observer");

	putC ("State = ");
	putC (arg);
	putC ("; ");

	if (CodeType == PROCESS)
		putC ("unwait (); ");

	putC ("goto __state_label_");
	putC (arg);
	putC ("; } while (0);");

	catchUp ();
	return (YES);
}

int     process_nt (
		int del,		// Delimiter
		int xt,			// Type ordinal
		const char *ki,		// Declaration keyword
		const char *ky,		// Base type name
		int es,			// Exposure status
		int si			// State inheritance
	) {

/* --------------------------------------------------------- */
/* Expands a simple type declaration, e.g., eobject, station */
/* --------------------------------------------------------- */

	char    arg [MAXKWDLEN+1], par [MAXKWDLEN+1];
	int     lc, mode, fsup, empty;
	SymDesc *ob, *co;

	// something A [virtual|abstract];
	// expands into 'class A;'
	//
	// something A [virtual|abstract] : symlist {
	// eobject A [virtual|abstract] {
	//
	// expands into:
	//
	// class A : symlist {          // public added to each symbol
	//              public:
	//              virtual const char *getTName () {
	//                  return ("A");
	//              };

	lc = del;
	empty = NO;
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of '%s' declaration", ki);
		return (NO);
	}
	if (lc == ERROR || !isalpha (lc) && lc != '_' && lc != '$') return (NO);

	lc = getKeyword (arg, lc);

	// Determine the delimiter
	if (lc == ';') {
		// Just announcing the type
		if (!addSym (arg, xt, ANNOUNCED, FREE, es)) return (NO);
Announce:
		if (CodeType) {
CodeError:
			xerror ("%s declaration within a code method", ki);
			return (NO);
		}
		putC ("class ");
		putC (arg);
		putC (';');
		catchUp ();
		return (YES);
	}

	if (lc != ':' && lc != '{') {

		// Check if followed by 'virtual' or 'abstract'
		lc = getKeyword (par, lc);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;

		if (strcmp (par, "virtual") == 0)
			mode = VIRTUAL;
		else if (strcmp (par, "abstract") == 0)
			mode = ABSTRACT;
		else {
Synerror:
			xerror ("'%s' declaration syntax error", ki);
			return (NO);
		}

		if (lc == ';') {
			if (!addSym (arg, xt, ANNOUNCED, mode, es))
				return (NO);
			goto Announce;
		}
		if (lc != ':' && lc != '{') goto Synerror;
	} else {
		mode = REAL;
	}

	if (!(co = addSym (arg, xt, DEFINED, mode, es))) return (NO);

	if (CodeType)
		goto CodeError;

	putC ("class "); putC (arg);

	fsup = YES;
	if (lc == ':') {
		// Derived from something: go through the list of identifiers
		// and append them to the declaration
		while (1) {
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if (strcmp (par, arg) == 0) {
				xerror ("'%s' refers to itself", par);
				goto Assumereal;
			}
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
				goto Assumereal;
			}
			if (!isDefined (ob, xt, DEFINED)) {
				xerror ("'%s' not a defined %s", par, ki);
				goto Assumereal;
			}
			if (fsup) {
				putC (" : ");
				fsup = NO;
				// The first keyword
				if (ob->Mode == VIRTUAL) {
					if (mode != VIRTUAL) {
						// Force the REAL status
						putC ("public ");
						putC (ky);
						putC (", ");
					} 
				} else {
					if (mode == VIRTUAL) {
xerror ("'%s' declared virtual but '%s' is not virtual", arg, par);
						goto Assumereal;
					}
					if (si) {
						// Inherit states
						if (ob->States)
							co->Parent = ob;
						else
							co->Parent = ob->Parent;
					}
				}
			} else {
				if (ob->Mode != VIRTUAL) {
				    xerror ("'%s' is not virtual", par);
				}
			}
Assumereal:
			putC ("public ");
			putC (par);
			if (lc == ',') {
				putC (',');
			} else if (lc == ';') {
				empty = YES;
				break;
			} else if (lc == '{') {
				break;
			} else goto Synerror;
		}
	} else {
		// Derived from base or from nothing
		if (mode != VIRTUAL) {
			putC (" : public ");
			putC (ky);
		}
	}

	putC (" { public: ");

	if (es == EXPOSABLE)
		// This test is confusing, but es == UNEXPOSABLE also identifies
		// Message and Packet
        	genDCons (arg);

	if (!empty) {
		if (pushDecl (co) == NO) return (NO);
		BraceLevel++;
	}

	if (mode != VIRTUAL) {
		if (es == EXPOSABLE) {
			// This won't hurt abstract types, will it?
			putC ("virtual const char *getTName () { return (\"");
			putC (arg);
			putC ("\"); }; ");
		} else {
			// This is for packets and messages
			putC ("virtual int frameSize () { return (sizeof (");
			putC (arg);
			putC (")); };");
		}
	}
	catchUp ();
	if (empty) {
		putC ("}; ");
		emitBuilder (co);
	}
	return (YES);
}

int     processEobject (int del) {

	return process_nt (del, EOBJECT, "eobject", "EObject", EXPOSABLE, NO);
}

int     processStation (int del) {

	return process_nt (del, STATION, "station", "Station", EXPOSABLE, NO);
}

int     processObserver (int del) {

	return process_nt (del, OBSERVER, "observer", "Observer", EXPOSABLE,
		YES);
}

int     processMessage (int del) {

	int res;

	res = process_nt (del, MESSAGE, "message", "Message", UNEXPOSABLE, NO);
	if (res && NoClient) {
		xerror ("'message' illegal with -C (no Client)");
		return NO;
	}

	return res;
}

int     processPacket (int del) {

	int res;

	res = process_nt (del, PACKET, "packet", "Packet", UNEXPOSABLE, NO);
	if (res && NoClient) {
		xerror ("'packet' illegal with -C (no Client)");
		return NO;
	}

	return res;
}

int     processLink (int del) {

	int res;

	res = process_nt (del, LINK, "link", "Link", EXPOSABLE, NO);
	if (res && NoLink) {
		xerror ("'link' illegal with -L (no links)");
		return NO;
	}

	return res;
}

int     processRFChannel (int del) {

	int res;

	res = process_nt (del, RFCHANNEL, "rfchannel", "RFChannel", EXPOSABLE,
		NO);
	if (res && NoRFC) {
		xerror ("'rfchannel' illegal with -X (no RF channels)");
		return NO;
	}

	return res;
}

int     processProcess (int del) {

/* ------------------------------------ */
/* Expands the Process type declaration */
/* ------------------------------------ */

	char    arg[MAXKWDLEN+1], par[MAXKWDLEN+1], sar[MAXKWDLEN+1];
	int     lc, mode, fsup, empty;
	SymDesc *ob, *co;

	// process A [virtual];                 -> class A;
	// process A [virtual] [: symlist] [(ST [, PR])] {
	//
	// full expansion:
        //
	// int zz_A_prcs;
	//
	// class A : symlist {          // public inserted before each symbol
	//                              // absent symlist -> 'public Process'
	//   public: ST *S;             // Only if ST specified
	//           PR *F;             // Only if PR specified
	//
	//   A () {
	//      S=(ST*)TheStation;      // Only if ST specified
	//      F=(PR*)TheProcess;      // Only if PR specified
	//      zz_typeid = (void*) (&zz_A_prcs);
	//   };
	//   virtual const char *getTName () {
	//      return ("A");
	//   };

	if (CodeType) {
		xerror ("process declaration within a code method");
		return (NO);
	}
	lc = del;
	empty = NO;
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of process declaration");
		return (NO);
	}
	if (lc == ERROR || !isalpha (lc) && lc != '_' && lc != '$') return (NO);

	// This is the new process type name
	lc = getKeyword (arg, lc);

	// Determine the delimiter

	if (lc == ';') {
		// Just announcing the type
		if (!addSym (arg, PROCESS)) return (NO);
Announce:
		putC ("class ");
		putC (arg);
		putC (';');
		catchUp ();
		return (YES);
	}

	if (lc != ':' && lc != '{' && lc != '(') {

		// Check if followed by 'virtual'
		lc = getKeyword (par, lc);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;

		if (strcmp (par, "virtual") == 0)
			mode = VIRTUAL;
		else if (strcmp (par, "abstract") == 0)
			mode = ABSTRACT;
		else {
Synerror:
			xerror ("'process' declaration syntax error");
			return (NO);
		}

		if (lc == ';') {
			if (!addSym (arg, PROCESS, ANNOUNCED, mode))
				return (NO);
			goto Announce;
		}
	} else {
		mode = REAL;
	}

	if (!(co = addSym (arg, PROCESS, DEFINED, mode))) return (NO);

	if (mode == REAL) {
	    // I guess, this isn't needed for abstract processes as well as
	    // for virtual ones
	    strcpy (par, "zz_");
	    strcat (par, arg);
	    strcat (par, "_prcs");
	    if (!(S->update (par))) putC ("extern ");
	    putC ("int ");
	    putC (par);
	    putC ("; ");
	}

	putC ("class ");
	putC (arg);

	fsup = YES;
	if (lc == ':') {
		// Derived from something: go through the list of identifiers
		// and append them to the declaration
		while (1) {
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if (strcmp (par, arg) == 0) {
				xerror ("'%s' refers to itself", par);
				goto Assumereal;
			}
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
				goto Assumereal;
			}
			if (!isDefined (ob, PROCESS, DEFINED)) {
				xerror ("'%s' not a defined Process", par);
				goto Assumereal;
			}
			if (fsup) {
				putC (" : ");
				fsup = NO;
				// The first keyword
				if (ob->Mode == VIRTUAL) {
					if (mode != VIRTUAL) {
						// Force the REAL status
						putC ("public Process, ");
					} 
				} else {
					if (mode == VIRTUAL) {
xerror ("'%s' declared virtual but '%s' is not virtual", arg, par);
						goto Assumereal;
					}
					if (ob->States)
						co->Parent = ob;
					else
						co->Parent = ob->Parent;
				}
			} else {
				if (ob->Mode != VIRTUAL) {
				    xerror ("'%s' is not virtual", par);
				}
			}
Assumereal:
			putC ("public ");
			putC (par);
			if (lc == ',') {
				putC (',');
			} else if (lc == '{' || lc == '(') {
				break;
			} else if (lc == ';') {
				empty = YES;
				break;
			} else goto Synerror;
		}
	} else {
		// Derived from Process or from nothing
		if (mode != VIRTUAL) putC (" : public Process");
	}

	putC (" { public: ");

	// Flag parent type and station type as undefined
	par [0] = sar [0] = '\0';

	if (lc == '(') {
		if (mode == VIRTUAL) {
			xerror ("a virtual process cannot specify "
				"father process or owning station");
			return (NO);
		}
		// Owning station type goes first
		lc = getKeyword (sar);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if ((ob = getSym (sar)) == NULL) {
			xerror ("'%s' undefined", sar);
		} else if (!isDefined (ob, STATION, ANNOUNCED)) {
			xerror ("'%s' not defined as 'Station'", sar);
		} else if (ob->Mode == VIRTUAL) {
			xerror ("'%s' is virtual", sar);
		}

		// Continue with this error
		if (lc == ',') {
			// Parent process goes next
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
			} else if (!isDefined (ob, PROCESS, ANNOUNCED)) {
			   xerror ("'%s' not defined as 'Process'", par);
			} else if (ob->Mode == VIRTUAL) {
			   xerror ("'%s' is virtual", par);
			}
		}
		if (lc != ')') goto Synerror;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
		if (lc == ';') {
			// Assume empty body
			empty = YES;
		} else if (lc != '{') goto Synerror;
	}

	if (!empty) {
		if (pushDecl (co) == NO) return (NO);
		BraceLevel++;
	}

	if (sar [0] != '\0') {
		// Owning station defined (illegal for a virtual process)
		putC (sar);
		putC (" *S; ");
	}
	if (par [0] != '\0') {
		// Parent process defined (see above)
		putC (par);
		putC (" *F; ");
	}
	if (mode != VIRTUAL) {
		putC (arg);
		putC (" () { ");
		if (sar [0] != '\0') {
			// Owning station defined
			putC ("S = (");
			putC (sar);
			putC ("*)TheStation; ");
		}
		if (par [0] != '\0') {
			// Parent process defined
			putC ("F = (");
			putC (par);
			putC ("*)TheProcess; ");
		}
		if (mode == REAL) {
			putC ("zz_typeid = (void*) (&zz_");
			putC (arg);
			putC ("_prcs); ");
		}

		putC (" }; virtual const char *getTName () { return (\"");
		putC (arg);
		putC ("\"); };");
	} else {
		mode++;
        	genDCons (arg);
	}
	catchUp ();
	if (empty) {
		putC ("}; ");
		emitBuilder (co);
	}
	return (YES);
}

int     processMailbox (int del) {

/* ------------------------------------ */
/* Expands the Mailbox type declaration */
/* ------------------------------------ */

	char    arg[MAXKWDLEN+1], par[MAXKWDLEN+1];
	int     lc, mode, fsup, empty;
	SymDesc *ob, *co;

	// mailbox A [virtual];
	// mailbox A [virtual] [: symlist] [(TYPE)] {
	//
	// full expansion:
	//
	// class A : symlist {          // public inserted before each symbol
	//                              // absent symlist -> 'public Mailbox'
	//   public:
	//	inline TYPE get () { return ((TYPE) zz_get ()); };
	//	inline TYPE first () { return ((TYPE) zz_first ()); };
	//	inline int put (TYPE a) { return zz_put ((void*) a); };
	//	inline int putP (TYPE a) { return zz_putP ((void*) a); };
	//	inline int erase () { return zz_erase (); };
	//	virtual const char *getTName () { return ("A"); };

	if (CodeType) {
		xerror ("mailbox declaration within a code method");
		return (NO);
	}
	lc = del;
	empty = NO;
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of mailbox declaration");
		return (NO);
	}
	if (lc == ERROR || !isalpha (lc) && lc != '_' && lc != '$') return (NO);

	// This is the new mailbox type name
	lc = getKeyword (arg, lc);

	// Determine the delimiter

	if (lc == ';') {
		// Just announcing the type
		if (!addSym (arg, MAILBOX)) return (NO);
Announce:
		putC ("class ");
		putC (arg);
		putC (';');
		catchUp ();
		return (YES);
	}

	if (lc != ':' && lc != '{' && lc != '(') {
		// Check if followed by 'virtual'
		lc = getKeyword (par, lc);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;

		if (strcmp (par, "virtual") == 0)
			mode = VIRTUAL;
		else if (strcmp (par, "abstract") == 0)
			mode = ABSTRACT;
		else {
Synerror:
			xerror ("'mailbox' declaration syntax error");
			return (NO);
		}

		if (lc == ';') {
			if (!addSym (arg, PROCESS, ANNOUNCED, mode))
				return (NO);
			goto Announce;
		}
	} else {
		mode = REAL;
	}

	if (!(co = addSym (arg, MAILBOX, DEFINED, mode))) return (NO);

	putC ("class ");
	putC (arg);

	fsup = YES;
	if (lc == ':') {
		// Derived from something: go through the list of identifiers
		// and append them to the declaration
		while (1) {
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if (strcmp (par, arg) == 0) {
				xerror ("'%s' refers to itself", par);
				goto Assumereal;
			}
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
				goto Assumereal;
			}
			if (!isDefined (ob, MAILBOX, DEFINED)) {
				xerror ("'%s' not a defined Mailbox", par);
				goto Assumereal;
			}
			if (fsup) {
				putC (" : ");
				fsup = NO;
				// The first keyword
				if (ob->Mode == VIRTUAL) {
					if (mode != VIRTUAL) {
						// Force the REAL status
						putC ("public Mailbox, ");
					} 
				} else {
					if (mode == VIRTUAL) {
xerror ("'%s' declared virtual but '%s' is not virtual", arg, par);
						goto Assumereal;
					}
				}
			} else {
				if (ob->Mode != VIRTUAL) {
				    xerror ("'%s' is not virtual", par);
				}
			}
Assumereal:
			putC ("public ");
			putC (par);
			if (lc == ',') {
				putC (',');
			} else if (lc == '{' || lc == '(') {
				break;
			} else if (lc == ';') {
				empty = YES;
				break;
			} else goto Synerror;
		}
	} else {
		// Derived from Mailbox or from nothing
		if (mode != VIRTUAL) putC (" : public Mailbox");
	}

	putC (" { public: ");
        genDCons (arg);

	par [0] = '\0';		// Flag == element type undefined

	if (lc == '(') {
		if (mode == VIRTUAL) {
			xerror ("a virtual mailbox cannot specify "
				"element type");
			return (NO);
		}
		lc = getArg (par);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if (lc != ')') goto Synerror;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
		if (lc == ';') {
			// Assume empty body
			empty = YES;
		} else if (lc != '{') goto Synerror;
	}

	if (!empty) {
		if (pushDecl (co) == NO) return (NO);
		BraceLevel++;
	}

	if (par [0] != '\0') {
		// Element type defined
		putC ("inline "); putC (par);
		putC (" get () { return ((");
		putC (par);
		putC (") (IPointer) zz_get ()); }; inline "); putC (par);
		putC (" first () { return ((");
		putC (par);
		putC (") (IPointer) zz_first ()); }; inline int put (");
		putC (par);
		putC (" a) { return (zz_put ((void*)(IPointer)a));}; "
				"inline int putP (");
		putC (par);
		putC (" a) { return (zz_putP ((void*)(IPointer)a)); };");
		putC (" inline int erase () { return (zz_erase ()); };");
		putC (" inline Boolean queued (");
		putC (par);
		putC (" a) { return (zz_queued ((void*)(IPointer)a));}; ");


		// Indicate type present (for inItem / outItem)
		co -> PAnn = YES;
		co -> States = (char**) new char [strlen (par) + 1];
		strcpy ((char*)(co->States), par);
	}

	if (mode != VIRTUAL) {
		putC ("virtual const char *getTName () { return (\"");
		putC (arg);
		putC ("\"); };");
	}
	catchUp ();
	if (empty) {
		putC ("}; ");
		emitBuilder (co);
	}
	InItemDecl = OutItemDecl = NO;
	return (YES);
}

int     processInitem (int del) {

/* ----------------------------------------- */
/* Processes 'inItem' occurring in a mailbox */
/* ----------------------------------------- */

	char    arg[MAXKWDLEN+1];
	int     lc;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != MAILBOX) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);

	if (CObject->PAnn == 0) {
		xerror ("inItem illegal for a type-less mailbox");
		return (NO);
	}

	if ((lc = del) != '(') {
Synerror:
		xerror ("inItem declaration syntax error");
		return (NO);
	}

	// This is the new mailbox type name
	lc = getArg (arg);
	if (arg [0] == '\0' || lc == ',') {
		xerror ("inItem should have exactly one argument");
		return (NO);
	}

	if (lc != ')') goto Synerror;

	InItemDecl++;

	putC ("inItem (");
	putC (arg);
	putC (')');

	catchUp ();
	return (YES);
}

int     processOutitem (int del) {

/* ------------------------------------------ */
/* Processes 'outItem' occurring in a mailbox */
/* ------------------------------------------ */

	char    arg[MAXKWDLEN+1];
	int     lc;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != MAILBOX) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);

	if (CObject->PAnn == 0) {
		xerror ("outItem illegal for a type-less mailbox");
		return (NO);
	}

	if ((lc = del) != '(') {
Synerror:
		xerror ("outItem declaration syntax error");
		return (NO);
	}

	// This is the new mailbox type name
	lc = getArg (arg);
	if (arg [0] == '\0' || lc == ',') {
		xerror ("outItem should have exactly one argument");
		return (NO);
	}

	if (lc != ')') goto Synerror;

	OutItemDecl++;

	putC ("outItem (");
	putC (arg);
	putC (')');

	catchUp ();
	return (YES);
}

void	endOfMailbox () {

/* ----------------------------------------------- */
/* Processes the end of a mailbox type declaration */
/* ----------------------------------------------- */

	if (InItemDecl) {
		putC ("virtual void zz_initem () { inItem ((");
		putC ((char*)(CObject->States));
		putC (") zz_mbi); }; ");
	}

	if (OutItemDecl) {
		putC ("virtual void zz_outitem () { outItem ((");
		putC ((char*)(CObject->States));
		putC (") zz_mbi); }; ");
	}

	InItemDecl = OutItemDecl = NO;

	putC ('}');
}

int     processSetup (int del) {

/* --------------------------------------- */
/* Processes 'setup' occurring in a Packet */
/* --------------------------------------- */

	char    typ[MAXKWDLEN+1], par[MAXKWDLEN+1];
	int     lc;
	SymDesc *ob;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != PACKET) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);

	if ((lc = del) != '(') {
Synerror:
		xerror ("setup declaration syntax error");
		return (NO);
	}

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc == ')') return (NO);	// Argument-less
	lc = getKeyword (typ, lc);

	// Ignore if the argument is not a message subtype pointer
	if (lc != '*') return (NO);
	if ((ob = getSym (typ)) == NULL) return (NO);
	if (ob->Type != MESSAGE) return (NO);

	// Announced already
	if (CObject->States != NULL) {
		if (strcmp ((char*)(CObject->States), typ)) {
  xerror ("Packet setup argument type collides with previous declaration");
			return (NO);
		}
	} else {
		CObject->States = (char**) new char [strlen (typ) + 1];
		strcpy ((char*)(CObject->States), typ);
	}

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc != ')') lc = getKeyword (par, lc); else par [0] = '\0';
	if (lc != ')') goto Synerror;

	putC ("setup (Message*");
	if (strcmp (typ, "Message") == 0) {
		putC (' ');
		putC (par);
		putC (')');
		catchUp ();
		return (YES);
	}

	if (par [0] != '\0') putC (" zz_mspar");
	putC (')');

	while (1) {
		lc = lkpC ();
		if (!isspace (lc)) break;
	}

	if (lc == '{') {
		BraceLevel++;
		if (par [0] == '\0') goto Synerror;
		putC ("{ register ");
		putC (typ);
		putC (" *");
		putC (par);
		putC (" = (");
		putC (typ);
		putC ("*) zz_mspar;");
	} else
		putC (lc);

	catchUp ();
	return (YES);
}

int     processPFMMQU (int del) {

/* ----------------------------------------- */
/* Processes 'pfmMQU' occurring in a Traffic */
/* ----------------------------------------- */

	char    typ[MAXKWDLEN+1], par[MAXKWDLEN+1];
	int     lc;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != TRAFFIC) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);
	if (CObject->States == NULL) {
		xerror ("pfmMQU illegal for a virtual Traffic type");
		return (NO);
	}

	if ((lc = del) != '(') {
Synerror:
		xerror ("pfmMQU declaration syntax error");
		return (NO);
	}

	lc = getKeyword (typ);
	if (lc != '*') {
		xerror ("pfmMQU argument must be a Message subtype pointer");
		return (NO);
	}

	if (strcmp (typ, "Message") == 0) return (NO);
	if (strcmp (typ, CObject->States [0])) {
	    xerror ("pfmMQU argument type doesn't match traffic message type");
	    return (NO);
	}

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc != ')') lc = getKeyword (par, lc); else par [0] = '\0';
	if (lc != ')') goto Synerror;

	putC ("pfmMQU (Message*");
	if (par [0] != '\0') putC (" zz_mspar");
	putC (')');

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc == '{') {
		BraceLevel++;
		if (par [0] == '\0') goto Synerror;
		putC ("{ register ");
		putC (typ);
		putC (" *");
		putC (par);
		putC (" = (");
		putC (typ);
		putC ("*) zz_mspar;");
	} else
		putC (lc);

	catchUp ();
	return (YES);
}

int     processPFMMDE (int del) {

/* ----------------------------------------- */
/* Processes 'pfmMDE' occurring in a Traffic */
/* ----------------------------------------- */

	char    typ[MAXKWDLEN+1], par[MAXKWDLEN+1];
	int     lc;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != TRAFFIC) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);
	if (CObject->States == NULL) {
		xerror ("pfmMDE illegal for a virtual Traffic type");
		return (NO);
	}

	if ((lc = del) != '(') {
Synerror:
		xerror ("pfmMDE declaration syntax error");
		return (NO);
	}

	lc = getKeyword (typ);
	if (lc != '*') {
		xerror ("pfmMDE argument must be a Message subtype pointer");
		return (NO);
	}

	if (strcmp (typ, "Message") == 0) return (NO);
	if (strcmp (typ, CObject->States [0])) {
	    xerror ("pfmMDE argument type doesn't match traffic message type");
	    return (NO);
	}

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc != ')') lc = getKeyword (par, lc); else par [0] = '\0';
	if (lc != ')') goto Synerror;

	putC ("pfmMDE (Message*");
	if (par [0] != '\0') putC (" zz_mspar");
	putC (')');

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc == '{') {
		BraceLevel++;
		if (par [0] == '\0') goto Synerror;
		putC ("{ register ");
		putC (typ);
		putC (" *");
		putC (par);
		putC (" = (");
		putC (typ);
		putC ("*) zz_mspar;");
	} else
		putC (lc);

	catchUp ();
	return (YES);
}

const char *pfmtype;

void	pxerror (const char *s) {

	xerror ("%s %s", pfmtype, s);
}

int     prcPFM (int del) {

/* ---------------------------- */
/* Common processing for pfmXXX */
/* ---------------------------- */

	char    typ[MAXKWDLEN+1], par[MAXKWDLEN+1];
	int     lc;

	if (CodeType) return (NO);
	if (CObject == NULL) return (NO);
	if (CObject->Type != TRAFFIC) return (NO);
	if (CObjectBC != BraceLevel-1) return (NO);
	if (CObject->States == NULL) {
		pxerror ("illegal for a virtual Traffic type");
		return (NO);
	}

	if ((lc = del) != '(') {
Synerror:
		pxerror ("declaration syntax error");
		return (NO);
	}

	lc = getKeyword (typ);
	if (lc != '*') {
		xerror ("argument must be a Packet subtype pointer");
		return (NO);
	}

	if (strcmp (typ, "Packet") == 0) return (NO);
	if (strcmp (typ, CObject->States [1])) {
	    xerror ("argument type doesn't match traffic packet type");
	    return (NO);
	}

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc != ')') lc = getKeyword (par, lc); else par [0] = '\0';
	if (lc != ')') goto Synerror;

	putC (pfmtype);
	putC (" (Packet*");
	if (par [0] != '\0') putC (" zz_mspar");
	putC (')');

	while (1) { lc = lkpC (); if (!isspace (lc)) break; }

	if (lc == '{') {
		BraceLevel++;
		if (par [0] == '\0') goto Synerror;
		putC ("{ register ");
		putC (typ);
		putC (" *");
		putC (par);
		putC (" = (");
		putC (typ);
		putC ("*) zz_mspar;");
	} else
		putC (lc);

	catchUp ();
	return (YES);
}

int	processPFMMTR (int del) { pfmtype = "pfmMTR"; return (prcPFM (del)); }
int	processPFMMRC (int del) { pfmtype = "pfmMRC"; return (prcPFM (del)); }
int	processPFMPTR (int del) { pfmtype = "pfmPTR"; return (prcPFM (del)); }
int	processPFMPRC (int del) { pfmtype = "pfmPRC"; return (prcPFM (del)); }
int	processPFMPDE (int del) { pfmtype = "pfmPDE"; return (prcPFM (del)); }

int     doPerform (int del, SymDesc *obj) {

/* --------------------------------- */
/* Expands the 'perform' declaration */
/* --------------------------------- */

	int     lc;

	// perform;                             // Announcement
	// expands into 'zz_code ();'
	//
	// perform {
	//
	// in a process expands into:
	//      zz_code () {switch (TheState) {
	// in an observer expands into:
	//      zz_code () {switch (TheObserverState) {

	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'perform' declaration");
		CodeType = NO;
		CodeObject = NULL;
		return (NO);
	}

	if (lc == ';') {
		// Just announcing
		if (obj != CObject) {
			xerror ("'perform' announced incorrectly");
			return (NO);
		}
		if (obj->Mode == VIRTUAL) {
Modeerr:
			xerror ("'perform' illegal in a virtual process");
			return (NO);
		}
		CodeType = NO;
		CodeObject = NULL;
		putC ("zz_code ();");
		obj->PAnn = YES;
		catchUp ();
		return (YES);
	}

	if (lc != '{') {
		CodeType = NO;
		CodeObject = NULL;
		xerror ("'perform' declaration syntax error");
		return (NO);
	}

	BraceLevel++;
	putC ("zz_code () {");
	CodeWasState = NO;

	if (obj->Mode == VIRTUAL) goto Modeerr;

	obj->PAnn = YES;

	catchUp ();
	return (YES);
}

int     processTraffic (int del) {

/* ------------------------------------ */
/* Expands the Traffic type declaration */
/* ------------------------------------ */

	char    arg[MAXKWDLEN+1], par[MAXKWDLEN+1], mar[MAXKWDLEN+1];
	int     lc, mode, fsup, empty;
	SymDesc *ob, *co;

	// traffic A [virtual];                 -> class A;
	// traffic A [virtual] [:symlist] [(M [, P])] {
	//
	// full expansion:
	//
	// class A : public symlist {           // public added, abs = Traffic
	//      private:
	//      virtual Message *zz_mkm () { return (new M); };
	//      virtual Packet  *zz_mkp () { Packet *p; p = new P;
	//              zz_mkpclean ();
	//              return (p);
	//      };      // defaults: P = Packet, M = Message
	//      public:
	//      virtual const char *getTName () { return ("A"); };

	lc = del;
	empty = NO;
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of traffic declaration");
		return (NO);
	}
	if (lc == ERROR || !isalpha (lc) && lc != '_' && lc != '$') return (NO);

	if (CodeType) {
		xerror ("traffic declaration within a code method");
		return (NO);
	}

	if (NoClient) {
		xerror ("'traffic' illegal with -C (no Client)");
		return NO;
	}

	// This is the new traffic type name
	lc = getKeyword (arg, lc);

	// Determine the delimiter

	if (lc == ';') {
		// Just announcing the type
		if (!addSym (arg, TRAFFIC)) return (NO);
Announce:
		putC ("class ");
		putC (arg);
		putC (';');
		catchUp ();
		return (YES);
	}

	if (lc != ':' && lc != '{' && lc != '(') {
		// Check if followed by 'virtual'
		lc = getKeyword (par, lc);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;

		if (strcmp (par, "virtual") == 0)
			mode = VIRTUAL;
		else if (strcmp (par, "abstract") == 0)
			mode = ABSTRACT;
		else {
Synerror:
			xerror ("'traffic' declaration syntax error");
			return (NO);
		}

		if (lc == ';') {
			if (!addSym (arg, PROCESS, ANNOUNCED, mode))
				return (NO);
			goto Announce;
		}
	} else {
		mode = REAL;
	}

	if (!(co = addSym (arg, TRAFFIC, DEFINED, mode))) return (NO);

	putC ("class "); putC (arg);

	fsup = YES;
	if (lc == ':') {
		// Derived from something: go through the list of identifiers
		// and append them to the declaration
		while (1) {
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if (strcmp (par, arg) == 0) {
				xerror ("'%s' refers to itself", par);
				goto Assumereal;
			}
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
				goto Assumereal;
			}
			if (!isDefined (ob, TRAFFIC, DEFINED)) {
				xerror ("'%s' not a defined Traffic", par);
				goto Assumereal;
			}
			if (fsup) {
				putC (" : ");
				fsup = NO;
				// The first keyword
				if (ob->Mode == VIRTUAL) {
					if (mode != VIRTUAL) {
						// Force the REAL status
						putC ("public Traffic, ");
					} 
				} else {
					if (mode == VIRTUAL) {
xerror ("'%s' declared virtual but '%s' is not virtual", arg, par);
						goto Assumereal;
					}
				}
			} else {
				if (ob->Mode != VIRTUAL) {
				    xerror ("'%s' is not virtual", par);
				}
			}
Assumereal:
			putC ("public ");
			putC (par);
			if (lc == ',') {
				putC (',');
			} else if (lc == '{' || lc == '(') {
				break;
			} else if (lc == ';') {
				empty = YES;
				break;
			} else goto Synerror;
		}
	} else {
		// Derived from Traffic or from nothing
		if (mode != VIRTUAL) putC (" : public Traffic");
	}

	// Flag message type and packet type as undefined
	mar [0] = par [0] = '\0';

	if (lc == '(') {
		if (mode == VIRTUAL) {
			xerror ("virtual traffic type cannot "
			    "specify associated message and/or packet type");
			return (NO);
		}
		// Message type goes first
		lc = getKeyword (mar);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if ((ob = getSym (mar)) == NULL) {
			xerror ("'%s' undefined", mar);
		} else if (!isDefined (ob, MESSAGE, ANNOUNCED)) {
			xerror ("'%s' not defined as 'Message'", mar);
		} else if (ob->Mode == VIRTUAL) {
			xerror ("'%s' is virtual", mar);
		}
		// Continue with this error
		if (lc == ',') {
			// Packet type goes next
			lc = getKeyword (par);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			if ((ob = getSym (par)) == NULL) {
				xerror ("'%s' undefined", par);
			} else if (!isDefined (ob, PACKET, ANNOUNCED)) {
			   xerror ("'%s' not defined as 'Packet'", par);
			} else if (ob->Mode == VIRTUAL) {
			   xerror ("'%s' is virtual", par);
			}
			// Continue with this error
		}
		if (lc != ')') goto Synerror;
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
		if (lc == ';') {
			// Empty body
			empty = YES;
		} else if (lc != '{') goto Synerror;
	}

	if (!empty) {
		if (pushDecl (co) == NO) return (NO);
		BraceLevel++;
	}

	if (mode != VIRTUAL) {
		putC (" { private: virtual Message *zz_mkm () { ");
		putC (" return ((Message*)(new ");
		putC (*mar != '\0' ? mar : "Message");
		putC (")); }; virtual Packet *zz_mkp () {Packet *p; p = new ");
		putC (*par != '\0' ? par : "Packet");
		putC ("; zz_mkpclean (); return (p); }; public: ");
		putC ("virtual const char *getTName () { return (\"");
		putC (arg);
		putC ("\"); };");
		co->States = new char* [2];
		if (*mar != '\0') {
			co->States [0] = new char [strlen (mar) + 1];
			strcpy (co->States [0], mar);
		} else
			co->States [0] = NULL;	
		if (*par != '\0') {
			co->States [1] = new char [strlen (par) + 1];
			strcpy (co->States [1], par);
		} else
			co->States [1] = NULL;	
	} else {
		putC (" { public: ");
        	genDCons (arg);
	}

	catchUp ();
	if (empty) {
		putC ("}; ");
		emitBuilder (co);
	}
	return (YES);
}

int     isSymbol (char *arg) {

/* -------------------------------------- */
/* Checks is the string is a legal symbol */
/* -------------------------------------- */

	if (!isalpha (*arg) && *arg != '_' && *arg != '$') return (NO);
	arg++;
	while (*arg != '\0') {
		if (!isalnum (*arg) && *arg != '_' && *arg != '$') return (NO);
		arg++;
	}
	return (YES);
}

SymDesc *isPrcs (char *s) {

/* ---------------------------------------------------------------- */
/* Checks whether the symbol is a process type id (used in inspect) */
/* ---------------------------------------------------------------- */

	SymDesc  *ob;

	if (!isSymbol (s)) {
		xerror ("process type id '%s' is not a symbol", s);
		return (NULL);
	}

	if ((ob = getSym (s)) == NULL) {
		xerror ("'%s' undefined", s);
		return (NULL);
	}

	if (ob->Type != PROCESS) {
		xerror ("'%s' is not a process type", s);
		return (NULL);
	}
	return (ob);
}

int     processInspect (int del) {

/* ----------------------- */
/* Expands the inpect call */
/* ----------------------- */

	// inspect (A)                  expands into:
	//
	// zz_inspect_rq ((Station*)ANY, (void*)(&zz_Process_prcs),
	//   (char*)ANY, ANY, A)
	//
	// inspect (A, B)               expands into:
	//
	// zz_inspect_rq ((Station*)(A), (void*)(&zz_Process_prcs),
	//   (char*)ANY, ANY, B)
	//
	// inspect (A, B, C)            expands into:
	//
	// zz_inspect_rq ((Station*)(A), (void*)(&zz_B_prcs), (char*)ANY,
	//   ANY, C)
	//
	// inspect (A, B, C, D)         expands into:
	//
	// zz_inspect_rq ((Station*)(A), (void*)(&zz_B_prcs), (char*)ANY,
	//   C, D)
	//
	// inspect (A, B, C, D, E)      expands into:
	//
	// zz_inspect_rq ((Station*)(A), (void*)(&zz_B_prcs), (char*)(C),
	//   D, E)

	char    *pps, arg [5] [MAXKWDLEN+1];  // Up to 5 arguments
	int     lc, ac, ap;
	SymDesc *to;

	if (CodeType != OBSERVER) return (NO);
	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of inspect");
		return (NO);
	}

	if (lc != '(') {
Synerror:
		xerror ("inspect syntax error");
		return (NO);
	}

	for (ac = 0; ac < 5;) {
		// Parse the arguments
		lc = getArg (arg [ac]);
		ac++;
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if (lc == ')') break;
		if (lc != ',') goto Synerror;
	}
	if (lc != ')') {
		xerror ("too many arguments to inspect");
		return (NO);
	}

	putC ("zz_inspect_rq ((Station*)(");

	// The rest off the processing differs depending on the argument count

	switch (ac) {

	    case 1:     // Monitor state only

		if (!isSymbol (arg[0])) {
Msterror:
			xerror ("monitor state id is not a symbol");
			return (NO);
		}
		if (!isState (arg[0])) return (NO);
		putC ("-1), (void*)(&zz_Process_prcs), ");
		putC ("(char*) (-1), -1, ");
		putC (arg[0]);
		putC (')');
		break;

	    case 2:     // Station + monitor state

		putC (arg[0]);
		putC ("), (void*)(&zz_Process_prcs), ");
		putC ("(char*) (-1), -1, ");
		if (!isSymbol (arg[1])) goto Msterror;
		if (!isState (arg[1])) return (NO);
		putC (arg[1]);
		putC (')');
		break;

	    case 3:     // Station + process type + monitor state

		putC (arg[0]);
		putC ("), (void*)(&");
		if (strcmp (arg[1], "(-1)") == 0) {
			putC ("zz_Process_prcs");
		} else if (!isPrcs (arg[1])) {
			return (NO);
		} else {
			putC ("zz_");
			putC (arg[1]);
			putC ("_prcs");
		}
		putC ("), (char*) (-1), -1, ");
		if (!isSymbol (arg[2])) goto Msterror;
		if (!isState (arg[2])) return (NO);
		putC (arg[2]);
		putC (')');
		break;

	    case 4:     // Station + process type + process state + mtr state

		putC (arg[0]);
		putC ("), (void*)(&");
		to = CodeObject;
		ap = (strcmp (arg[1], "(-1)") == 0);
		if (!ap && !(CodeObject = isPrcs (arg[1]))) {
			CodeObject = to;
			return (NO);
		}
		if (ap) {
			putC ("zz_Process_prcs");
		} else {
			putC ("zz_");
			putC (arg[1]);
			putC ("_prcs");
		}
		putC ("), (char*) (-1), ");
		if (strcmp (arg[2], "(-1)")) {
			if (!isSymbol (arg[2])) {
Psterror:
				xerror ("process state is not a symbol");
				CodeObject = to;
				return (NO);
			}
			if (ap) {
Pstanerr:
		xerror ("process type must be specified if state is not 'ANY'");
				CodeObject = to;
				return (NO);
			}
			if ((pps = isStateP (arg[2])) == NULL) {
				CodeObject = to;
				return (NO);
			}
			putC (pps);
			putC ("::");
		}
		CodeObject = to;
		putC (arg[2]);
		putC (", ");
		if (!isSymbol (arg[3])) goto Msterror;
		if (!isState (arg[3])) return (NO);
		putC (arg[3]);
		putC (')');
		break;

	    case 5:     // St + prcs type + prcs nknm + prcs state + mtr state

		putC (arg[0]);
		putC ("), (void*)(&");
		to = CodeObject;
		ap = (strcmp (arg[1], "(-1)") == 0);
		if (!ap && !(CodeObject = isPrcs (arg[1]))) {
			CodeObject = to;
			return (NO);
		}
		if (ap) {
			putC ("zz_Process_prcs");
		} else {
			putC ("zz_");
			putC (arg[1]);
			putC ("_prcs");
		}
		putC ("), ");
		putC (arg[2]);
		putC (", ");
		if (strcmp (arg[3], "(-1)")) {
			if (!isSymbol (arg[3])) goto Psterror;
			if (ap) goto Pstanerr;
			if ((pps = isStateP (arg[3])) == NULL) {
				CodeObject = to;
				return (NO);
			}
			putC (pps);
			putC ("::");
		}
		CodeObject = to;
		putC (arg[3]);
		putC (", ");
		if (!isSymbol (arg[4])) goto Msterror;
		if (!isState (arg[4])) return (NO);
		putC (arg[4]);
		putC (')');
		break;
	}

	catchUp ();
	return (YES);
}

int     processTimeout (int del) {

/* ------------------------ */
/* Expands the timeout call */
/* ------------------------ */

	// timeout (a, b);
	//
	// expands into 'zz_timeout_rq (a, b);

	char    arg [MAXKWDLEN+1];
	int     lc;

	if (CodeType != OBSERVER) return (NO);
	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of 'timeout'");
		return (NO);
	}

	if (lc != '(') {
Synerror:
		xerror ("'timeout' syntax error");
		return (NO);
	}

	lc = getArg (arg);
	if (lc == ERROR) return (NO);
	if (lc == END) goto Endfile;
	if (lc != ',') goto Synerror;           // Two arguments expected
	putC ("zz_timeout_rq (");
	putC (arg);
	lc = getKeyword (arg);
	if (lc == ERROR) return (NO);
	if (lc == END) goto Endfile;
	if (lc != ')') goto Synerror;           // And that's it
	if (!isState (arg)) return (NO);
	putC (", ");
	putC (arg);
	putC (')');

	catchUp ();
	return (YES);
}

int     processCreate (int del) {

/* ----------------------- */
/* Expands the create call */
/* ----------------------- */

	// p = create A;                        -> p = create A ();
	// p = create A (plist);
	// p = create A, B;                     -> p = create A, B, ();
	// p = create A, B, (plist);
	//
	// full expansion (no B):
	//
	// ({A *zz_op = new A; zz_op->zz_start (); zz_op->setup (plist);
	//  zz_op);});
	//
	// full expansion (with B -- the nickname):
	//
	// ({ char *zz_cc = (B); A *zz_op = new A; zz_op->zz_start ();
	//  zz_op->zz_nickname = new char [strlen(zz_cc)+1];
	//   strcpy (zz_op->zz_nickname, zz_cc); zz_op->setup (plist); zz_op;});
	//
	// Note: for observers zz_start and ->setup exchange places
	//
	// p = create (expr) A ...
	//
	// sets TheStation to 'expr' or 'idToStation (expr)' before creation
	//

	// In the AT&T version, the following sequence is generated:
	//
	// (zz_setths (expr), zz_bld_A (B), ((A*)(zz_COBJ[zz_clv]))->setup (..),
	//  ((A*)(zz_COBJ[zz_clv--]))
	//
	// for an observer, it is:
	//
	// (zz_setths (expr), zz_bld_A (B), ((A*)(zz_COBJ[zz_clv]))->setup (..),
	//  ((A*)(zz_COBJ[zz_clv]))->zz_start (), ((A*)(zz_COBJ[zz_clv--])))
	// 

	char    tpn [MAXKWDLEN+1], ths [MAXKWDLEN+1], nkn [MAXKWDLEN+1], *sptr;
	int     lc, sls, delim;
	SymDesc *ob;

	lc = del;
	delim = -1;
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of 'create'");
		return (NO);
	}

	// Get the type name
	if (lc == ERROR) return (NO);
	if (lc == '(') {
		// Request to set 'TheStation'
		lc = getArg (ths);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if (lc != ')') return (NO);
		while (1) {
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
		if (!isalpha (lc) && lc != '_' && lc != '$') return (NO);
	} else {
		ths [0] = '\0';
		if (!isalpha (lc) && lc != '_' && lc != '$') return (NO);
	}

	lc = getKeyword (tpn, lc);
	if (lc == ERROR) return (NO);
	if (lc == END) goto Endfile;
	// Check if the type is defined properly
	if ((ob = getSym (tpn)) == NULL) {
		xerror ("'%s' undefined", tpn);
		return (NO);
	}
	if (ob->Status != DEFINED) {
		xerror ("'%s' announced but undefined", tpn);
		return (NO);
	}
	if (ob->Mode != REAL) {
		xerror ("'%s' is virtual or abstract", tpn);
		return (NO);
	}
	putC ("({");
	if (ths [0] != '\0') {
		putC ("zz_setths (");
		putC (ths);
		putC ("); ");
	}
	putC ("zz_bld_");
	putC (tpn);
	putC (" (");
	if (lc == ',') {
		// Nickname specified
		if (ob->Type == MESSAGE || ob->Type == PACKET || ob->Type ==
		  SGROUP || ob->Type == CGROUP) {
		    xerror ("'%s': objects of this type don't have nicknames",
			tpn);
		    return (NO);
		}
		lc = getArg (nkn);
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		// Output the nickname
		putC (nkn);
		if (lc == ',') {
			// Skip to '(' or ';', whichever comes first
			while (1) {
				lc = lkpC ();
				if (!isspace (lc)) break;
			}
		}
	}
	putC ("); ((");
	putC (tpn);
	putC ("*)(zz_COBJ[zz_clv]))->setup (");
	if (lc == '(') {
		// Setup argument list
		sptr = EnLkpPtr;
		sls = SkLineCnt;
		while (1) {
			// Check if followed immediately by ')'
			lc = lkpC ();
			if (!isspace (lc)) break;
		}
		if (lc == ')') goto Setupdone;
		EnLkpPtr = sptr;        // Backspace
		SkLineCnt = sls;
		while (1) {
			// Process setup arguments
			lc = getArg (nkn);
			if (lc == ERROR) return (NO);
			if (lc == END) goto Endfile;
			putC (nkn);
			if (lc == ')') goto Setupdone;
			if (lc != ',') {
Synerr:
				xerror ("create syntax error");
				return (NO);
			}
			putC (',');
		}
	} else delim = lc;
Setupdone:
	putC ("); ");
	if (ob->Type == OBSERVER) {
		putC ("((");
		putC (tpn);
		putC ("*)(zz_COBJ[zz_clv]))->zz_start (); ");
	}

	if (ths [0] != '\0')
		// Remove the station from the stack
		putC ("zz_remths (); ");

	putC ('(');
	putC (tpn);
	putC ("*)(zz_COBJ[zz_clv--]);})");

	if (delim != -1) putC (delim);
	catchUp ();
	return (YES);
}

void    emitBuilder (SymDesc *co) {

/* ------------------------------------------------------- */
/* Generates the 'builder' function for the current object */
/* ------------------------------------------------------- */

	int     lc, nn;
	char    **cp;

	if (co == NULL) {
		co = CObject;
		if (co->Mode == VIRTUAL) return;
		if ((lc = startLkp ()) != END) {
			while (1) {
				lc = lkpC ();
				if (!isspace (lc)) break;
			}
			if (lc == ';') putC ("; ");
		}
	} else
		lc = END;

	if (co->Type == PROCESS || co->Type == OBSERVER) {
	    if (co->States != NULL) {
		// Emit the state table initializer
		putC (" const int ");
		putC (co->Name);
		putC ("::zz_ns");
		putC (SPref);
		cp = co->States;
		for (nn = 0; *cp != NULL; cp++) nn++;
		putC (form (" = %1d; ", nn));
		putC ("const char *");
		putC (co->Name);
		putC ("::zz_sl");
		putC (SPref);
		putC (form (" [%1d] = {", nn));
		cp = co->States;
		while (1) {
			putC ('"');
			putC (*cp);
			putC ('"');
			if (*(++cp) == NULL) break;
			putC (", ");
		}
		putC ("}; ");
            }
	    if (strcmp (co->Name, "Root") == 0) goto Done;
	}

	nn = co->Type != MESSAGE && co->Type != PACKET &&
		co->Type != SGROUP && co->Type != CGROUP;

	if (co->Mode == REAL) {
		putC ("inline void zz_bld_");
		putC (co->Name);
		putC (" (");
		if (nn) putC ("char *nn = 0");
		putC (") { register ");
		putC (co->Name);
		putC (" *p; zz_COBJ [++zz_clv] = (void*) (p = new ");
		putC (co->Name);
		putC ("); ");
		if (co->Type != OBSERVER)
			// For an observer, zz_start must be called 'by hand'
			putC ("p->zz_start (); ");
		if (nn) {
			// Assign the nickname
			putC ("if (nn != 0) { p->zz_nickname = ");
			putC ("new char [strlen (nn) + 1]; ");
			putC ("strcpy (p->zz_nickname, nn); } ");
		}

		putC ("}; ");
	}
Done:
	if (lc != ';' && lc != END && lc != ERROR) putC (lc);
	if (lc != END) catchUp ();
}

int     processNew (int del) {

/* ---------------------------------------------------------- */
/* Checks  if  'new'  is not followed by the name of a SMURPH */
/* type                                                       */
/* ---------------------------------------------------------- */

	char    arg [MAXKWDLEN+1];
	int     lc;

	lc = del;
	if (lc == END) {
		xerror ("file ends unexpectedly");
		return (NO);
	}
	if (lc == ERROR || !isalpha (lc) && lc != '_' && lc != '$') return (NO);

	// This is the identifier following 'new'
	lc = getKeyword (arg, lc);

	// Determine the delimiter
        if (lc != ';' && lc != '(' && lc != '[') return (NO);

	// Not a SMURPH type
	if (getSym (arg) != NULL)
		xerror ("'new' for a SMURPH object type (use 'create')");
	return (NO);
}

int     doExposure (int del, SymDesc *obj) {

/* ------------------------------- */
/* Expands the exposure definition */
/* ------------------------------- */

	int     lc;

	// exposure;                            Announcement within object
	//
	// expands into:
	//
	// virtual void zz_expose (int, char *h = NULL, long s = NONE);
	//
	// A::exposure {                        Exposure definition
	//
	// expands into:
	//
	// void A::zz_expose (int zz_emode, char *Hdr, long SId) {
	//      int     zz_em;

	lc = del;
	if (lc == END) {
		xerror ("file ends in the middle of 'exposure'");
		return (NO);
	}

	if (lc == ';') {
	  // Just announcing
	  if (obj == NULL || obj != CObject) {
	    xerror ("'exposure' announced incorrectly");
	    return (NO);
	  }
	  if (obj->EStat == UNEXPOSABLE) {
Unexperr:
		xerror ("'%s' is unexposable", obj->Name);
		return (NO);
	  }
	  obj->EStat = ANNOUNCED;
	  putC ("virtual zz_expose (int, const char *h = 0, ");
	  putC (XLong);
	  putC (" s = -1);");
	  goto Done;
	}
	if (lc != '{') return (NO);

	if (CodeType) {
		xerror ("exposure definition in illegal context");
		return (NO);
	}
	if (WithinExposure) {
		xerror ("exposure definitions cannot be nested");
		return (NO);
	}
	if (obj == NULL) {
		xerror ("'exposure' unrelated to any object");
		return (NO);
	}
	if (obj->EStat == UNEXPOSABLE) goto Unexperr;
	if (obj->EStat != ANNOUNCED && obj != CObject) {
		xerror ("'exposure' not announced for type '%s'", obj->Name);
		return (NO);
	}
	WithinExposure = BraceLevel++;
	OnpaperDone = OnscreenDone = NO;
	if (obj == CObject) {
		putC ("virtual ");
		putC ("zz_expose (int zz_emode, const char *Hdr = 0, ");
		putC (XLong);
		putC (" SId =-1)");
		putC (" {int zz_em;");
	} else {
		putC ("zz_expose (int zz_emode, const char *Hdr, ");
		putC (XLong);
		putC (" SId)");
		putC (" {int zz_em;");
	}
Done:
	catchUp ();
	return (YES);
}

int     processOnpaper (int del) {

/* --------------------------------------------- */
/* Expands the 'onpaper' header of exposure mode */
/* --------------------------------------------- */

	int     lc;

	// onpaper {
	//
	// expands into:
	//
	// ZZ_ONPP: if ((zz_em = zz_emode) >= ZZ_EMD_0) goto ZZ_ONSC;
	//          if (zz_em == ANY) return; zz_emode = ANY;
	//          switch (zz_em) {

	lc = del;
	if (!WithinExposure) return (NO);
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'onpaper'");
		return (NO);
	}

	if (lc != '{') {
		xerror ("'onpaper' syntax error");
		return (NO);
	}
	if (OnpaperDone) {
		xerror ("'onpaper' occurs twice ore more within one exposure");
		return (NO);
	}

	OnpaperDone = YES;
	putC ("ZZ_ONPP: if ((zz_em = zz_emode) >= 4096) goto ZZ_ONSC; ");
	putC ("if (zz_em == -1) return; zz_emode = -1; switch (zz_em) {");
	WithinExpMode = BraceLevel++;
	WasExmode = NO;

	catchUp ();
	return (YES);
}

int     processOnscreen (int del) {

/* ---------------------------------------------- */
/* Expands the 'onscreen' header of exposure mode */
/* ---------------------------------------------- */

	// onscreen {
	//
	// expands into:
	//
	// ZZ_ONSC: if ((zz_em = zz_emode) < ZZ_EMD_0) goto ZZ_ONPP;
	//          zz_emode = ANY; zz_em -= ZZ_EMD_0;
	//          switch (zz_em) {

	int     lc;

	lc = del;
	if (!WithinExposure) return (NO);
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'onscreen'");
		return (NO);
	}

	if (lc != '{') {
		xerror ("'onscreen' syntax error");
		return (NO);
	}
	if (OnscreenDone) {
		xerror ("'onscreen' occurs twice ore more within one exposure");
		return (NO);
	}

	OnscreenDone = YES;
	putC ("ZZ_ONSC: if ((zz_em = zz_emode) < 4096) goto ZZ_ONPP; ");
	putC ("zz_emode = -1; zz_em -= 4096; ");
	putC ("switch (zz_em) {");
	WithinExpMode = BraceLevel++;
	WasExmode = NO;

	catchUp ();
	return (YES);
}

int     processExmode (int del) {

/* ------------------- */
/* Expands 'exmode m:' */
/* ------------------- */

	// exmode m:
	//
	// expands into: 'break; case m: zz_dispfound = YES;'

	int     lc;
	char    m [MAXKWDLEN+1];

	lc = del;
	if (!WithinExposure || !WithinExpMode) return (NO);
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'exmode'");
		return (NO);
	}

	// Get the expression
	lc = getKeyword (m, lc, YES);   // May be a number

	if (lc != ':') {
		xerror ("'exmode' syntax error");
		return (NO);
	}
	if (WasExmode) putC ("break; ");
	putC ("case ");
	putC (m);
	putC (": zz_dispfound = 1; ");
	WasExmode = YES;

	catchUp ();
	return (YES);
}

int     doExpose (int del) {

/* -------------------------------------------------- */
/* Expands 'expose;' (must be preceded by 'symbol::') */
/* -------------------------------------------------- */

	// expose;
	//
	// expands into: 'zz_expose (zz_emode, Hdr, SId)'

	int     lc;

	// We know for sure that we are WithinExposure
	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
		xerror ("file ends in the middle of 'expose'");
		return (NO);
	}

	if (lc != ';') {
		xerror ("'expose' syntax error");
		return (NO);
	}
	putC ("zz_expose (zz_emode, Hdr, SId);");

	catchUp ();
	return (YES);
}

int     processExpose (int del) {

/* -------------------------------------------------------- */
/* Processes illegal exposure -- not preceded by 'symbol::' */
/* -------------------------------------------------------- */

	if (WithinExposure)
		xerror ("'expose' must be preceded by 'symbol::'");
	return (NO);
}

int     processGetproclist (int del) {

	// getproclist (a, b, k, n)	->
	//
	// 	zz_getproclist (a, (void*)(&zz_b_prcs), k, n);
	//
	// getproclist (a, k, n)	->
	//
	// 	zz_getproclist (a, NULL, k, n);
	//
	// getproclist (k, n)		->
	//
	// 	zz_getproclist (NULL, NULL, k, n);
	//

	char    arg [4] [MAXKWDLEN+1];  // Up to 4 arguments
	int     lc, ac;

	lc = del;
	if (lc == ERROR) return (NO);
	if (lc == END) {
Endfile:
		xerror ("file ends in the middle of getproclist");
		return (NO);
	}

	if (lc != '(') {
Synerror:
		xerror ("getproclist syntax error");
		return (NO);
	}

	for (ac = 0; ac < 4;) {
		// Parse the arguments
		lc = getArg (arg [ac]);
		ac++;
		if (lc == ERROR) return (NO);
		if (lc == END) goto Endfile;
		if (lc == ')') break;
		if (lc != ',') goto Synerror;
	}
	if (lc != ')') {
		xerror ("too many arguments to getproclist");
		return (NO);
	}

	putC ("zz_getproclist (");

	switch (ac) {

	    case 2:     // Process list and size only

		putC ("NULL, NULL, ");
		putC (arg [0]);
		putC (", ");
		putC (arg [1]);
		putC (')');
		break;

	    case 3:	// + Station

		putC ("(Sation*)(");
		putC (arg [0]);
		putC ("), NULL, ");
		putC (arg [1]);
		putC (", ");
		putC (arg [2]);
		putC (')');
		break;

	    case 4:	// + Type Id

		putC ("(Sation*)(");
		putC (arg [0]);
		if (!isSymbol (arg [1])) {
			xerror ("process type name is formally illegal");
			return (NO);
		}
		putC ("), (void*)(&zz_");
		putC (arg [1]);
		putC ("_prcs), ");
		putC (arg [3]);
		putC (", ");
		putC (arg [4]);
		putC (')');
		break;

	    default:

		xerror ("illegal number of arguments to getproclist");
		return NO;
	}

	catchUp ();
	return (YES);
}

int     processPerform (int del) {

/* ----------------------------------------------------- */
/* Preprocesses 'perform' within object type declaration */
/* ----------------------------------------------------- */

	if (CodeType) {
		xerror ("'perform' within another 'perform'");
		return (NO);
	}

	if (CObject == NULL) return (NO);
	if (CObject->Type == PROCESS) {
		CodeType = PROCESS;
	} else if (CObject->Type == OBSERVER) {
		CodeType = OBSERVER;
	} else return (NO);
	CodeTypeBC = BraceLevel;
	CodeObject = CObject;
	putC ("void ");
	return (doPerform (del, CObject));
}

int     processExposure (int del) {

/* ------------------------------------------------------ */
/* Preprocesses 'exposure' declared within an object type */
/* ------------------------------------------------------ */

	putC ("void ");
	return (doExposure (del, CObject));
}

int     processKeyword (int fc) {

/* --------------------------------------------------------- */
/* Try to match the keyword with the ones in your dictionary */
/* --------------------------------------------------------- */

	char    kwd [MAXKWDLEN+1], *septr;
	int     del, skc, status;
	KFUNC   f;

	SkLineCnt = 0;  // Count skipped lines
	if (startLkp () == END) return (NO);
	// This is the keyword we should try to match
	del = getKeyword (kwd, fc);
	// del is the non-blank delimiter

	// Check if it is not a sequence of the form: symbol::perform,
	// symbol::exposure, or symbol::expose.
	// Must be able to tell whether the perform belongs to a process
	// or an observer

	if (!ArrowInFront && del == ':') {
		char    pfm [MAXKWDLEN+1], pfmm [16];
		SymDesc *ob, *ox;
		// Save the lookahead pointer
		septr = EnLkpPtr;
		skc = SkLineCnt;
		// Get the second delimiter
		del = lkpC ();
		if (del != ':') {
			// Backspace
			del = ':';
			EnLkpPtr = septr;
			SkLineCnt = skc;
			goto Tryother;
		}
		// Now, check if it is followed by 'perform' or 'exposure'
                while (1) { del = lkpC (); if (!isspace (del)) break; }
                if (del == '~') {
			// This is a destructor
			del = ':';
			EnLkpPtr = septr;
			SkLineCnt = skc;
			goto Tryother;
                }
		del = getKeyword (pfm, del);
		if (!strcmp (pfm, "perform")) {
		    // perform
		    if (CodeType) {
			xerror ("'perform' within another 'perform'");
			return (NO);
		    }
		    if (CObject != NULL) {
		       xerror ("illegal '::perform' declaration inside object");
		       return (NO);
		    }
		    // Determine the status of the ownining object
		    if ((ob = getSym (kwd)) == NULL) {
			xerror ("'perform' for an undefined process type");
			return (NO);
		    }
		    if (ob->Type == PROCESS) {
			CodeType = PROCESS;
		    } else if (ob->Type == OBSERVER) {
			CodeType = OBSERVER;
		    } else {
			xerror ("'perform' neither in process nor in observer");
			return (NO);
		    }
		    if (!(ob->PAnn)) {
			xerror ("'perform' was not announced in '%s'", kwd);
		    }
		    CodeTypeBC = BraceLevel;
		    if (ob->Mode == VIRTUAL) {
		   xerror ("'perform' for a virtual or announced process type");
		       CodeType = NO;
		       return (NO);
		    }
		    CodeObject = ob;
		    putC ("void ");
		    putC (kwd);
		    putC ("::");
		    status = doPerform (del, ob);
		} else if (!strcmp (pfm, "exposure")) {
		    // exposure
		    if ((ob = getSym (kwd)) == NULL) {
			      xerror ("exposure for an undefined object type");
			      return (NO);
		    }
		    putC ("void ");
		    putC (kwd);
		    putC ("::");
		    status = doExposure (del, ob);
		} else if (WithinExposure && !strcmp (pfm, "expose")) {
		    // This was the last chance
		    putC (kwd);
		    putC ("::");
		    status = doExpose (del);
		} else if (!strcmp (pfm, "setup")) {
		    if ((ob = getSym (kwd)) == NULL) {
Ignsetup:
			del = ':';
			EnLkpPtr = septr;
			SkLineCnt = skc;
			goto Tryother;
		    }
		    if (ob->Type != PACKET) goto Ignsetup;
	            if (del != '(') {
Setserr:
		        xerror ("setup definition syntax error");
		        return (NO);
		    }
		    while (1) { del = lkpC (); if (!isspace (del)) break; }
		    if (del == ')') goto Ignsetup;
	            del = getKeyword (pfm, del);

		    if (del != '*') return (NO);
		    if ((ox = getSym (pfm)) == NULL) return (NO);
		    if (ox->Type != MESSAGE) return (NO);

		    if (ob->States == NULL) {
	xerror ("this setup was not announced in the class declaration");
			return (NO);
		    }
		    if (strcmp ((char*)(ob->States), pfm)) {
       xerror ("Packet setup argument type collides with previous declaration");
			return (NO);
		    }
	            if (strcmp (pfm, "Message") == 0) goto Ignsetup;

		    del = getKeyword (pfm);
	            if (del != ')') goto Setserr;

		    putC (kwd);
	            putC ("::setup (Message*");
		    putC (" zz_mspar)");

		    while (1) { del = lkpC (); if (!isspace (del)) break; }

		    if (del != '{') goto Setserr;
		    BraceLevel++;
		    putC ("{ register ");
		    putC ((char*)(ob->States));
		    putC (" *");
		    putC (pfm);
		    putC (" = (");
		    putC ((char*)(ob->States));
		    putC ("*) zz_mspar;");
		    catchUp ();
		    status = YES;
		} else if (!strcmp (pfm, "pfmMQU") || !strcmp (pfm, "pfmMDE")) {
		    strcpy (pfmm, pfm);
		    pfmtype = pfmm;
		    if ((ob = getSym (kwd)) == NULL) {
IgnMQU:
			del = ':';
			EnLkpPtr = septr;
			SkLineCnt = skc;
			goto Tryother;
		    }
		    if (ob->Type != TRAFFIC) goto IgnMQU;
	            if (del != '(') {
SetMQU:
			pxerror ("definition syntax error");
		        return (NO);
		    }
	            del = getKeyword (pfm);
		    if (del != '*') {
			pxerror ("argument must be a Message subtype pointer");
			return (NO);
		    }
		    if (strcmp (pfm, "Message") == 0) goto IgnMQU;
		    if (ob->States == NULL) {
			pxerror ("illegal for a virtual Traffic type");
			return (NO);
		    }
		    if (ob->States [0] == NULL || strcmp (ob->States [0],pfm)) {
	    		pxerror ("argument type doesn't match traffic message "
				"type");
			return (NO);
		    }

		    del = getKeyword (pfm);
	            if (del != ')') goto SetMQU;

		    putC (kwd);
		    putC ("::");
		    putC (pfmtype);
	            putC (" (Message*");
		    putC (" zz_mspar)");

		    while (1) { del = lkpC (); if (!isspace (del)) break; }

		    if (del != '{') goto SetMQU;
		    BraceLevel++;
		    putC ("{ register ");
		    putC (ob->States [0]);
		    putC (" *");
		    putC (pfm);
		    putC (" = (");
		    putC (ob->States [0]);
		    putC ("*) zz_mspar;");
		    catchUp ();
		    status = YES;
		} else if (!strcmp (pfm, "pfmMTR") || !strcmp (pfm, "pfmMRC") ||
		           !strcmp (pfm, "pfmPTR") || !strcmp (pfm, "pfmPRC") ||
			   !strcmp (pfm, "pfmPDE")) {
		    strcpy (pfmm, pfm);
		    pfmtype = pfmm;
		    if ((ob = getSym (kwd)) == NULL) {
IgnPFF:
			del = ':';
			EnLkpPtr = septr;
			SkLineCnt = skc;
			goto Tryother;
		    }
		    if (ob->Type != TRAFFIC) goto IgnPFF;
	            if (del != '(') {
SetPFF:
		        pxerror ("definition syntax error");
		        return (NO);
		    }
	            del = getKeyword (pfm);
		    if (del != '*') {
		        pxerror ("argument must be a Packet subtype pointer");
			return (NO);
		    }
		    if (strcmp (pfm, "Packet") == 0) goto IgnPFF;
		    if (ob->States == NULL) {
			pxerror ("illegal for a virtual Traffic type");
			return (NO);
		    }
		    if (ob->States [1] == NULL || strcmp (ob->States [1],pfm)) {
	           pxerror ("argument type doesn't match traffic packet type");
			return (NO);
		    }

		    del = getKeyword (pfm);
	            if (del != ')') goto SetPFF;

		    putC (kwd);
	            putC ("::");
		    putC (pfmtype);
		    putC (" (Packet*");
		    putC (" zz_mspar)");

		    while (1) { del = lkpC (); if (!isspace (del)) break; }

		    if (del != '{') goto SetPFF;
		    BraceLevel++;
		    putC ("{ register ");
		    putC (ob->States [1]);
		    putC (" *");
		    putC (pfm);
		    putC (" = (");
		    putC (ob->States [1]);
		    putC ("*) zz_mspar;");
		    catchUp ();
		    status = YES;
	  	} else {
		    del = ':';
		    EnLkpPtr = septr;
		    SkLineCnt = skc;
		    goto Tryother;
		}
		goto Done;
	}

Tryother:
		
	if (!islower (*kwd)) return (NO);
	if ((f = getKey (kwd)) == NULL) return (NO);

	status = f (del);
Done:
	if (status) {
		// Make sure the output line count agrees
		while (SkLineCnt--) putC ('\n');
	}
	return (status);
}

main (int argc, char *argv []) {

/* ------------- */
/* The main loop */
/* ------------- */

	int     c;      // The current character

	// Get the 'static' prefix
	if (argc < 2) {
		SPref [0] = '\0';
	} else {
		for (argv++, c = 0; c < 11 && isdigit ((*argv)[c]); c++)
			SPref [c] = (*argv) [c];
		SPref [c] = '\0';
	}

	SFName = (argc < 3) ? NULL : *(++argv);

        argc -= 3; argv++;
        while (argc > 0) {
          if ((*argv)[1] == 'p')
            Tagging = YES;
          else if ((*argv)[1] == 'c')
            NoClient = YES;
          else if ((*argv)[1] == 'l')
            NoLink = YES;
          else if ((*argv)[1] == 'r')
            NoRFC = YES;
          else if ((*argv)[1] == 's')
            NoStats = YES;
          argc--; argv++;
        }

	// Initialize the tables
	for (c = 0; c < HASHTS; c++) {
		SymTab [c] = NULL;
		KeyTab [c] = NULL;
	}

	// Create the signature structure
	S = new Signature;

	// Add keywords
	new KeyDesc ("pfmMQU", processPFMMQU);
	new KeyDesc ("pfmMDE", processPFMMDE);
	new KeyDesc ("pfmMTR", processPFMMTR);
	new KeyDesc ("pfmMRC", processPFMMRC);
	new KeyDesc ("pfmPTR", processPFMPTR);
	new KeyDesc ("pfmPRC", processPFMPRC);
	new KeyDesc ("pfmPDE", processPFMPDE);
	new KeyDesc ("outItem", processOutitem);
	new KeyDesc ("inItem", processInitem);
	new KeyDesc ("identify", processIdentify);
	new KeyDesc ("terminate", processTerminate);
	new KeyDesc ("expose", processExpose);
	new KeyDesc ("exposure", processExposure);
	new KeyDesc ("onpaper", processOnpaper);
	new KeyDesc ("onscreen", processOnscreen);
	new KeyDesc ("exmode", processExmode);
	new KeyDesc ("getDelay", processGetDelay, YES);
	new KeyDesc ("setDelay", processSetDelay, YES);
	new KeyDesc ("transient", processTransient);
	new KeyDesc ("new", processNew);
	new KeyDesc ("states", processStates);
	new KeyDesc ("eobject", processEobject);
	new KeyDesc ("observer", processObserver);
	new KeyDesc ("inspect", processInspect);
	new KeyDesc ("timeout", processTimeout);
	new KeyDesc ("link", processLink);
	new KeyDesc ("rfchannel", processRFChannel);
	new KeyDesc ("mailbox", processMailbox);
	new KeyDesc ("traffic", processTraffic);
	new KeyDesc ("message", processMessage);
	new KeyDesc ("packet", processPacket);
	new KeyDesc ("perform", processPerform);
	new KeyDesc ("setup", processSetup);
	new KeyDesc ("process", processProcess);
	new KeyDesc ("sleep", processSleep);
	new KeyDesc ("skipto", processSkipto);
	new KeyDesc ("station", processStation);
	new KeyDesc ("proceed", processProceed);
	new KeyDesc ("sameas", processSameas);
	new KeyDesc ("create", processCreate);
	new KeyDesc ("state", processState);
	new KeyDesc ("wait", processWait, YES);
	new KeyDesc ("wait", processWait);
	new KeyDesc ("getproclist", processGetproclist);

	addSym ("Station", STATION, DEFINED, REAL);
	addSym ("EObject", EOBJECT, DEFINED, REAL);
	addSym ("Process", PROCESS, DEFINED, REAL);
	addSym ("Observer", OBSERVER, DEFINED, REAL);
        addSym ("Mailbox", MAILBOX, DEFINED, REAL);
	if (NoClient == NO) {
	  addSym ("Traffic", TRAFFIC, DEFINED, REAL);
	  addSym ("Message", MESSAGE, DEFINED, REAL, UNEXPOSABLE);
	  addSym ("Packet", PACKET, DEFINED, REAL, UNEXPOSABLE);
	  addSym ("SGroup", SGROUP, DEFINED, REAL, UNEXPOSABLE);
	  addSym ("CGroup", CGROUP, DEFINED, REAL, UNEXPOSABLE);
        }
	if (NoLink == NO) {
	  addSym ("Link", LINK, DEFINED, REAL);
	  addSym ("BLink", LINK, DEFINED, REAL);
	  addSym ("PLink", LINK, DEFINED, REAL);
	  addSym ("ULink", LINK, DEFINED, REAL);
	  addSym ("CLink", LINK, DEFINED, REAL);
	  addSym ("Port", PORT, DEFINED, REAL);
	}
	if (NoRFC == NO) {
	  addSym ("RFChannel", RFCHANNEL, DEFINED, REAL);
	  addSym ("Transceiver", TRANSCEIVER, DEFINED, REAL);
	}
	if (NoStats == NO)
	  addSym ("RVariable", RVARIABLE, DEFINED, REAL);

	// Just in case: cccp should do it in the first line
	CFName [0] = '\0';
	// Now, this is included before preprocessing
        // putC ("#include \"Smurph.h\"\n");
        while (BraceLevel) {
          if (WithinString) {
                // Processing a string or comment
            if ((c = getC ()) == END) { // End of file
UnclString:
              if (WithinString == '"' || WithinString == '\'')
                xerror ("unclosed string opened at line %1d", StrLine);
              finish ();
            }
            putC (c);
            if (c == '\\') {            // Escape, copy the next char verbatim
              if ((c = getC ()) == END) goto UnclString;
              putC (c);
              continue;
            }
                if (c == '\n' && WithinString == '/') {
                  WithinString = NO;
                  continue;
                }
            if (c == WithinString) {
                  if (WithinString == '*') {
                    if ((c = getC ()) == END) goto UnclString;
                        putC (c);
                        if (c != '/') continue;
                        WithinString = NO;
                  } else if (WithinString != '/')  WithinString = NO;
                }
            continue;
          }

	  switch (c = getC ()) {

	    case '#' :

	      // Possible line command

	      if (BOLine)
		processLineCommand ();
	      else
		putC (c);
	      continue;

	    case '\'':
	    case '"':

	      // Start of text

	      ArrowInFront = NO;
	      putC (WithinString = c);
	      StrLine = LineNumber;
	      continue;

	    case '/':
		putC (c);
		if ((c = peekC ()) == '/' || c == '*') {
			WithinString = c;
			putC (getC ());
		}
		continue;

	    case END:

	      finish ();
	      break;                    // No return from finish - just in case

	    case '}':

	      // Count braces

	      BraceLevel--;
	      ArrowInFront = NO;
	      if (CodeType) {
		if (BraceLevel == CodeTypeBC) {
		  if (CodeWasState) putC ('}');
		  CodeType = NO;
		  CodeObject = NO;
		}
	      }

	      if (CObject != NULL && CObjectBC == BraceLevel) {
		// Pop the stack
		if (CObject->Type == MAILBOX)
		    endOfMailbox ();
		else
		    putC ('}');
		emitBuilder ();
		if (DeclStackDepth) {
		  DeclStackDepth--;
		  CObject = DeclStack [DeclStackDepth] . CObject;
		  CObjectBC = DeclStack [DeclStackDepth] . CObjectBC;
		} else {
		  CObject = NULL;
		}
	      } else if (WithinExposure && BraceLevel == WithinExposure) {
		WithinExposure = NO;
		if (OnpaperDone && !OnscreenDone) {
		  putC ("ZZ_ONSC: if (zz_emode > MAX_int) goto ZZ_ONPP; ");
		} else if (OnscreenDone && !OnpaperDone) {
		  putC ("ZZ_ONPP: if (zz_emode > MAX_int) goto ZZ_ONSC; ");
		}
		putC ('}');
	      } else {
		if (WithinExpMode && BraceLevel == WithinExpMode)
		  WithinExpMode = NO;
		putC ('}');               // Put the closing brace
	      }
	      continue;

	    case '{':

	      putC ('{');
	      ArrowInFront = NO;
	      BraceLevel++;
	      continue;

	    case '-':

	      // This may be the first element of a '->'

	      putC ('-');
	      if (!ArrowInFront) ArrowInFront = 1;
	      continue;

	    case '.':

	      putC ('.');
	      if (!ArrowInFront) ArrowInFront = 2;
              continue;

	    case '>':

	      // Second part of '->' ?

	      putC ('>');
	      if (ArrowInFront == 1)
		ArrowInFront = 2;
	      else
		ArrowInFront = NO;
	      continue;

	    case '!':

	      // Two consecutive '!' outside string are removed
	      ArrowInFront = NO;
	      if (peekC () == '!') {
		// Strip them
		getC ();
	      } else {
		putC ('!');
	      }
	      continue;

	    default:

	      if (isspace (c)) {
		putC (c);
		// Don't let it destroy the arrow status
		if (ArrowInFront == 1) ArrowInFront = NO;
		continue;
	      }

	      // Check if it is the beginning of an interesting keyword

	      if (! (isalnum (c) || c == '_' || c == '$')) {
		ArrowInFront = NO;
		putC (c);
		continue;
	      }

	      if (isdigit (c) || !processKeyword (c)) {
		// Skip this keyword
		ArrowInFront = NO;
		putC (c);
		while ((c = peekC ()) == '_' || c == '$' || isalnum (c))
			putC (getC ());
	      }
	      continue;
	  }
	}
}
