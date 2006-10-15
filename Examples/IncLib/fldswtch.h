#ifndef __floodswitch_h__
#define __floodswitch_h__

#include "mesh.h"

// This is the type definition file for the FLOODNET protocol. It is based
// on the 'mesh' configuration, similarly as MNA.

extern Long MinPL,         // As usual: minimum packet length
            MaxPL,         // Maximum packet payload length
            FrameL,        // The length of header and trailer
            NoiseL;        // Noise packet length
            
extern TIME MinActTime,    // Duration of a shortest recognizable activity
            RcvRecDelay,   // Receiver recognition time
            FloodTime,     // Network flooding time for backoff
            PSpace;        // Packet spacing
            
extern RATE TRate;         // Time granularity: ITUs/bit
            
station Switch : MeshNode {
  Mailbox *IdleSignal,     // To signal the idle status of the switch
          *RelaySignal,    // To signal relay request to port servers
          *StopSignal,     // To signal end of packet to port servers
          *AbortSignal,    // To abort packet relaying
          *BlockSignal;    // To signal the block condition
  int NActive,             // Counts ports being active relaying
      NBouncing;           // Counts inactive ports to reach Idle consensus
  Boolean Idle;            // Switch status
  Packet Noise,            // The noise packet
         *RPacket;         // Pointer to the packet being relayed
  void setup (int order) {
    MeshNode::configure (order);
    IdleSignal  = create Mailbox (0);
    RelaySignal = create Mailbox (0);
    StopSignal  = create Mailbox (0);
    AbortSignal = create Mailbox (0);
    BlockSignal = create Mailbox (0);
    Idle = YES;
    // Relaying will be initialized by output processes
    Noise.fill (NONE, NONE, NoiseL);
  };
};

station Host : MeshNode, ClientInterface {
  Packet Noise;
  void setup () {
    MeshNode::configure (1);
    ClientInterface::configure ();
    Noise.fill (NONE, NONE, NoiseL);
  };
};

process PortServer (Switch) {
  Port *IPort, *OPort; // The ports serviced by the process
  TIME RStarted;       // Time when the relay operation was started
  int Order;
  Packet *Noise;
  Mailbox *IdleSignal, *RelaySignal, *StopSignal, *AbortSignal, *BlockSignal;
  void setup (int p) {
    Order = S->Order;
    IPort = S->IPorts [p];
    OPort = S->OPorts [p];
    Noise = &(S->Noise);
    IdleSignal  = S->IdleSignal;
    RelaySignal = S->RelaySignal;
    StopSignal  = S->StopSignal;
    AbortSignal = S->AbortSignal;
    BlockSignal = S->BlockSignal;
  };
  states {Idle, GrabIt, Relaying, Stop, Abort, Bouncing, Bounce, NDone,
          DAbort, Quit, WaitEOT, CheckEOT, Blocked};
  perform;
};

process Receiver (Host) {
  Port *IPort, *OPort;
  Packet *RcvPacket, *Noise;
  void setup () {
    IPort = S->IPorts [0];
    OPort = S->OPorts [0];
    Noise = &(S->Noise);
  };
  states {Idle, NewPacket, WaitEOT, CheckMy, NDone, CheckEOT};
  perform;
};

process Transmitter (Host) {
  Port *IPort, *OPort;
  TIME TStarted;       // Time when the relay operation was started
  Packet *Buffer;
  TIME backoff ();
  int BlockCount;
  void setup () {
    IPort = S->IPorts [0];
    OPort = S->OPorts [0];
    Buffer = &(S->Buffer);
  };
  states {Acquire, Waiting, XDone, Abort, Quit};
  perform;
};
    
#endif
