// ------------------------------------------------------------------------- //
// This stuff is used by display.c and the monitor (in ../MONITOR/monitor.c) //
// ------------------------------------------------------------------------- //

#ifdef	CDEBUG
#define dumpbuf(b) (b)->dump ()
#else
#define dumpbuf(b)
#endif

class SockBuff {
// --------------------------------
// Socket buffer (input and output)
// --------------------------------
  public:
    unsigned char *Buffer;
    int size, fill, ptr;
    SockBuff (int sz) {
      Buffer = new unsigned char [size = sz];
      clear ();
    };
#ifdef	CDEBUG
    void dump () {
      int i, ip, li;
      unsigned char line [80];
      cdebugl ("Buffer dump");
      for (ip = li = i = 0; i < fill; i++) {
        line [ip] = tohex ((Buffer [i] & 0xf0) >> 4);
        ip++;
        line [ip] = tohex (Buffer [i] & 0x0f);
        ip++;
        line [ip++] = ' ';
        if ((i+1) % 16 == 0 || i == fill-1) {
          line [ip++] = ' '; line [ip++] = ' ';
          while (li <= i) {
            line [ip] = Buffer [li++];
            if (line [ip] < 0x20 || line [ip] > 0x7f) line [ip] ='.';
            ip++;
          }
          line [ip] = '\0';
          cdebugl ((char*)line);
          ip = 0;
        }
      }
    }
#endif
    ~SockBuff () {
      delete [] Buffer;
    };
    int tlength () {
      return (Buffer [1] << 16) | (Buffer [2] << 8) | Buffer [3];
    };
    void grow (int nsize = 0) {
      unsigned char *NewBuffer;
      int i;
      while ((size += size) < nsize);
      NewBuffer = new unsigned char [size];
      for (i = 0; i < fill; i++) NewBuffer [i] = Buffer [i];
      delete [] Buffer;
      Buffer = NewBuffer;
    };
    void reset (unsigned char b) {
      Buffer [0] = b;
      fill = 4; 
      ptr = 0;
    };
    int header () {
      ptr = 4;
      return Buffer [0];
    };
    void close () {
      Buffer [1] = (fill >> 16) & 0xff;
      Buffer [2] = (fill >>  8) & 0xff;
      Buffer [3] = (fill & 0xff);
      ptr = 0;
    };
    void out (unsigned char b) {
      if (fill == size) grow ();
      Buffer [fill++] = b;
    };
    int inp () {
      if (ptr == fill) return 0;
      // We are not supposed to read past the end of buffer
      return Buffer [ptr++];
    };
    int peek () {
      if (ptr == fill) return 0;
      return Buffer [ptr];
    };
    int more () {
      return fill - ptr;
    };
    void clear () {
      fill = ptr = 0;
    };
    int read (int, int block = NO);
    int write (int, int block = NO);
    int empty () {
      return (fill == 0);
    };
};
