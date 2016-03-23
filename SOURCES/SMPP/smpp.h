/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-03   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

// #define         DEBUG


#include "../version.h"

#define	XLong ZZ_XLong

#include        "../LIB/lib.h"
#include        "../LIB/syshdr.h"

IPointer	MemoryUsed;

/* ----------------- */
/* Boolean constants */
/* ----------------- */

#define YES     1
#define NO      0
#define END     (-1)
#define ERROR   (-2)
#define	NOTHING	ERROR
#define	ENDFILE	END

/* --------------------------------------------- */
/* Types and constants for the SMPP preprocessor */
/* --------------------------------------------- */

#define STDIN   0                       // I/O streams
#define STDOUT  1
#define STDERR  2

#define IBSIZE  16384                   // Input buffer size
#define	MXPREP	4096			// Maximum length of prep line
#define OBSIZE  1024                    // Output buffer size
#define MFNLEN  PATH_MAX+1              // Maximum length of a filename
#define MSTACK  256                     // Process declaration stack size
#define	HASHTS	256			// Hash table size (must be power of 2)

#define	LNAMESIZE 64			// Should be at least as big as
					// defined in mks.c

char    IBuffer [IBSIZE+1],             // This is a circular buffer
	OBuffer [OBSIZE];               // And this is a regular one

char    *FirstP = IBuffer,              // Input buffer pointers
	*LimitP = IBuffer + (IBSIZE + 1);
char    *InP = FirstP, *OutP = FirstP, *EnLkpPtr, *EnSPrepD = NULL;

int     Eof = NO,                       // YES, if EOF on input
	EnLkpEOL = NO;			// YES, if EOL during skipping

char    *StartP = OBuffer,              // Output buffer pointers
	*EndP = OBuffer+OBSIZE;
char    *FreeP = StartP;

char    CFName [MFNLEN],                // The current file name
	SPref [12];			// The 'static' prefix


/* ------------ */
/* Symbol types */
/* ------------ */

#define PROCESS         1
#define OBSERVER        2
#define EOBJECT         3
#define STATION         4
#define TRAFFIC         5
#define MESSAGE         6
#define PACKET          7
#define LINK            8
#define	PORT		9
#define	RVARIABLE	10
#define	SGROUP		11
#define	CGROUP		12
#define	MAILBOX		13
#define	RFCHANNEL	14
#define	TRANSCEIVER	15
#define UNKNOWN         999

/* ------------- */
/* Symbol status */
/* ------------- */

#define ANNOUNCED       0
#define DEFINED         1

/* ----------- */
/* Symbol mode */
/* ----------- */

#define VIRTUAL         0
#define REAL            1
#define FREE            2
#define	ABSTRACT	3

/* --------------- */
/* Exposure status */
/* --------------- */

#define	UNEXPOSABLE	2
#define EXPOSABLE	1

/* ------------------------ */
/* Maximum number of errors */
/* ------------------------ */

#define	MAXERRCNT	12

/* ---------------------------- */
/* Special symbols to be caught */
/* ---------------------------- */

#define MAXKWDLEN       2048            // Maximum length of an identifier

class	SymDesc;
SymDesc *addSym (const char*, int, int stts=ANNOUNCED,
		 int mode=FREE, int estat=EXPOSABLE);

class   SymDesc {                       // A symbol description

	friend	SymDesc *addSym (const char*, int, int stts,
					int mode, int estat);
	friend  SymDesc *getSym (const char*);

	private:
	SymDesc *next;                  // Rehash list pointer

	public:
	SymDesc	*Parent;		// Parent type (process only)
	char    *Name;                  // The identifier
	int     Type;                   // Qualification
	char 	Status,                 // Defined/Announced
		Mode,                   // Virtual / real / free
		EStat,			// Exposure status
		PAnn;			// 'perform' announced (process only)

	char	**States;		// State list (process only)

	SymDesc (const char*, int, int, int, int, int estat); // Constructor
};

typedef	int (*KFUNC)(int);		// Pointer to a key processing functn

class	KeyDesc {			// A keyword descriptor

	friend	KFUNC	getKey (char*);

	private:
	KeyDesc	*next;			// Rehash list pointer

	public:
	char	*Name;			// The keyword text
	int	Arrow;			// True if expected after '->'
	KFUNC	PFunct;			// Processing function

	KeyDesc (const char*, KFUNC, int arr = NO); // The constructor
};

SymDesc *SymTab [HASHTS];		// Symbol table
KeyDesc *KeyTab [HASHTS];		// Keyword table

class   Signature {

        char LIndex [LNAMESIZE];        // Library index
        int  NSFiles,                   // Number of lists
             MaxNSFiles,                // Maximum
             LastChar,
	     BufPtr, BufSize;
	char *Buffer;
        int FilD, EndFF;		// File descriptor
        char ***CL,                     // The array of lists
	      **TL;			// Currently built list
	int  TLLength, TLMaxLength;	// Length attributes of TL

        public:

        Signature ();
        int empty () { return CL == NULL; };
        int update (const char*);
        int pkc (), chr (), nbl ();
	void outsig (char*), outsig (char), flsig (), close ();
};

/* ----- */
/* Flags */
/* ----- */

int 	WithinExposure = NO,            // Within exposure definition
	WithinExpMode = NO,             // Within 'onpaper' or 'onscreen'
	OnpaperDone = NO,               // The 'onpaper' part done
	OnscreenDone = NO,              // The 'onscreen' part done
	PerformDeclared = NO,           // Perform method declared
	WithinString = NO,              // Within a text string
	BOLine = YES,                   // First character of a line
	ErrorsFound = NO,               // True if errors were found
	ArrowInFront = NO,              // True if the last symbol was ->
	Identified = NO,                // True if 'identify' occurred
	InItemDecl = NO,		// 'inItem' declared for the mailbox
	OutItemDecl = NO;		// 'outItem' declared for the mailbox

int     BraceLevel = 16384,	      	// Counter of "{,}" pairs
	LineNumber = 1,                 // Input line number
	SkLineCnt,                      // Counter for skipped lines
	StrLine,                        // Where the current string started
	CurrChar = '\n';                // Current character

SymDesc	*CObject = NULL,		// Current object dictionary entry
	*CodeObject = NULL;		// The object owning the 'perform'

int	CObjectBC,			// Brace level of the current object
	CodeType = NO,			// Tells if parsing 'perform'
	CodeTypeBC,
	CodeWasState,			// At least one state defined
	WasExmode;			// Not the first exmode within exposure

struct	DeclStackFrame {		// Declaration stack frame

	public:

	SymDesc	*CObject;		// Object pointer
	int	CObjectBC;
};

int     DeclStackDepth = 0;             // Depth of process/observer declaration
					// stack

char	*SFName = NULL;			// Source file name

int	Tagging = NO,			// Three-argument wait
	NoClient = NO,			// Client not compiled
	NoLink = NO,			// Link/port not compiled
	NoRFC = NO,			// Radio channels not compiled
	NoStats = NO;			// RVariables not compiled

Signature *S = NULL;			// Signature pointer

					// Process/observer declaration stack
DeclStackFrame DeclStack [MSTACK];

void	xerror (const char*, ...);
