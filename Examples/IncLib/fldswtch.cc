#ifndef __floodswitch_c__
#define __floodswitch_c__

#include "fldswtch.h"
#include "mesh.cc"

// This is the FLOODNET protocol based on the 'mesh' configuration,
// similarly as MNA.

Long MinPL,         // As usual: minimum packet length
     MaxPL,         // Maximum packet payload length
     FrameL,        // The length of header and trailer
     
     NoiseL;        // Noise packet length
            
TIME MinActTime,    // Minimum duration of an aborted packet
     RcvRecDelay,   // Receiver recognition time
     FloodTime,     // Flooding time
     PSpace;        // Packet spacing
     
RATE TRate;         // Time granularity

PortServer::perform {
  TIME t;
  state Idle:
    if (!S->Idle) proceed Relaying;   // Somebody has grabbed it already
    IPort->wait (BOT, GrabIt);
    RelaySignal->wait (NEWITEM, Relaying);
  state GrabIt:
    // Avoid race
    if (S->Idle) {
      S->RPacket = ThePacket;
      S->Idle = NO;
      RelaySignal->put ();
      S->NBouncing = 0;
      S->NActive = Order - 1;
      skipto WaitEOT;
    }
  transient Relaying:
    RStarted = Time;
    OPort->startTransfer (S->RPacket);
    StopSignal->wait (NEWITEM, Stop);
    AbortSignal->wait (NEWITEM, Abort);
    IPort->wait (ACTIVITY, DAbort);
  state Stop:
    OPort->stop ();
    proceed Bouncing;
  state Abort:
    OPort->abort ();
  transient Bouncing:
    if (++(S->NBouncing) == Order) {
      // All processes are in this state -- switch to Idle
      S->Idle = YES;
      IdleSignal->put ();
      proceed Idle;
    }
    IdleSignal->wait (NEWITEM, Idle);
    IPort->wait (BOT, Bounce);
  state Bounce:
    // Avoid race from the previous state
    if (S->Idle) proceed Idle;
    S->NBouncing--;  // Step out temporarily
    OPort->transmit (Noise, NDone);
  state NDone:
    OPort->abort ();
    proceed Bouncing;
  state DAbort:
    if ((t = Time - RStarted) < MinActTime) {
      Timer->wait (MinActTime - t, Quit);
      sleep;
    }
  transient Quit:
    OPort->abort ();
    if (--(S->NActive) == 0) BlockSignal->put ();
    proceed Bouncing;
  state WaitEOT:
    IPort->wait (ANYEVENT, CheckEOT);
    BlockSignal->wait (NEWITEM, Blocked);
  state CheckEOT:
    if (IPort->events (EOT)) {
      assert (S->NActive, "Late block");
      StopSignal->put ();
    } else
      // Note: no need to block an aborted packet
      AbortSignal->put ();
    proceed Bouncing;
  state Blocked:
    OPort->transmit (Noise, NDone);
};

Receiver::perform {
  state Idle:
    IPort->wait (BOT, NewPacket);
  state NewPacket:
    RcvPacket = ThePacket;
    if (OPort->busy ()) skipto Idle;
    skipto WaitEOT;
  state WaitEOT:
    Timer->wait (RcvRecDelay-1, CheckMy);
    IPort->wait (ANYEVENT, Idle);
  state CheckMy:
    if (RcvPacket->isMy ())
      IPort->wait (ANYEVENT, CheckEOT);
    else
      OPort->transmit (Noise, NDone);
  state NDone:
    OPort->abort ();
    proceed Idle;
  state CheckEOT:
    if (IPort->events (EOT))
      Client->receive (RcvPacket, IPort);
    proceed Idle;
};

TIME Transmitter::backoff () {
  return toss (BlockCount) * FloodTime; 
};
      
Transmitter::perform {
  TIME t;
  state Acquire:
    if (!S->ready (MinPL, MaxPL, FrameL)) {
      Client->wait (ARRIVAL, Acquire);
      sleep;
    }
    BlockCount = 0;
  transient Waiting:
    if (IPort->busy ()) {
      IPort->wait (SILENCE, Waiting);
      sleep;
    }
    if (OPort->busy ()) {
      OPort->wait (SILENCE, Waiting);
      sleep;
    }
    TStarted = Time;
    OPort->transmit (Buffer, XDone);
    IPort->wait (ACTIVITY, Abort);
  state XDone:
    OPort->stop ();
    Buffer->release ();
    Timer->wait (PSpace, Acquire);
  state Abort:
    if ((t = Time - TStarted) < MinActTime) {
      Timer->wait (MinActTime - t, Quit);
      sleep;
    }
  transient Quit:
    OPort->abort ();
    BlockCount++;
    Timer->wait (PSpace + backoff (), Waiting);
};

#endif
