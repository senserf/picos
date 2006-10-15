/* ----------------------------------------------------------------- */
/* Virtual socket layer -- for a transition from UNIX to DOS/Windows */
/* ----------------------------------------------------------------- */

// Client
int openClientSocket (const char*, int);        // TCP/IP
int openClientSocket (const char*);             // UNIX

// Server
int openServerSocket (int);                     // TCP/IP
int openServerSocket (const char*);             // UNIX
int getSocketConnection (int);                  // Accept

// Both
int setSocketUnblocking (int);
int setSocketBlocking (int);

void closeAllSockets ();

// Standard BSD stuff
#define readSocket(s,b,c)  ::read (s, b, c)
#define writeSocket(s,b,c)  ::write (s, b, c)
#define closeSocket(s)  ::close(s)
#define myHostName(s,n)  ::gethostname (s, n)
