// ------------------------------------- //
// Shared stuff for opening stream files //
// ------------------------------------- //

using	std::ios;

ostream *zz_openOStream (const char *fn, const char *m) {
/* -------------------------------------------- */
/* Opens an output stream for a given file name */
/* -------------------------------------------- */
	ostream *os;

	os = (*m == 'a') ?
		new ofstream (fn, ios::out | ios::app) :
			new ofstream (fn);

	if (os -> good ())
		return os;

	delete os;

	return NULL;
}

istream *zz_openIStream (const char *fn) {
/* ------------------------------------------- */
/* Opens an input stream for a given file name */
/* ------------------------------------------- */
	istream *str = new ifstream (fn);

	if (str->good ())
		return str;

	delete str;
	return NULL;
}

#ifdef	ZZ_MONHOST

// ------------------------------------------------------------------- //
//              Shared stuff for handling templates                    //
// ------------------------------------------------------------------- //

// Note: this code has been moved from the old DSD with a minimum effort. This
// is why it fits the rest rather loosely.

static int TemplateNo, LineNo;

static void  abrt (char *msg) {
/* ------------------------ */
/* Processes template error */
/* ------------------------ */
  char *t;
  t = form ("%s (template %1d, line %1d)", msg, TemplateNo, LineNo);
  tabt (t);
};

static istream *ts;
static Template *LastTemp, *Templates = NULL; 

static int PeekCh, Column, Row, Width, LineType, NFields,
           LastLineNum, LastNotRepNum;

static  Field   *LastLineStart, // Pointer to first field of this line
                *LastNotRepeat, // Last non-repeat field
                *LastField;     // Last processed field

static  RegDesc *RegList;

// Special value of the peek character meaning "absent"
#define CLEARED  (-4)

// static inline int isdigit (int c) { return c <= '9' && c >= '0'; };

Field::Field () {
/* ------------------------- */
/* General field constructor */
/* ------------------------- */
  next = NULL;
  NFields++;
  if (LastField == NULL)
    LastTemp->fieldlist = this;
  else
    LastField->next = this;
  LastField = this;
};

FixedField::FixedField (short xx, short yy) {
/* ----------------------- */
/* Fixed field constructor */
/* ----------------------- */
  type  = FT_FIXED;
  x     = xx;
  y     = yy;
  dmode = DM_NORMAL;              // Default display mode
  LastNotRepeat = this;
  LastNotRepNum = NFields-1;
};

FilledField::FilledField (short xx, short yy) {
/* ----------------------- */
/* Fixed field constructor */
/* ----------------------- */
  type  = FT_FILLED;
  x     = xx;
  y     = yy;
  dmode = DM_NORMAL;              // Default display mode
  LastNotRepeat = this;
  LastNotRepNum = NFields-1;
};

RegionField::RegionField (short xx, short yy, short xx1) {
/* ------------------------ */
/* Region field constructor */
/* ------------------------ */
  type  = FT_REGION;
  x     = xx;
  y     = yy;
  x1    = xx1;
  y1    = -1;             // Unknown at the beginning
  dmode = DM_NORMAL;
  pc    = '*';            // Defaults
  lc    = '*';
  LastNotRepeat = this;
  LastNotRepNum = NFields-1;
};

RepeatField::RepeatField (int cnt) {
/* ------------------------ */
/* Repeat field constructor */
/* ------------------------ */
  type  = FT_REPEAT;
  from  = LastLineNum;
  to    = LastNotRepNum;
  count = cnt;
};

#ifdef  CDEBUG
void    dumpTemplates () {
  Template  *t;
  int       tc, i, fc;
  Field     *f, *g;

  FixedField  *fixf;
  RepeatField *repf;
  RegionField *regf;
  FilledField *filf;

  for (tc = 0, t = Templates; t != NULL; tc++, t = t->next) {
    cdebugl (form ("Template #%1d, %s, %1d, %1d, %s",
                            tc,  t->typeidnt, t->mode, t->status, t->desc));
    cdebugl (form ("Size: %1d, %1d", t->xclipd, t->yclipd));
    cdebugl (form ("Fields (%1d):", t->nfields));

    for (fc = 0, f = t->fieldlist; f != NULL; fc++, f = f->next) {
      switch (f->type) {
        case FT_FIXED:
          fixf = (FixedField*) f;
          cdebugl (form ("Field #%1d -- FIXED: DM: %1d, XY=(%1d,%1d), CN=%s",
            fc, (int)(fixf->dmode), fixf->x, fixf->y, fixf->contents));
          break;
        case FT_REPEAT:
          repf = (RepeatField*) f;
          cdebugl (form ("Field #%1d -- REPEAT: FTC=(%1d,%1d,%1d)",
            fc, repf->from, repf->to, repf->count));
          break;
        case FT_REGION:
          regf = (RegionField*) f;
          cdebugl (form ("Field #%1d -- REGION: DM: %1d, XYXY=(%1d,%1d,%1d,%1d), PCLC=%c%c", fc, (int)(regf->dmode), regf->x, regf->y, regf->x1, regf->y1, regf->pc, regf->lc));
          break;
        case FT_FILLED:
          filf = (FilledField*) f;
          cdebugl (form ("Field #%1d -- FILLED: DM: %1d, JS: %1d, LN: %1d, XY=(%1d,%1d)", fc, (int)(filf->dmode), filf->just, filf->length, filf->x, filf->y));
          break;
        default:
          cdebugl (form ("Field #%1d -- ILLEGAL: %1d", f->type));
      }
    }
  }
}
#else
#define dumpTemplates ()
#endif

static int getChar () {
  int c;
  char cc;
  // Note: CLEARED is -4 it is supposed to be something different from a
  // character and from ENF
  if (PeekCh != CLEARED) {
    if ((c = PeekCh) != ENF) {
      PeekCh = CLEARED;
      Column++;
    }
    return c;
  } else {
    TS.get (cc);
    if (TS.eof())
      return (PeekCh = ENF);
    Column++;
    return (int) cc;
  }
};

static int peekChar () {
  char cc;
  if (PeekCh == CLEARED) {
    TS.get (cc);
    if (TS.eof ())
      PeekCh = ENF;
    else {
      PeekCh = (int) cc;
    }
  }
  return PeekCh;
};

static  void    skipToEol () {
/* -------------------------------------- */
/* Skip to the beginning of the next line */
/* -------------------------------------- */
  int     c;
  while ((c = getChar ()) != '\n') if (c == ENF) abrt ("incomplete template");
  LineNo++;
};

static  int     skipBlanks () {
/* ---------------------------------------------------------- */
/* Ignore   blanks  on  the  template  file,  stop  at  first */
/* non-blank without actually reading it                      */
/* ---------------------------------------------------------- */
  int     c;
  while ((c = peekChar ()) == ' ' || c == '\t') getChar ();
  return (c);
};

static  int     skipWhite () {
/* ------------------------ */
/* Skip blanks and newlines */
/* ------------------------ */
  int     c;
  do { c = skipBlanks (); } while (c == '\n' && ++LineNo && getChar ());
  return (c);
};

static void readTemplates () {
  if ((ts = zz_openIStream (zz_TemplateFile)) == NULL)
    abrt ("cannot open template file");
  PeekCh = CLEARED;
  RegList = NULL;
  Templates = LastTemp = NULL;
  LineNo = 1;
  TemplateNo = 0;
  while (skipWhite () != ENF) {
    new Template;
  }
  delete ts;
  // dumpTemplates ();
};

static int isLetter (int c) {
/* ---------------------------------------------------------- */
/* Checks  is  the  character is a legal first character of a */
/* name                                                       */
/* ---------------------------------------------------------- */
  if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') return YES;
  if (c == '_') return (YES);       // Underscore is also legal
  return (NO);
};

static int isDigit (int c) { return c <= '9' && c >= '0'; };

static int isAlnum (int c) { return isLetter (c) || isDigit (c); };

static  char    *getName () {
/* ----------------------------------------- */
/* Read an identifier from the template file */
/* ----------------------------------------- */
  int     cnt;
  char    tmp [SANE], *r;

  if (!isLetter (peekChar ()))
    abrt ("type name starts with a non-letter");
  cnt = 1;
  tmp [0] = getChar ();
  while (isAlnum (peekChar ()))
    if (cnt >= SANE-1)
      abrt ("type name is too long");
    else
      tmp [cnt++] = getChar ();
  // Allocate memory for the string and return pointer to it
  tmp [cnt] = '\0';
  r = new char [cnt+1];
  strcpy (r, tmp);
  return (r);
};

static  Long    getINumber () {
/* --------------------------------------------- */
/* Read an integer number from the template file */
/* --------------------------------------------- */
  int     c, pos;
  Long    res;
  if ((c = peekChar ()) == '-') {
    pos = NO;
    getChar ();
  } else {
    pos = YES;
    if (c == '+') getChar ();
  }
  for (res = 0; isdigit (peekChar ()); res = res*10 - (getChar () - '0'));
  if (pos) res = -res;
  return (res);
};

int specialChar (int &c, short *Escape) {
/* ---------------------------------------------------------- */
/* Checks  whether  the  next character is a special one. All */
/* characters within a region are special (blanks).           */
/* ---------------------------------------------------------- */
  RegDesc *r;

  if (Column >= Width) return (YES);

  // Check if within a region
  for (r = RegList; r != NULL; r = r->next) {
    if (r->xs > Column) break;
    if (r->xs == Column && peekChar () == '@' &&
      Escape [Column] != ES_NSP) {
      // Region termination
      c = '@';
      return (YES);
    }
    if (r->xe > Column) {
      c = ' ';
      return (YES);
    } else if (r->xe == Column && peekChar () == '@' &&
     Escape [Column] != ES_NSP) {
      c = '@';
      return (YES);
    }
  }
  c = peekChar ();
  if (Escape [Column] == ES_NSP) return (NO);
  if (c == '%' || c == '&' || c == '@' || c == ' ' || c == '\n') return (YES);
  return (NO);
};

int convertChar (short *Escape) {
/* ---------------------------------------------------------- */
/* Convert  a  character  to a normal printable form. Returns */
/* blanks if within a region.                                 */
/* ---------------------------------------------------------- */
  int     c;
  RegDesc *r;

  // Check if within a region
  for (r = RegList; r != NULL; r = r->next) {
    // Just relax
    if (r->xs > Column) break;
    if (r->xs == Column && peekChar () == '@' && Escape [Column] != ES_NSP) {
      // Region termination
      return (getChar ());
    }
    if (r->xe > Column) {
      getChar ();
      return (' ');
    } else if (r->xe == Column && peekChar () == '@' &&
      Escape [Column] != ES_NSP) {
      // Termination
      return (getChar ());
    }
  }
  if ((c = getChar ()) == '~') // This stands for a honorary blank
    return (' ');
  else
    return (c);
};

RegDesc::RegDesc (short xst, short xen, RegionField *r) {
/* -------------------------------------- */
/* The constructor of a region descriptor */
/* -------------------------------------- */
  RegDesc *p, *q;

  xs = xst;
  xe = xen;
  rfp = r;
  // Put the new description onto sorted list
  if (RegList == NULL) {
    // The list is empty
    next = prev = NULL;
    RegList = this;
    return;
  }
  for (p = RegList; p != NULL; q = p, p = p->next) {
    if (p->xs >= xs) {
      if (p->xe >= xs) abrt ("overlapping regions");
      break;
    }
  }
  if (p == RegList) {
    // Add in front
    next = RegList;
    prev = NULL;
    RegList->prev = this;
    return;
  }
  q->next = this;
  prev = q;
  if ((next = p) != NULL) p->prev = this;
};

RegDesc::~RegDesc () {
/* --------------------------------------------------- */
/* Removes the region description from the sorted list */
/* --------------------------------------------------- */
  if (next != NULL) next->prev = prev;
  if (prev != NULL)
    prev->next = next;
  else
    RegList = next;
};

void    Template::layoutLine (int wid, short *E) {
/* ----------------------- */
/* Process one layout line */
/* ----------------------- */
  FixedField      *FixF;
  FilledField     *FilF;
  RegionField     *RegF;

  int             FirstInLine = YES, c, i;
  char            Txt [SANE];

  while (1) {     // Loop for fields
    if (!specialChar (c, E)) {
      // The beginning of a regular 'as is' field
      FixF = new FixedField (Column, Row);
      if (FirstInLine) {
        LastLineStart = FixF;
        LastLineNum = NFields - 1;
        FirstInLine = NO;
      }
      Txt [0] = convertChar (E);
      for (i = 1; !specialChar(c, E) && E[Column] != ES_FLD;
        i++)
          Txt [i] = convertChar (E);
      Txt [i] = '\0';
      FixF->contents = new char [i+1];
      strcpy (FixF->contents, Txt);
      if (Column > wid)
        abrt ("fixed field outside window bounds");
      continue;
    }
    // Here we have a special character
    if (Column >= wid) {
      // We have reached the end of line
      if (peekChar () != '|')
           abrt ("layout line does not terminate with '|'");
      return;
    }
    if (c == '\n') abrt ("layout line ends prematurely");
    if (c == ' ') {         // Skip blanks
      while (1) {
        if (Column >= wid) break;
        specialChar (c, E);
        if (c == ' ') convertChar (E); else break;
      }
      continue;
    }
    if (c == '%' || c == '&') {
      // Filled field
      int     sc;
      sc = c;         // The starting character
      FilF = new FilledField (Column, Row);
      FilF->just = (c == '%') ? RIGHT : LEFT;
      if (FirstInLine) {
        LastLineStart = FilF;
        LastLineNum = NFields - 1;
        FirstInLine = NO;
      }
      FilF->length = 1;
      convertChar (E);
      while (specialChar (c, E) && c == sc &&
       E[Column] != ES_FLD && Column < wid) {
        convertChar (E);
        FilF->length ++;
      }
      if (Column > wid) abrt ("filled field outside window bounds");
      continue;
    }
    if (c == '@') {         // Region
      int         x;
      RegDesc     *rd;
      x = Column;
      // Look for a non-special matching '@'
      convertChar (E);
      while (!specialChar (c, E) || c != '@') {
        if (Column >= wid) abrt ("unmatched region"); else convertChar (E);
      }
      if (RegList != NULL) {
        // Check if it is not a closing frame
        c = NO;
        for (rd = RegList; rd != NULL; rd = rd->next)
          if (rd->xs == x) {
            if (rd->xe != Column) abrt ("tilted region");
            rd->rfp->y1 = Row;
            delete (rd);
            c = YES;
            break;
          }
        if (c) {
          convertChar (E);
          continue;
        }
      }
      RegF = new RegionField (x, Row, Column);
      new RegDesc (x, Column, RegF);
      if (FirstInLine) {
        LastLineStart = RegF;
        LastLineNum = NFields - 1;
        FirstInLine = NO;
      }
      convertChar (E);
      continue;
    }
    abrt ("internal error -- illegal special character");
  }
};

/* ------------------------------ */
/* A few functions for fieldFlags */
/* ------------------------------ */

static  int     ddmm;                   // Display mode
static  char    acc, bcc;               // Line/point display characters
static  int     mdset, pcset,           // Flags to detect duplicate setting
    lcset;

int     static  getAttr () {
/* ---------------------------------------------------------- */
/* Decodes  one  attribute, stores it in the above variables, */
/* and returns the attribute type                             */
/* ---------------------------------------------------------- */
  int     c;
  while (1) {
    if ((c = skipBlanks ()) == ',') {
      getChar ();
      continue;
    }
    if (c == '\n') return (AT_END);
    break;
  }
  if (c == 'n' || c == 'N') {
    getChar ();
    ddmm = DM_NORMAL;
    return (AT_DMO);
  }
  if (c == 'r' || c == 'R') {
    getChar ();
    ddmm = DM_REVERSE;
    return (AT_DMO);
  }
  if (c == 'h' || c == 'H') {
    getChar ();
    ddmm = DM_BRIGHT;
    return (AT_DMO);
  }
  if (c == 'b' || c == 'B') {
    getChar ();
    ddmm = DM_BLINKING;
    return (AT_DMO);
  }
  if (c == '\'') {
    // Expect region display character(s)
    getChar ();
    if ((c = peekChar ()) == '\'' || c == '\n')
      abrt ("illegal display character specification");
    acc = c;
    getChar ();
    if ((c = peekChar ()) == '\'') {
      getChar ();
      return (AT_DCH);
    }
    if (c == '\n') abrt ("illegal display character specification");
    bcc = c;
    getChar ();
    if (peekChar () != '\'') abrt ("illegal display character specification");
    getChar ();
    return (AT_DCS);
  }
  return (AT_END);
};

static  void    setDMode (Field *f) {
  if (f == NULL) abrt ("superfluous field attribute(s)");
  if (mdset) abrt ("display mode set twice for the same field");
  mdset = YES;
  ((FilledField*)f)->dmode = ddmm;
};

static  void    setDChar (Field *f) {
  RegionField     *rf;
  if (f == NULL) abrt ("superfluous field attribute(s)");
  if (f->type != FT_REGION)
    abrt ("attempt to set display char for a non-region field");
  rf = (RegionField*) f;
  if (!pcset) {
    rf->lc = rf->pc = acc;
    pcset = YES;
    return;
  }
  if (lcset) abrt ("display char defined twice");
  lcset = YES;
  rf->lc = acc;
};

static  void    setDChrs (Field *f) {
  RegionField     *rf;
  if (f == NULL) abrt ("superfluous field attribute(s)");
  if (f->type != FT_REGION)
    abrt ("display chars for a non-region field");
  rf = (RegionField*) f;
  if (pcset) abrt ("display chars defined twice");
  pcset = lcset = YES;
  rf->pc = acc;
  rf->lc = bcc;
};

void    fieldFlags () {
/* -------------------------------------------- */
/* Process field flags for the last layout line */
/* -------------------------------------------- */
  Field           *f;
  int             More;

  f = LastLineStart;      // The first field of the last line
  while (1) {
    mdset = pcset = lcset = NO;
    if (skipBlanks () == '(') {
      // A group
      getChar ();
      for (More = YES; More;) {
        switch (getAttr ()) {
          case AT_DMO:
            setDMode (f);
            break;
          case AT_DCH:
            setDChar (f);
            break;
          case AT_DCS:
            setDChrs (f);
            break;
          case AT_END:
            if (getChar () != ')')
              abrt ("missing ')' in attribute specification");
            More = NO;
        }
      }
    } else {
      switch (getAttr ()) {
        case AT_DMO:
          setDMode (f);
          break;
        case AT_DCH:
          setDChar (f);
          break;
        case AT_DCS:
          setDChrs (f);
          break;
        case AT_END:
          if (peekChar () != '\n') abrt ("illegal attribute specification");
          else
            return;
      }
    }
    if (f != NULL) f = f->next;
    if (f == NULL) {
      if (skipBlanks () != '\n') abrt ("too many attribute specifications");
      return;
    }
  }
};

Template::Template () {
/* ------------------------------------------- */
/* Reads a new template from the template file */
/* ------------------------------------------- */
  int       c, More, LRow, i, j;
  short     Escape [SANE];
  char      ds [SANE];
  RepeatField *repf;

  next = NULL;
  if (Templates == NULL)
    Templates = this;
  else
    LastTemp->next = this;
  LastTemp = this;
  TemplateNo ++;
  // Decode type name
  typeidnt = getName ();
  if (skipBlanks () == ENF) abrt ("incomplete template header");
  if ((mode = (int)getINumber ()) < 0) abrt ("negative exposure mode");
  if (skipBlanks () == ENF) abrt ("incomplete template header");
  if (peekChar () == '*') {
    getChar ();
    if (peekChar () == '*') {
      status = TS_FLEXIBLE;
      getChar ();
    } else
      status = TS_RELATIVE;
  } else
    status = TS_ABSOLUTE;
  // Get the comment text
  skipWhite ();
  if ((c = getChar ()) != '"') abrt ("description text missing");
  j = 256-1;
  for (i = 0; (c = getChar ()) != ENF; i++) {
    if (c == '"') break;
    if (c == '\n')
      abrt ("description string contains newline(s)");
    if (c == '\\')
      if ((c = getChar ()) == ENF) break;
    if (i >= j) continue;
    ds [i] = c;
  }
  if (c == ENF) abrt ("unterminated description text");
  ds [i++] = '\0';
  desc = new char [i];
  strcpy (desc, ds);
  // Skip everything to the first line starting with 'b' or 'B'
  while ((c = peekChar ()) != ENF) {
    if (c == '\n') {
      getChar ();
      LineNo++;
      Column = 0;
      continue;
    }
    if ((c == 'b' || c == 'B') && Column == 0) break;
    getChar ();
  }
  if (c == ENF) abrt ("premature EOF");
  // Here we have the first line of the template
  xclipd = yclipd = -1;   // Undefined yet
  getChar ();             // Swallow the 'b'
  Column = 0;             // Start counting from here

  while ((c = peekChar ()) != 'b' && c != 'B' && c != '\n') {
    if (c == '+') {
      // Horizontal clipper
      if (xclipd != -1) abrt ("default x-clipping defined more than once");
      xclipd = Column;
    }
    getChar ();
  }
  Width = Column;
  if (xclipd == -1) xclipd = Width-1;

  LRow = Row = 0;
  More = YES;
  LastLineStart = LastField = LastNotRepeat = NULL;
  NFields = 0;

  // Clear the escape array
  for (c = 0; c < Width; c++) Escape [c] = 0;

  while (More) {
    skipToEol ();                 // Move to the beginning of next line
    c = getChar ();               // Remove the EOL
    Column = 0;
    switch (c) {
      case 'e':                   // The end of the template
      case 'E':
        More = NO;
        break;
      case 'x':                   // The exceptions line
      case 'X':
        LineType = LT_ESC;
        if (Row == -1) abrt ("'**' followed by 'exceptions' line");
        while ((c = peekChar ()) != '\n' && c != ENF) {
          if (Column < Width) {
            if (c == '|')
              Escape [Column] = ES_NSP;
            else if (c == '+')
              Escape [Column] = ES_FLD;
          }
          getChar ();
        }
        if (c == ENF) abrt ("Escape line incomplete");
        // Start for a new line with escape array set
        continue;
      case '|':                   // A regular field line
        LineType = LT_LAY;
        if (Row == -1) abrt ("'**' followed by layout line");
        layoutLine (Width, Escape);
        if (Row != -1) Row++;
        break;
      case '*':                   // Replication
        LineType = LT_REP;
        if (Row == -1) abrt ("'**' followed by another replication line");
        if (Row == 0 || LastLineStart == NULL)
          abrt ("nothing to duplicate -- '*' illegal");
        if (RegList != NULL) abrt ("'*' illegal within region");
        if (peekChar () == '*') {
          if (yclipd == -1) yclipd = Row - 1;
          c = 0;          // Count undefined
          LRow = Row;
          Row = -1;       // Nondeterministic height
        } else {
          if ((c = (int)getINumber ()) < 0) abrt ("illegal repetition count");
          if (c == 0) c = 1;
          Row += c;
        }
        // Now we should generate a repeat field. However, we will try to
        // compress subsequent fields referring to the same area.
        if (LastField != NULL && LastField->type == FT_REPEAT && 
          (repf = (RepeatField*)LastField)->from == LastLineNum &&
           repf                           ->to   == LastNotRepNum ) {
                 // Compress
                 if (c == 0)
                   repf->count = 0;  // indefinite repeat
                 else if (repf->count != 0)
                   repf->count += c;
                 // Otherwise ignore the new repeat because the previous
                 // already indefinite
        } else
          new RepeatField (c);
    }
    if (! More) break;            // After 'e' line
    // Skip to '|' or EOL, whatever comes first
    while ((c = peekChar ()) != '|' && c != '\n') {
      if (c == ENF) abrt ("incomplete template line");
      getChar ();
    }
    if (c == '|') {               // Possible attributes to follow
      getChar ();
      if ((c = peekChar ()) == '+') {
        // Clipping
        if (Row == -1) abrt ("illegal cliping with undefined height");
        if (yclipd != -1) abrt ("default y-clipping defined more than once");
        yclipd = Row-1;
        getChar ();
      }
      // Process field flags
      if (LineType == LT_LAY) fieldFlags ();
    }
    if (LineType == LT_LAY)
    // Clear the escape array
      for (c = 0; c < Width; c++) Escape [c] = 0;
  }       // End while (More)
  while ((c = getChar ()) != '\n') if (c == ENF) break;
  LineNo++;
  if (RegList != NULL) abrt ("incomplete region");
  // Vertical clipping
  if (yclipd == -1) {
    if (Row == -1)
      yclipd = LRow-1;
    else
      yclipd = Row-1;
  }
  nfields = NFields;
};

static int tmatch (Template *t, const char *nm) {
// ---------------------------------
// Matches template with object name
// ---------------------------------
  char *tn;
  tn = t->typeidnt;
  while (*nm == *tn) {
    if (*nm == '\0') return YES;
    nm++; tn++;
  }
  if (*nm == ' ' && *tn == '\0') return YES;
  return NO;
};

void Template::send () {
// ------------------
// Sends one template
// ------------------
  Field *f;
  FixedField      *fixf;
  FilledField     *filf;
  RegionField     *regf;
  RepeatField     *repf;
  int fc;

  sendOctet (PH_TPL);
  sendText (typeidnt);
  sendInt (mode);
  sendChar (status);
  sendText (desc);
  sendInt (xclipd);
  sendInt (yclipd);
  sendInt (nfields);
  for (fc = 0, f = fieldlist; f; f = f->next, fc++) {
    sendChar (f->type);
    switch (f->type) {
      case FT_FIXED:
        fixf = (FixedField*) f;
        sendChar (fixf->dmode);
        sendInt (fixf->x);
        sendInt (fixf->y);
        sendText (fixf->contents);
        break;
      case FT_FILLED:
        filf = (FilledField*) f;
        sendChar (filf->dmode);
        sendChar (filf->just);
        sendInt (filf->x);
        sendInt (filf->y);
        sendInt (filf->length);
        break;
      case FT_REPEAT:
        repf = (RepeatField*) f;
        sendInt (repf->from);
        sendInt (repf->to);
        sendInt (repf->count);
        break;
      case FT_REGION:
        regf = (RegionField*) f;
        sendChar (regf->dmode);
        sendInt (regf->x);
        sendInt (regf->y);
        sendInt (regf->x1);
        sendInt (regf->y1);
        sendChar (regf->pc);
        sendChar (regf->lc);
        break;
      default:
        tabt ("illegal field -- internal error");
    }
  }
  if (fc != nfields) abrt ("illegal field count -- internal error");
};

#endif    // ZZ_MONHOST
