#ifndef __server_h__
#define __server_h__

// Definitions for implementing socket-based servers

mailbox Socket (int);

process Server {
  Socket *Master;
  void setup (int);
  states {WaitConnection};
  perform;
};

#define readSocket(sk,buf,len,wai,fai) \
          do { \
            if (sk->read (buf, len)) { \
              if (sk->isActive ()) { \
                sk->wait (len, wai); \
                sleep; \
              } else { \
                proceed (fai); \
              } \
            }\
          } while (0)

#define readToSentinel(sk,buf,len,wai,fai) \
          do { \
            if (sk->readToSentinel (buf, len) <= 0) { \
              if (sk->isActive ()) { \
                sk->wait (SENTINEL, wai); \
                sleep; \
              } else { \
                proceed (fai); \
              } \
            }\
          } while (0)

#define writeSocket(sk,buf,len,wai,fai) \
          do { \
            if (sk->write (buf, len)) { \
              if (!(sk->isActive ())) proceed (fai); \
              sk->wait (OUTPUT, wai); \
              sleep; \
            } \
          } while (0)

#define disconnectClient(sk,again,timeout) \
          do { \
            if (sk->isConnected ()) { \
              sk->erase (); \
              if (sk->disconnect (CLIENT) == ERROR) { \
                Timer->wait (timeout, again); \
                sleep; \
              } \
            }\
            delete (sk); \
          } while (0)

#endif
