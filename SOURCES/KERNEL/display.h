/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */



/* ------------------------- */
/* Display-related constants */
/* ------------------------- */

/* ---------------------------------------------------------- */
/* Master socket node name for the single-machine version  of */
/* the monitor                                                */
/* ---------------------------------------------------------- */
#define         SOKNAME                 "monitor_socket"

/* ---------------------------------------------------------- */
/* Short timeout,  e.g.,  to determine whether the monitor is */
/* there at all.                                              */
/* ---------------------------------------------------------- */
#ifndef		STIMEOUT
#define         STIMEOUT                30      // Seconds
#endif

/* ---------------------------------------------------------- */
/* Connection timeout: after that many seconds smurph assumes */
/* that the server couldn't make it                           */
/* ---------------------------------------------------------- */
#define         CTIMEOUT                900     // Seconds == 15 minutes

/* -------------------------------------------------------- */
/* Connection timeout when display is requested by the user */
/* -------------------------------------------------------- */
#define         UTIMEOUT                1800    // Seconds == 30 minutes

/* ------------------------ */
/* Default display interval */
/* ------------------------ */
#if ZZ_REA
// In real mode, this is in milliseconds -- one second
#define         DEFDINTVL               1000    // Milliseconds
#else
#define         DEFDINTVL               5000    // Events
#endif

/* ----------------------------------------- */
/* The maximum length of the supplement list */
/* ----------------------------------------- */
#define         MAXSUPPL                512     // Items

/* --------------------------------------------------- */
/* Minimum size of the monitor output and input buffer */
/* --------------------------------------------------- */
#define		MINMOBSI                8192
#define         MINMIBSI                 256

/* ---------------------------------------------------------- */
/* Async I/O granularity. The size of an atomic write request */
/* ---------------------------------------------------------- */
#define         AIOGRANL                8192

/* --------------------------------------------------------------------- */
/* Initial number of credits. Used to determine the number of updates we */
/* can send in advance before we should get something in response.       */
/* --------------------------------------------------------------------- */
#define		INITCRED                   2

/* ------------------------- */
/* Maximum number of credits */
/* ------------------------- */
#define         MAXXCRED                  12

/* ------------ */
/* Phrase bytes */
/* ------------ */
#define         PH_BLK                  255     // Start of update block
#define         PH_MEN                  254     // Menu request
#define         PH_INM                  253     // Integer number
#define         PH_FNM                  252     // Floating-point number
#define         PH_BNM                  251     // BIG number
#define         PH_TXT                  250     // Text
#define         PH_STA                  249     // Status request
#define         PH_STP                  248     // Step
#define         PH_DIN                  247     // Display interval
#define         PH_BPR                  246     // Initial parameters
#define         PH_DRQ                  245     // Display request
#define         PH_REM                  244     // Remove an object
#define         PH_WND                  243     // Request window
#define         PH_EWN                  242     // Cancel window
#define         PH_CRE                  241     // Credit message
#define         PH_REG                  240     // Region
#define         PH_REN                  239     // Region end
#define         PH_SLI                  238     // Region line segment
#define         PH_ERR                  237     // Error indication
#define         PH_END                  236     // Terminate display
#define         PH_EXI                  235     // Terminate execution
#define		PH_SIG			234     // Signature block
#define         PH_UPM                  233     // Up menu request
#define         PH_REL                  232     // Relative step delay
#define         PH_ESM                  231     // Exit step mode
#define         PH_TRN                  230     // Truncate
#define         PH_SRL                  228     // Step release
#define         PH_TPL                  227     // Template
#define		PH_MBL			226	// Monitor block
#define         PH_MIN                  226     // The current minimum
// ------------------------------------------------- //
// Note: minimum phrase code is 224, i.e., 1110 0000 //
// ------------------------------------------------- //

/* ----------------------------------- */
/* Numerical descriptors special codes */
/* ----------------------------------- */

#define         DSC_DOT                 10      // Decimal point
#define         DSC_EXP                 11      // Exponent
#define         DSC_ENN                 12      // End of negative number
#define         DSC_ENP                 13      // End of positive number

// Note:  descriptor  codes  14  and 15 are illegal. No legal byte can
//        look like a phrase code. This puts a lower limit to the legal
//        phrase code, which is 224.

/* ----------- */
/* Error codes */
/* ----------- */

#define         DERR_ILS                0       // Illegal spurt (formal error)
#define         DERR_NSI                1       // Negative display interval
#define         DERR_ONF                2       // Object not found
#define         DERR_NMO                3       // Nonexistent mode
#define         DERR_ISN                4       // Illegal station number
#define         DERR_CUN                5       // No window to unstep
#define         DERR_NOT                6       // Note
#define         DERR_UNV                7       // Not available
#define		DERR_NRP		8	// Response timeout
#define         DERR_DSC                9       // Disconnected

/* ----------------------------- */
/* ORQ (outgoing requests) types */
/* ----------------------------- */
#define		ORQ_MENU                257
#define         ORQ_UPMENU              258
#define         ORQ_REMOVE              259     // Window object removal
#define         ORQ_TEMPLATE            260     // Window template
#define		ORQ_END                 261     // End handshake

/* ------------ */
/* Window flags */
/* ------------ */
#define         WF_OPENING              1
#define         WF_CLOSING              2

/* ---------------- */
/* Smurph signature */
/* ---------------- */
#define         SMRPHSIGN               "smurphzz"

/* ---------------- */
/* Server signature */
/* ---------------- */
#define         SERVRSIGN               "dservers"  /* Changed for SIDE */

/* ---------------------------------------------------------- */
/* Note:  the  above signatures can be changed, but they must */
/* be of the same length determined by the constant below     */
/* ---------------------------------------------------------- */
#define         SIGNLENGTH              8

// -------------------
// Stuff for templates
// -------------------
#define ENF  NONE
#define TS   (*ts)
#define SANE 512       // Limit for scratch array size

/* ------------------- */
/* Template line types */
/* ------------------- */
#define         LT_LAY          0       // Layout
#define         LT_ESC          1       // Escape
#define         LT_REP          2       // Replication

/* ------------ */
/* Escape codes */
/* ------------ */
#define         ES_NSP          1       // Non-special character
#define         ES_FLD          2       // Field separation

/* ----------------------------- */
/* Justification for fixed field */
/* ----------------------------- */
#define         LEFT            0
#define         RIGHT           1

/* ---------------------------- */
/* Display item attribute types */
/* ---------------------------- */
#define         AT_END          0       // No more attributes
#define         AT_DMO          1       // Display mode
#define         AT_DCH          2       // Region display character
#define         AT_DCS          3       // Region display characters
// Scaling attributes have been eliminated

/* ------------- */
/* Display modes */
/* ------------- */
#define         DM_NORMAL       0
#define         DM_REVERSE      1
#define         DM_BLINKING     2
#define         DM_BRIGHT       3

/* ----------- */
/* Field types */
/* ----------- */
#define         FT_FIXED        0       // Fixed contents
#define         FT_REPEAT       1       // Replication of last line
#define         FT_REGION       2       // Region description
#define         FT_FILLED       3       // Filled dynamically

/* -------------------------------------------------- */
/* Constants describing relative status of a template */
/* -------------------------------------------------- */
#define         TS_ABSOLUTE     1       // No station-relative
#define         TS_RELATIVE     2       // Mandatory relative
#define         TS_FLEXIBLE     3       // Relative or not
    // I don't know why these constants were negative in the previous
    // version

class   RegionField;                    // Forward

class   RegDesc {
/* -------------------------------------------------------- */
/* A local data structure representing a region description */
/* -------------------------------------------------------- */
  public:
    RegDesc *next, *prev;               // List pointers
    short   xs, xe;                     // Column bounds
    RegionField     *rfp;               // Field pointer
    RegDesc (short, short, RegionField*);
    ~RegDesc ();
};

class   Field           {
/* ----------------------------------- */
/* The internal description of a field */
/* ----------------------------------- */
  public:

  Field   *next;                  // List pointer
  char    type;                   // The type of this field
  Field ();
};

class   FixedField : public Field {
/* ------------------------------ */
/* A field with constant contents */
/* ------------------------------ */
  public:

  FixedField (short, short);

  char    dmode;                  // Display mode
  short   x, y;                   // Starting coordinates
  char    *contents;              // The stuff to be displayed
};

class   FilledField : public Field {
/* -------------------------------- */
/* A field to be filled dynamically */
/* -------------------------------- */
  public:

  FilledField (short, short);

  char    dmode,                  // Display mode
          just;                   // Justification (LEFT, RIGHT)
  short   x, y,                   // Starting coordinates
          length;                 // The size
};

class   RegionField : public Field {
/* --------------------------- */
/* A field describing a region */
/* --------------------------- */
  public:

  RegionField (short, short, short);

  char    dmode;                  // Display mode
  short   x, y, x1, y1;           // Corner coordinates
  char    pc, lc;                 // Point/line display characters
  // Scaling attributes have been eliminated
};

class   RepeatField : public Field {
/* ---------------------------------------------------------- */
/* A  dummy  field  describing  replication  of a sequence of */
/* previous fields                                            */
/* ---------------------------------------------------------- */
  public:

  int     from, to;               // Replication boundaries
  int     count;                  // Replication count 0 == unlimited
  // Note: regions cannot occur within the replication boundaries
  RepeatField (int);
};

class   Template        {
/* ----------------------------------------------------- */
/* This is the internal description of a window template */
/* ----------------------------------------------------- */
  public:

  Template  *next;  // List pointer

  char  *typeidnt;  // Object type identifier
  int   mode;       // The exposure mode
  short status;     // How the window can be related to a station
  char  *desc;      // Description text
	// Legal values for 'status' are:
	//         -1  -- no station-relative
	//         -2  -- only station-relative (any station)
	//         -3  -- optionally station relative
  int     nfields;        // The number of fields
  Field   *fieldlist;     // List of fields
  short   xclipd,         // Default clipping column
          yclipd;         // Default clipping row
  Template ();
  void    layoutLine (int, short*);
  void    send ();
};
