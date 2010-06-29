#ifndef __ctransmitter_h__
#define __ctransmitter_h__

// This file defines the process type describing the shared transmitter code
// for TCR and DP, and possibly other collision protocols in which colliding
// stations play a 'tournament' to determine the transmission order. This
// process type must be extended to redefine the virtual methods.

extern TIME SlotLength,      // The length of a virtual slot (2L + epsilon)
            GuardDelay,      // To compensate for non-zero clock tolerance
            TPSpace,         // Precomputed TIME versions of PSpace and JamL
            TJamL;

extern RATE TRate;           // Time granularity (read from data file)

process CTransmitter abstract (EtherStation) {
  Port *Bus;          // A copy of the bus port
  Packet *Buffer;     // Packet buffer pointer
  Boolean TournamentInProgress, Transmitting, Competing;
  void setup () {
    Bus = S->Bus;
    Buffer = &(S->Buffer);
    TournamentInProgress = Transmitting = Competing = NO;
  };
  virtual void onCollision () = 0;   // When the station senses a collision
  virtual void onEndSlot () = 0;     // End of an empty slot
  virtual void onEOT () = 0;         // Sensing end of a valid transmission
  virtual Boolean participating () {
    // Tells whether station is making a move
    return YES;
  };
  states {NPacket, Transmit, Busy, XDone, SenseEOT, SenseCollision, EndJam,
          NewSlot, EndSlot};
  perform;
};

#endif
