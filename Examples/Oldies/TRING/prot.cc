/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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
identify "Token Ring";

int     MinPL, MaxPL, FrmL, HdrL, TokL, TRate;

/* ----- */
/* Types */
/* ----- */


packet Token {

/* ---------------------------------------------------------- */
/* The  type  of  the token packet (no special attributes are */
/* required)                                                  */
/* ---------------------------------------------------------- */

   void setup (int TokL) {

	TP = NONE;
	ILength = TLength = TokL;
   };

};

Traffic    *TPtrn;              // The traffic pattern

station Node {

/* --------------------------------------------------------- */
/* A standard ring station with 2 ports and 2 packet buffers */
/* --------------------------------------------------------- */

    Port   *IPort,              // Input port (never used to transmit anything)
	   *OPort;              // Output port

    Mailbox *Xmit, *Pass;       // To synchronize IProcess and OProcess

    Packet RBuffer,             // Relay buffer
	   PBuffer;             // Own packet buffer

    void setup () {
      Xmit = create Mailbox (1);
      Pass = create Mailbox (1);
      IPort = create Port (TRate);
      OPort = create Port (TRate);
    };

};

/* ---------------------------------------------- */
/* A macro to tell the token packet from the rest */
/* ---------------------------------------------- */

#define         ISTOKEN(p)      ((p)->TP == NONE)

/* --------- */
/* Processes */
/* --------- */

process IProcess (Node) {

/* ------------------------------------ */
/* The process servicing the input port */
/* ------------------------------------ */

  Port   *IPort;
  Packet *RBuffer;

  void setup () {
    IPort    = (S->IPort);
    RBuffer  = &(S->RBuffer);
    if (ident (S) == 0) {
      Token *tk;
      tk = create Token (TokL);
      *RBuffer = *tk;
      S->Xmit->put ();
    }
  };

  states {WaitPacket, Receiving, Recognized, GotToken, Received};
  perform;
};

IProcess::perform {

      state WaitPacket:
	IPort->wait (BOT, Receiving);
      state Receiving:
	*RBuffer = *ThePacket;
	Timer->wait (IPort->bitsToTime (HdrL), Recognized);
      state Recognized:
	if (ISTOKEN (RBuffer)) proceed (GotToken);
	if (RBuffer->isMy()) {
	  IPort->wait (EOT, Received);
	} else {
	  S->Pass->put ();
	  proceed (WaitPacket);
	}
      state GotToken:
	S->Xmit->put ();
	proceed (WaitPacket);
      state Received:
	Client->receive (ThePacket, ThePort);
	proceed (WaitPacket);
};

process OProcess (Node) {

/* ------------------------------------- */
/* The process servicing the output port */
/* ------------------------------------- */

  Port   *OPort;
  Packet *PBuffer, *RBuffer;

  void setup () {
    OPort    = S->OPort;
    RBuffer  = &(S->RBuffer);
    PBuffer  = &(S->PBuffer);
  };

  states {Start, XmitOwn, XmitOther, OwnDone, OtherDone};

  perform {

      state Start:
	S->Xmit->wait (RECEIVE, XmitOwn);
	S->Pass->wait (RECEIVE, XmitOther);
      state XmitOwn:
	if (Client->getPacket (PBuffer, MinPL, MaxPL, FrmL))
	  OPort->transmit (PBuffer, OwnDone);
	else
	  OPort->transmit (RBuffer, OtherDone);
      state XmitOther:
	  OPort->transmit (RBuffer, OtherDone);
      state OwnDone:
	  OPort->stop ();
	  PBuffer->release ();
	  OPort->transmit (RBuffer, OtherDone);
      state OtherDone:
	  OPort->stop ();
	  proceed (Start);
  };
};

process Root {

/* ----------------------------------------- */
/* The 'Main' process starting everything up */
/* ----------------------------------------- */

  int       NNodes;

  void  genTopology (), genTraffic (), genProtocol ();

  states {Start, Stop};

  perform {

      state Start:
	setEtu (1000);
	setTolerance (0.0001);
	setLimit (2000);
	genTopology ();  genTraffic ();  genProtocol ();
	Kernel->wait (DEATH, Stop);
      state Stop:
	System->printTop ("Network topology");
	Client->printDef ("Traffic parameters");
	TPtrn->printPfm ();
  };
};

void Root::genTopology () {

  int     NChannels;
  long    LkLength;
  Link    *lk;
  Node    *st1, *st2;
  int     i;

  readIn (NChannels);
  NNodes = NChannels;
  readIn (LkLength);
  readIn (TRate);

  for (i = 0; i < NNodes; i++) {
    create Link (2);
    create Node;
  }

  for (i = 0; i < NChannels; i++) {
    lk = idToLink (i);
    st1 = (Node*) idToStation (i);
    st2 = (Node*) idToStation ((i+1) % NNodes);
    st1->OPort -> connect (lk);
    st2->IPort -> connect (lk);
    st1->OPort -> setDTo (st2->IPort, LkLength);
  }
};

void Root::genTraffic () {

  double  MeanMIT, MeanMLE;

  readIn (MeanMIT);
  readIn (MeanMLE);

  TPtrn = create Traffic (MIT_exp+MLE_exp, MeanMIT, MeanMLE);
  TPtrn->addSender ();
  TPtrn->addReceiver ();

  readIn (MinPL);
  readIn (MaxPL);
  readIn (FrmL);
  readIn (HdrL);
  readIn (TokL);
};

void Root::genProtocol () {

  for (int i = 0; i < NNodes; i++) {
    create (i) IProcess;
    create (i) OProcess;
  }
};
