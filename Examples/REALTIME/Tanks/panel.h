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

#include "types.h"
#include "board.h"

#include "server.h"

#ifndef __panel_h__
#define	__panel_h__

#define	DEFAULT_UPDATE_DELAY         0.1   // This is in seconds
#define FORCED_UPDATE_DELAY	     5.0   // Also in seconds
#define OPBUFSIZE                   1024   // Mailbox buffer size
#define	MAXFILL                    32767   // For scaling tank fill

#define	DISCONNECTDELAY		     1.0   // Seconds

#define  CMD_UPDATE      0
#define  CMD_GETMAP      2
#define  CMD_DISCONNECT  8

#define	 CMDHDRL         3                 // Command header length

#define  CMDHDRL    3          // Header length
#define  COMMAND    (((int)((unsigned char)(CommandBuffer [0]))) & 0xff)
#define  CMDLEN0    (((int)(CommandBuffer [1])) & 0xff)
#define  CMDLEN1    (((int)(CommandBuffer [2])) & 0xff)
#define  CMDLENGTH  ((CMDLEN0<<8) + CMDLEN1)

#define	 TANK_UPDATE_SIZE   3
#define  PUMP_UPDATE_SIZE   2

#define	 MINCMDBUFSIZE     64              // Starting size of the command buf

#define  upper(a)   ((unsigned char)((a) >> 8))
#define  lower(a)   ((unsigned char)((a) & 0xff))

#define  byte0(a)      ((unsigned char)(((a) >> 24) & 0xff))
#define  byte1(a)      ((unsigned char)(((a) >> 16) & 0xff))
#define  byte2(a)      ((unsigned char)(((a) >>  8) & 0xff))
#define  byte3(a)      ((unsigned char)( (a)        & 0xff))

#define  extshort(ix) \
(((((int)(CommandBuffer[(ix)]))&0xff)<<8)+(((int)(CommandBuffer[(ix)+1]))&0xff))

process Receiver;

process Updater {
  TIME LastUpdate;
  int *TFill, *PStat;
  char *UpdateBuffer;
  int UpdateFill;
  Socket *Sk;
  Receiver *MyReceiver;
  void reset ();
  void setup (Socket*, Receiver*);
  states {Init, Update};
  perform;
};

process Receiver {
  Socket *Sk;
  char *CommandBuffer;
  int CommandBufferLength, CommandLength;
  Updater *MyUpdater;
  void resize (int);
  void setup (Socket*);
  states {WaitCommand, WaitTail, SendMap, Disconnect, Disconnecting};
  perform;
};

void initPanel ();

#endif
