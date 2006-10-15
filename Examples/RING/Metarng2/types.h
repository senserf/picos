#define PrivateQualifier

#include "mring.h"
#include "utraffi2.h"

extern Long MinPL, MaxPL, FrameL, HdrL,    // Packetization parameters
            SATLength;
            
extern TIME SATTimeout;

extern int K, L;                           // Parameters of the SAT mechanism

// This is the same as in 'Insertion'
mailbox PQueue (Packet*) {
  Long MaxLength,    // Maximum length of the buffer in bits
       CurLength;    // Combined length of all packets in the buffer
  TIME TSTime;       // Time when the first packet became ready
  Long free () {     // Returns the length of the used portion
    return CurLength == 0 ? MaxLength : 
      MaxLength - CurLength + (Long) (Time - TSTime);
  };
  void inItem (Packet *p) {
    if (CurLength == 0) TSTime = Time;
    CurLength += p->TLength;
  };
  void outItem (Packet *p) {
    CurLength -= p->TLength;
    TSTime = Time;
    delete p;
  };
  void setup (Long l) {
    MaxLength = l;
    CurLength = 0;
    setLimit ((MaxLength + MinPL + FrameL - 1) / (MinPL + FrameL));
  };
};

station MStation : MRingInterface, ClientInterface {
  PQueue *IBuffer [2];
  Packet SATPkt;
  Boolean Blocked [2],     // Incoming packet, transmitter blocked
          SATFlag [2];     // SAT pending
  int Count [2];
  void setup (Long bufl) {
    int i;
    MRingInterface::configure ();
    ClientInterface::configure ();
    for (i = 0; i < 2; i++) {
      IBuffer [i] = create PQueue (bufl);
      Blocked [i] = NO;
      SATFlag [i] = NO;
      Count [i] = 0;
      SATPkt.fill (NONE, NONE, SATLength);
    }
  };
};

process SATSender;

process Transmitter (MStation) {
  PQueue *IBuffer;
  Packet *Buffer;
  Boolean *Blocked;
  int *Count;
  SATSender *SATSnd;
  int Direction;
  void setup (int dir) {
    Direction = dir;
    IBuffer = S->IBuffer [dir];
    Buffer = &(S->Buffer [dir]);
    Blocked = &(S->Blocked [dir]);
    Count = &(S->Count [dir]);
  };
  states {Acquire};
  perform;
};

process Input (MStation) {
  Port *IRing;
  PQueue *IBuffer;
  Transmitter *Xmitter;  // Pointer to the transmitter process
  Packet *Pkt;           // Used to save pointer to the incoming packet
  Boolean *Blocked;
  void setup (int dir) {
    IRing = S->IRing [dir];
    IBuffer = S->IBuffer [dir];
    Blocked = &(S->Blocked [dir]);
  };
  states {WaitBOT, NewPacket, CheckRcv, Receive, Drop};
  perform;
};

process Relay (MStation) {
  Port *ORing;
  PQueue *IBuffer;
  void setup (int dir) {
    ORing = S->ORing [dir];
    IBuffer = S->IBuffer [dir];
  };
  states {WaitPacket, XDone};
  perform;
};

process SATReceiver (MStation) {
  Port *ISat;
  Boolean *SATFlag;
  SATSender *SATSnd;
  void setup (int dir) {
    ISat = S->ISat [dir];
    SATFlag = &(S->SATFlag [dir]);
  };
  states {WaitSAT, Receive};
  perform;
};

process SATSender (MStation) {
  Port *OSat;
  Boolean *SATFlag;
  int *Count, Direction;
  Transmitter *Xmitter;
  SATReceiver *SATRcv;
  Packet *SATPkt;
  void setup (int dir) {
    Direction = dir;
    OSat = S->OSat [dir];
    SATFlag = &(S->SATFlag [dir]);
    Count = &(S->Count [1 - Direction]);
    SATPkt = &(S->SATPkt);
  };
  states {WaitSend, CheckSend, XDone};
  perform;
};
    
  
