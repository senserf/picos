// #define CDEBUG

#ifdef	CDEBUG

#define cdebug(s) ::write (2, s, strlen (s))
#define cdebugl(s) do { cdebug (s); cdebug ("\n"); } while (0)

static void cdhex (void *p, int nb = 0) {
  char e [10]; int i, k;
  for (i = 7; i >= 0; i--) {
    k = (int) p & 0xf;
    e [i] = tohex (k);
    p = (void*) ((long) p >> 4);
  }
  if (nb)
    e [8] = ' ';
  else
    e [8] = '\n';
  e [9] = '\0';
  cdebug (e);
};

static void cdebugs (const char *s, int n) {
  char e [81], *ep, b;
  int fill, i, d;
  i = fill = 0;
  ep = e;
  while (1) {
    if ((i == n && fill) || fill == 10) {
      // Break the line
      *ep = '\0';
      cdebugl (e);
      fill = 0;
      ep = e;
    }
    if (i == n) break;
    b = s [i];
    d = ((b & 0xf0) >> 4) & 0x0f;
    *ep = tohex (d);
    ep++;
    d = b & 0x0f;
    *ep = tohex (d);
    ep++;
    *ep++ = ' ';
    *ep++ = '(';
    if (b < 0x20 || b > 0x7f) b = ' ';
    *ep++ = b;
    *ep++ = ')';
    *ep++ = ',';
    *ep++ = ' ';
    i++;
    fill++;
  }
};

#define OCT2(b,x)\
	t = 077 & (a >> ((b)-5));\
	e [x]   = (char) ('0' + (t / 8));\
	e [x+1] = (char) ('0' + (t & 07));

#define	DEC2(b,x)\
	t = 037 & (a >> ((b)-4));\
	e [x]   = (char) ('0' + (t / 10));\
	e [x+1] = (char) ('0' + (t % 10));

#define HEX4(b,x)\
 t = 0xffff & (a >> ((b)-15));\
 tt=t%16; e[x+3]= tohex (tt); t/=16;\
 tt=t%16; e[x+2]= tohex (tt); t/=16;\
 tt=t%16; e[x+1]= tohex (tt); t/=16;\
 tt=t   ; e[x  ]= tohex (tt);

#ifdef	mips
static void cdf1 (long a) {
  char e [15]; long t, tt;
  strcpy (e, "00/00/00/0000 ");
  OCT2 (31,0);
  DEC2 (25,3);
  DEC2 (20,6);
  HEX4 (15,9);
  cdebug (e);
};

static void cdf2 (long a) {
  char e [4]; long t;
  strcpy (e, "00/");
  OCT2 (31,0);
  cdebug (e);
  t = (a & 0377777777) << 2;
  cdhex ((void*) t, 1);
};

static void cdf3 (long a) {
  char e [19]; long t;
  strcpy (e, "00/00/00/00/00/00\n");
  OCT2 (31,0);
  DEC2 (25,3);
  DEC2 (20,6);
  DEC2 (15,9);
  DEC2 (10,12);
  OCT2 (5,15);
  cdebug (e);
};

static void cdump (void *a, void *b) {
  while ((unsigned) a <= (unsigned) b) {
    cdhex (a, 1);
    cdhex ((void*) (*((long*) a)), 1);
    cdf1 (*((long*) a));
    cdf2 (*((long*) a));
    cdf3 (*((long*) a));
    a = (void*) ((char*)a + 4);
  }
};
#else
#define cdump(a,b)
#endif

#else

#define cdebug(s)
#define cdebugl(s)
#define cdhex(s)
#define cdump(a,b)
#define cdebugs(s,n)

#endif
