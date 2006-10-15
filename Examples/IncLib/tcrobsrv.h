#ifndef __tcrobserver_h__
#define __tcrobserver_h__

#define Left  NO            // Subtree indicators
#define Right YES

observer TCRObserver {
  int CurrentLevel;
  Boolean *Tree, **Players, TournamentInProgress;
  TIME CollisionCleared;
  void newPlayer (), removePlayer (), validateTransmissionRights (),
       descend (), advance ();
  states {SlotStarted, EmptySlot, CleanBOT, Success, StartCollision,
          ClearCollision, CollisionBOT, Descend};
  void setup ();
  perform;
};

#endif
