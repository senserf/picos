#ifndef __ptransmitter_h__
#define __ptransmitter_h__

// Defines global variables and announces the transmitter type for Piggyback
// Ethernet.

#define Left  0          // Piggyback directions
#define Right 1

#define piggyDirection(p) (flagSet (p->Flags, PF_usr0))
#define setPiggyLeft(p)   (clearFlag (p->Flags, PF_usr0))
#define setPiggyRight(p)  (setFlag (p->Flags, PF_usr0))

extern int  MinIPL,      // Minimum inflated packet length (frame excluded)
            MinUPL,      // Minimum uninflated packet length
            MaxUPL,      // Maximum packet length (payload)
            PFrame;      // Frame length

// JamL and PSpace are borrowed from Ethernet, but we need these to have
// their TIME versions

extern TIME TPSpace,     // After a collision
            TJamL;

extern RATE TRate;

extern Long DelayQuantum;

extern DISTANCE L;       // Actual bus length

process PTransmitter (PiggyStation) {
  Port *Bus;
  Packet *Buffer;
  int CCounter;
  Mailbox *Ready;
  void inflate (), deflate ();
  TIME backoff ();
  virtual void setPiggyDirection () { };
  void setup ();
  states {NPacket, WaitBus, Piggyback, XDone, Abort, EndJam, Error};
  perform;
};

#endif
