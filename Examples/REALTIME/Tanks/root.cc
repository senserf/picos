#include "types.h"
#include "board.h"
#include "panel.h"
#include "sensor.cc"

process Root {
  states {Start, Stop};
  perform {
    state Start:
      initBoard ();
      initSystem ();
      initPanel ();
    state Stop:
      terminate;
  };
};
