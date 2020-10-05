/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __lrwtexp_c__
#define __lrwtexp_c__

// This is the exposure code for local request waiting time shared by rpctrfl.c and
// fstrfl.cc

RQTraffic::exposure {
  int i, NS; LONG count;
  double X [MAXSTATIONS], Y [MAXSTATIONS], min, max, mom [2], Max;
  Boolean First;
  Traffic::expose;
  onpaper {
    exmode 4:
      if (Hdr == NULL) Hdr = "Local request waiting times";
      print (Hdr);
      print ("\n\n");
      for (i = 0, First = YES; i < NStations; i++)
        if (CInt [i] != NULL)
          if (First) {
            CInt [i] -> RWT -> printACnt ();
            First = NO;
          } else
            CInt [i] -> RWT -> printSCnt ();
      print ("\n");
      RWT->printCnt ("Global request waiting time");
  };
  onscreen {
    exmode 4:
      // Store the delays
      for (i = 0, NS = 0, Max = 0.0; i < NStations; i++) {
        X [i] = (double) i;
        if (CInt [i] != NULL) {
          NS = i;
          CInt [i] -> RWT -> calculate (min, max, mom, count);
          if ((Y [i] = mom [0]) > Max) Max = Y [i];
        } else
          Y [i] = 0.0;
      }
      startRegion (0.0, (double) NS, 0.0, Max * 1.05);
      displaySegment (022, NS+1, X, Y);
      endRegion ();
	  RWT->displayOut (0);
  }
};

void printLocalMeasures () { RQTP->printOut (4); };

#endif
