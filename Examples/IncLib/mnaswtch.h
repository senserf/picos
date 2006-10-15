#ifndef __mnaswitch_h__
#define __mnaswitch_h__

#include "mesh.h"

// This is the type definition file for the MNA protocol. It is used to define
// a number of different MNA configurations (see directories MNA* in MESH).

extern Long MinPL,         // Minimum packet length
            MaxPL,         // Maximum packet length
            FrameL;        // Header and trailer
            
extern RATE TRate;         // Made global: used in Host setup

station Switch : MeshNode {
  unsigned char **PRanks;  // Routing table (port ranks per destination)
  Boolean *Idle;           // Status of output ports
  virtual Port *iPort (int i) { return IPorts [i]; };
  int route (Packet*);
  void setup (int order) {
    int i;
    MeshNode::configure (order);
    Idle = new Boolean [order];
    for (i = 0; i < order; i++) Idle [i] = YES;
  };
};

void assignPortRanks ();

station Host : Switch, ClientInterface {
  Port **NDelay, **XDelay; // Ports to delay links
  int *Used;               // Status of IPorts (packets in the tunnel)
  Mailbox *FreePort;       // To signal port availability to transmitters
  Port *iPort (int i) { return XDelay [i]; };
  void setup (int order, DISTANCE delay) {
    int i;
    PLink *lk;
    Switch::setup (order);
    ClientInterface::configure (order);  // Param = the number of buffers
    NDelay = new Port* [order];
    XDelay = new Port* [order];
    Used = new int [order];
    FreePort = create Mailbox (0);
    for (i = 0; i < order; i++) {
      NDelay [i] = create Port (TRate);
      XDelay [i] = create Port (TRate);
      lk = create PLink (2);
      NDelay [i] -> connect (lk);
      XDelay [i] -> connect (lk);
      NDelay [i] -> setDTo (XDelay [i], delay);
      Used [i] = 0;
    }
  };
};

process Router (Switch) {
  Port *IPort, *OPort;
  int  OP;  // Output port index (selected dynamically)
  void setup (int p) { IPort = S->iPort (p); };
  states {Waiting, NewPacket, WaitEnd, EndPacket, WaitRcv, Rcv};
  perform;
};

process InDelay (Host) {
  Port *IPort, *NPort;
  int MP;
  void setup (int p) {
    IPort = S->IPorts [MP = p];
    NPort = S->NDelay [MP];
  };
  states {Waiting, In, WaitEOT, RDone};
  perform;
};

process OutDelay (Host) {
  Port *XPort;
  int MP;
  Mailbox *FreePort;
  void setup (int p) {
    XPort = S->XDelay [MP = p];
    FreePort = S->FreePort;
  };
  states {Waiting, Out};
  perform;
};

process Transmitter (Host) {
  Port *XPort;
  int BF, XP, Order;
  Packet *Buffer;
  Mailbox *FreePort;
  void setup (int b) {
    Buffer = S->Buffer [BF = b];
    Order = S->Order;
    FreePort = S->FreePort;
    // Note: XPort set dynamically based on port availability
  };
  states {NewMessage, RetryRoute, EndXmit, Error};
  perform;
};
    
#endif
