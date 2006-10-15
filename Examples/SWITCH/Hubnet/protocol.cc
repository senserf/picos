#include "types.h"

Long MinPL, MaxPL, FrameL;   // Packetization parameters
TIME SndRecTime;             // Sender recognition delay

HubProcess::perform {
  state Wait:                                 // Wait for a packet
    SPort->wait (BOT, NewPacket);
  state NewPacket:                            // A packet arrives
    if (S->Busy) skipto Wait;                 // Hub busy, ignore
    S->Busy = YES;                            // Reserve the hub
    BPort->transmit (ThePacket, Done);
  state Done:                                 // Stop packet transmission
    BPort->stop ();
    S->Busy = NO;                             // Release the hub
    proceed Wait;                             // Wait for more
};

Transmitter::perform {
  state NewPacket:                            // Attempt to acquire a packet
    if (S->ready (MinPL, MaxPL, FrameL))
      proceed Retransmit;
    else
      Client->wait (ARRIVAL, NewPacket);      // No ready packet
  state Retransmit:                           // Transmit or retransmit
    SPort->transmit (Buffer, Done);
    StartEW->put ();                          // Wait for packet echo
    NACK->wait (RECEIVE, Lost);
  state Done:
    SPort->stop ();                           // Terminate the packet
    NACK->wait (RECEIVE, Retransmit);         // Wait for a signal from
    ACK->wait (RECEIVE, Confirmed);           // monitor
  state Confirmed:                            // Release the packet
    Buffer->release ();
    proceed NewPacket;                        // Take care of the next one
  state Lost:
    SPort->abort ();
    proceed Retransmit;
};

Receiver::perform {
  state Wait:                                 // Wait for the end of packet
    BPort->wait (EMP, NewPacket);
  state NewPacket:                            // Accept the packet
    Client->receive (ThePacket, BPort);
    skipto (Wait);                            // and continue
};

AClock::perform {
  state Start:                                // After creation
    Timer->wait (Delay, GoOff);
  state GoOff:                                // Play the trumpet
    F->signal ();
    terminate;
};

Monitor::perform {
  state WaitSignal:                           // Wait for a waking signal
    StartEW->wait (RECEIVE, WaitEcho);
  state WaitEcho:                             // Setup the alarm clock
    AC = create AClock (EchoTimeout);
    proceed Waiting;
  state Waiting:                              // Continue waiting for echo
    BPort->wait (BOT, NewPacket);
    TheProcess->wait (SIGNAL, NoEcho);
  state NewPacket:                            // Determine the packet sender
    Pkt = ThePacket;
    Timer->wait (SndRecTime, CheckEcho);
    TheProcess->wait (SIGNAL, NoEcho);
  state CheckEcho:
    if (Pkt -> Sender == S->getId ()) {
      if (erase () == 0)
        // No pending Signal from the Trumpet
        AC -> terminate ();
      ACK->put ();
      proceed WaitSignal;
    } else
      proceed Waiting;
  state NoEcho:                               // Timeout
    NACK->put ();
    proceed WaitSignal;
};
