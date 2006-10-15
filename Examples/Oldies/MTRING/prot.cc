/* ---------------------------------------------------------- */
/* This is a version of the TRING protocol with two observers */
/* ---------------------------------------------------------- */

identify "Token Ring";

int     MinPL, MaxPL, FrmL, HdrL, TokL, TRate;

BIG     TokenPassingTimeout;

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

  states {Start, XmitOwn, XmitOther, OwnDone, TokenDone, OtherDone};

  perform {

      state Start:
	S->Xmit->wait (RECEIVE, XmitOwn);
	S->Pass->wait (RECEIVE, XmitOther);
      state XmitOwn:
	if (Client->getPacket (PBuffer, MinPL, MaxPL, FrmL))
	  OPort->transmit (PBuffer, OwnDone);
	else
	  OPort->transmit (RBuffer, TokenDone);
      state XmitOther:
	  OPort->transmit (RBuffer, OtherDone);
      state OwnDone:
	  OPort->stop ();
	  PBuffer->release ();
	  OPort->transmit (RBuffer, TokenDone);
      state OtherDone:
      transient TokenDone:
	  OPort->stop ();
	  proceed (Start);
  };
};

observer FairnessMonitor {

/* ------------------------------------------------------------- */
/* Verifies local fairness: whether the token is passed from the */
/* immediate successor.                                          */
/* ------------------------------------------------------------- */

  long  LastToken, SId;

  void  setup () { LastToken = NONE; };

  states {Resume, Verify};

  perform {

    state Resume:
      /* --------------------------------------------------------------- */
      /* Issue  an  inspect  request  to be restarted whenever any input */
      /* process receives the token.                                     */
      /* --------------------------------------------------------------- */
      inspect (ANY, IProcess, GotToken, Verify);

    state Verify:
      /* --------------------------------------------------------------- */
      /* Restarted  due  to  an input process receiving the token. Check */
      /* if the token has been passed by the legitimate predecessor.     */
      /* --------------------------------------------------------------- */
      SId = TheStation->getId ();
      assert (LastToken == NONE || (LastToken + 1) % NStations == SId,
						      "Unfair token passing");
      LastToken = SId;
      proceed Resume;
    };
};

observer TokenMonitor {

/* -------------------------------- */
/* Detects duplicated or lost token */
/* -------------------------------- */

  states {Resume, Verify, Duplicate, Lost};

  perform {

    state Resume:
      /* ---------------------------- */
      /* Observe all token receptions */
      /* ---------------------------- */
      inspect (ANY, IProcess, GotToken, Verify);
      /* ------------------------------------------------------------ */
      /* If the token is nowhere received before the timeout,  assume */
      /* that it has been lost.                                       */
      /* ------------------------------------------------------------ */
      timeout (TokenPassingTimeout, Lost);

    state Verify:
      /* ------------------------------------------------------------ */
      /* The current station must pass the token before ....          */
      /* ------------------------------------------------------------ */
      inspect (TheStation, OProcess, TokenDone, Resume);
      /* ------------------------------------------------------------ */
      /* .... any (other) station receives the token.  Otherwise, the */
      /* token is duplicated.                                         */
      /* ------------------------------------------------------------ */
      inspect (ANY, IProcess, GotToken, Duplicate);
      /* ------------------------------------------------------------ */
      /* If the token  is not passed before the timeout,  assume that */
      /* it has been lost.                                            */
      /* ------------------------------------------------------------ */
      timeout (TokenPassingTimeout, Lost);

    state Duplicate:

      excptn ("Duplicate token");

    state Lost:

      excptn ("Lost token");

  };
};

Observer *FMtr, *TMtr;

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
	setLimit (4000);
	genTopology (); genTraffic (); genProtocol ();
	Kernel->wait (DEATH, Stop);
      state Stop:
	System->printTop ("Network topology");
	Client->printDef ("Traffic parameters");
	TPtrn->printPfm ();
	// Deallocate "owned" observers before termination
	FMtr->terminate ();
	TMtr->terminate ();
	terminate;
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

  readIn (TokenPassingTimeout);
};

void Root::genProtocol () {

  for (int i = 0; i < NNodes; i++) {
    create (i) IProcess;
    create (i) OProcess;
  }
  FMtr = create FairnessMonitor;
  TMtr = create TokenMonitor;
};
