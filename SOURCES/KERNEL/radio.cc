/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-18   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

#if	ZZ_NOR

/* ---------- */
/* RF Channel */
/* ---------- */

#include        "system.h"

// Activity stages reflecting event schedule at the destination
#define	RFA_STAGE_BOP	0	// Beginning of preamble
#define	RFA_STAGE_BOT	1	// Beginning of packet
#define	RFA_STAGE_EOT	2	// End of packet
#define	RFA_STAGE_APR	3	// End of aborted preamble
#define	RFA_STAGE_OFF	4	// Packet done

#define	bop_now(a)	(((a)->Schedule == Time && (a)->Stage == RFA_STAGE_BOP)\
				|| ((a)->LastEvent == Time && (a)->Stage ==\
					RFA_STAGE_BOT))

#define	eop_now(a)	((((a)->Schedule == Time) && (((a)->Stage ==\
				RFA_STAGE_APR) || ((a)->Stage ==\
					RFA_STAGE_BOT))) || ((a)->LastEvent ==\
						Time) && ((a)->Stage ==\
							RFA_STAGE_EOT))

#define	bot_now(a)	(((a)->Schedule == Time && (a)->Stage == RFA_STAGE_BOT)\
				|| ((a)->LastEvent == Time && (a)->Stage ==\
					RFA_STAGE_EOT))

#define	eot_now(a)	(((a)->Schedule == Time && (a)->Stage == RFA_STAGE_EOT)\
				|| ((a)->LastEvent == Time && (a)->Stage ==\
					RFA_STAGE_OFF))

static	Process	*rshandle = NULL;	// Roster service process handle

#define	tpckt (*((Packet**)(&Info01)))		// Triggering packet
#define	txcvr (*((Transceiver**)(&Info02)))	// Transceiver in question

static  IPointer    rinfo;          	// Used by wait functions
static  ZZ_RSCHED   *tact;       	// Ditto

// These ones are needed for anotherPacket
static	ZZ_RSCHED 	*apca = NULL;	// Last activity pointer
static	Long		apevn = -2;	// Simulation event number
#if	ZZ_ASR
static	int		appid;		// Port Id
#endif

extern  int  zz_RosterService_prcs;

enum {Start, HandleRosterSchedule, PurgeActivity, TriggerBOT};

class   RosterService : public ZZ_SProcess {

    public:

    Station *S;

    RosterService () {
	S = TheStation;
	zz_typeid = (void*) (&zz_RosterService_prcs);
    };

    virtual const char *getTName () { return ("RosterService"); };

    private:

    public:

    void setup () {

	// Initialize state name list
	zz_sl = new const char* [zz_ns = 4];
	zz_sl [0] = "Start";
	zz_sl [1] = "HandleRosterSchedule";
	zz_sl [2] = "PurgeActivity";
	zz_sl [3] = "TriggerBOT";
    };

    void zz_code ();
};

void    RosterService::zz_code () {

#define	TheACT	((ZZ_RF_ACTIVITY*)Info01)
#define	TheRFC	((RFChannel*)zz_ai)

  switch (TheState) {

      case Start:

	// Void action (go to sleep)

      break; case HandleRosterSchedule:

	TheACT -> handleEvent ();

      break; case PurgeActivity:

	if (TheRFC->PCleaner)
		(*(TheRFC->PCleaner)) (&(TheACT->Pkt));

	TheACT->destruct ();
	delete [] (char*) TheACT;

      break; case TriggerBOT:

	TheACT -> triggerBOT ();
	
  }
#undef	TheACT
#undef	TheRFC
}

inline Boolean ZZ_RSCHED::within_packet () {
/*
 * Checks if the activity is currently within a packet, excluding the
 * very EOT event.
 */
	return (Stage == RFA_STAGE_BOT && Schedule == Time) || 
	       (Stage == RFA_STAGE_EOT && Schedule > Time);
}

inline Boolean ZZ_RSCHED::in_packet () {
/*
 * Checks if the activity is currently within a packet, including the EOT
 * event.
 */
	if (Stage == RFA_STAGE_BOT && Schedule == Time)
		// The very BOT
		return YES;
	if (Stage == RFA_STAGE_EOT && Schedule >= Time)
		// Not past EOT
		return YES;
	if (Stage == RFA_STAGE_OFF && LastEvent == Time)
		return YES;
	return NO;
}

inline void ZZ_RSCHED::initSS () {
/*
 * Initialize signal statistics
 */
	INT.init ();
	Destination->updateIF ();
}

inline void ZZ_RSCHED::initAct () {

	txcvr = Destination;
	tpckt = &(RFA->Pkt);

	// Save the original power
	OXPower = RSS.Level;

	// And transform it into perceived signal level
	RSS.Level = Destination->RFC->RFC_att (&RSS, ituToDu (Distance),
		RFA->Tcv);

	pool_in (this, Destination->Activities);

	Destination->NActivities++;

	if (Destination->RxOn)
		initSS ();
}

void RFChannel::setPacketCleaner (void (*pclr) (Packet *p)) {

	PCleaner = pclr;
}

double IHist::avg (double ts, double du) const {
/*
 * Average starting from ts for du ITUs. If du is negative (or unspecified),
 * we go to the end.
 */
	int i;
	double s, t, u, d, v;
		
	s = 0.0;
	t = 0.0;

	if (ts < 0.0) {
		// du ITUs from the end
		if (du < 0.0)
			return avg ();

		for (i = NEntries - 1; i >= 0; i--) {
			d = (double) (History [i] . Interval);
			v = History [i] . Value;
			u = t + d;
			if (u >= du) {
				u = du;
				d = u - t;
				i = -1;
			}
			s = s * (t / u) + ((v * d) / u);
			t = u;
		}
		return s;
	}

	for (i = 0; i < NEntries; i++) {
		t += (double)(History [i] . Interval);
		if (t > ts) {
			s = History [i] . Value;
			t -= ts;
			break;
		}
	}

	if (du < 0.0) {

		for (i++; i < NEntries; i++) {
			d = (double) (History [i] . Interval);
			v = History [i] . Value;
			u = t + d;
			s = s * (t / u) + ((v * d) / u);
			t = u;
		}

	} else if (t < du) {

		for (i++; i < NEntries; i++) {
			d = (double) (History [i] . Interval);
			v = History [i] . Value;
			u = t + d;
			if (u >= du) {
				u = du;
				d = u - t;
				// To break the loop
				i = NEntries;
			}
			s = s * (t / u) + ((v * d) / u);
			t = u;
		}
	}

	Assert (s >= 0.0, "IHist->avg, returning negative value %g", s);
	return s;
}

double IHist::max (double ts, double du) const {
/*
 * Maximum starting from ts for du ITUs. If du is negative (or unspecified),
 * we go to the end.
 */
	int i;
	double s, t;

	s = 0.0;
	t = 0.0;

	if (ts < 0.0) {
		// du ITUs from the end
		if (du < 0.0)
			return max ();

		for (i = NEntries - 1; i >= 0; i--) {

			if (t >= du)
				break;

			t += (double) (History [i] . Interval);
			if (History [i] . Value > s)
				s = History [i] . Value;
		}

		return s;
	}

	for (i = 0; i < NEntries; i++) {
		t += (double)(History [i] . Interval);
		if (t > ts) {
			s = History [i] . Value;
			t -= ts;
			break;
		}
	}

	if (du < 0.0) {
		for (i++; i < NEntries; i++) {
			if (t >= du)
				break;
			t += (double) (History [i] . Interval);
			if (History [i] . Value > s)
				s = History [i] . Value;
		}
	} else {
		for (i++; i < NEntries && t < du; i++) {
			t += (double) (History [i] . Interval);
			if (History [i] . Value > s)
				s = History [i] . Value;
		}
	}
	return s;
}

double Transceiver::sigLevel () {

	ZZ_RSCHED *a;
	int na;
	SLEntry xm;
	const SLEntry *ac [NActivities];

	if (RxOn == NO)
		// If the receiver is off, we hear no signal
		return 0.0;

	for (na = 0, a = Activities; a != NULL; a = a->next)
		if (!a->Done)
			ac [na++] = &(a->RSS);
	txcvr = this;

	xm.Tag = XmtSig.Tag;
	xm.Level = (Activity != NULL) ? XmtSig.Level : 0.0;

	return RFC->RFC_add (na, NONE, ac, &xm);
}

void Transceiver::reassess () {
/*
 * Called to reassess the signals perceived by the transceiver. To be used
 * after a change of an attribute in the transceiver that has impact on
 * RFC_att.
 */
	ZZ_RSCHED *a;
	SLEntry xm;
	double sl, sn;

	// Previous total signal level
	sl = sigLevel ();
	
	txcvr = this;

	for (a = Activities; a != NULL; a = a->next) {
		tpckt = &(a->RFA->Pkt);
		xm.Level = a->OXPower;
		xm.Tag = a->RSS.Tag;
		a->RSS.Level = RFC->RFC_att (&xm, ituToDu (a->Distance),
			a->RFA->Tcv);
	}
	updateIF ();

	// New total signal level
	sn = sigLevel ();
	if (sn > sl) {
		reschedule_thh (sn);
		reschedule_act (sn);
	} else if (sn < sl) {
		reschedule_thl (sn);
		reschedule_sil (sn);
	}
}

inline int ZZ_RSCHED::any_event () {
/*
 * Checks if the activity triggers any event and returns NONE or
 * event identifier.
 */
	if (Schedule == Time) {
		if (Stage == RFA_STAGE_BOP)
			return BOP;
		if (Stage == RFA_STAGE_APR)
			return EOP;
		tpckt = &(RFA->Pkt);
		if (Stage == RFA_STAGE_BOT)
			return BOT;
		// ENDTRANSFER
		INT.update ();
		return EOT;
	}

	if (LastEvent == Time) {
		if (Stage == RFA_STAGE_BOT)
			return BOP;
		assert (Stage != RFA_STAGE_BOP,
			"ZZ_RSCHED->any_event: bad stage, internal error");
		tpckt = &(RFA->Pkt);
		return BOT;
	}

	return NONE;
}

#ifdef	ZZ_RF_DEBUG

void	ZZ_RF_ACTIVITY::dump () {

	int i;
	ZZ_RSCHED *r;

	trace ("RF ACTIVITY DUMP");
	Ouf << "Tcv: " << Tcv->getOName ();
	Ouf << ", Abt: " << (char)(Aborted ? 'Y' : 'N');
	Ouf << ", Nei: " << NNeighbors << '\n';
	Ouf << "Times: "<< BOTTime << ' ' << EOTTime << '\n';
	Ouf << "EP: " << form ("%x %x", (long) RE, (long) RF) << '\n';
	Ouf << "Pkt: " << Pkt.Sender << ' ' << (Long) (Pkt.Receiver) << ' ' <<
		Pkt.TLength << ' ' << Pkt.Signature << '\n';
	Ouf << "Roster:\n";
	for (i = 0; i < NNeighbors; i++) {
		r = Roster + i;
		Ouf << "D: " << r->Destination->getOName () << " [" << 
			r->Distance << "]\n";
		Ouf << "S: " << r->Schedule << ' ' << r->LastEvent;
		if (r == SchBOP)
			Ouf << " BOP";
		if (r == SchBOT)
			Ouf << " BOT";
		if (r == SchEOT)
			Ouf << " EOT";
		if (r == SchBOTL)
			Ouf << " BOTL";
		if (r == SchEOTL)
			Ouf << " EOTL";
		Ouf << '\n';
		Ouf << "F: " << (r->Killed ? 'K' : 'A') << (r->Done ? 'D' : 'P')
			<< '[' << (int)(r->Stage) << "]\n";
		Ouf << "I:"
			<< ' ' << r->RSS.Level
			<< '\n';
	}
	Ouf << "END\n";
}

void Transceiver::dump (const char *t) {

	ZZ_RSCHED	*a;

	trace ("ACTIVITIES [%s] <%s>:", getOName (), t);

	for (a = Activities; a != NULL; a = a->next) {
		Ouf << form ("  STA: %1d[%1d%1d], ", (int)(a->Stage), 
			a->Killed, a->Done);
		Ouf << "SCH: " << a->Schedule << ", LST: " << a->LastEvent <<
			'\n';
		Ouf << form ("RSI: %f: %f\n", a->RSS.Level);
		Ouf << form ("  PKT: [%d, %d, %d, %d, %d]\n",
			a->RFA->Pkt . Sender,
			a->RFA->Pkt . Receiver,
			a->RFA->Pkt . TLength,
			a->RFA->Pkt . TP,
			a->RFA->Pkt . Signature);
	}
	if (Activity)
		Ouf << "  OWN:\n";

	Ouf << "  SIGLEVEL: " << sigLevel () << '\n';
}

#endif

void    RFChannel::zz_start () {

/* --------------------------- */
/* The nonstandard constructor */
/* --------------------------- */

	// Initialize some members
	FlgSPF = ON;
	PurgeDelay = TIME_1;
	PCleaner = NULL;
	// Add to the Kernel
	pool_in (this, TheProcess->ChList);
};

void RFChannel::setup (Long nx, RATE r, int pre, double XP, double RP,
							  TIME pdel, int spf) {

	static	int asize = 3;
	RFChannel **scratch;
	int i;

	Assert (!zz_flg_started,
		"RFChannel->setup: cannot create new RF channels after the "
			"protocol has started");

	FlgSPF = spf;
	PurgeDelay = pdel;

	assert (nx > 1, "RFChannel: the number of transceivers (%1d) must be at"
		" least 2", nx);

	assert (XP >= 0.0 && RP >= 0.0, "RFChannel: transmit/receive power "
		"(%f, %f) cannot be negative", XP, RP);

	DefXPower = XP;
	DefRPower = RP;
	DefTRate = r;
	DefPreamble = pre;
	DefMinDistance = ituToDu (DISTANCE_1);
	DefXTag = DefRTag = 0;
	DefAevMode = YES;
	DefErrorRun = 1;

	NTransceivers = (int) nx;

	NTAttempts = 0;
	NRBits = NTBits = BITCOUNT_0;
	NRMessages = NTMessages = NRPackets = NTPackets = 0;

	if ((Id = sernum++) == 0) {
		// Create the array of channels
		zz_rf = new RFChannel* [asize];
	} else if (Id >= asize) {
		// Grow the array
		scratch = new RFChannel* [asize];
		for (i = 0; i < asize; i++)
			// Backup copy
			scratch [i] = zz_rf [i];

		delete [] zz_rf; 
		zz_rf = new RFChannel* [asize = (asize+1) * 2 - 1];
		while (i--)
			zz_rf [i] = scratch [i];
		delete [] scratch;
	}

	Class = AIC_rfchannel;

	zz_rf [Id] = this;
	NRFChannels = sernum;

	Tcvs = new Transceiver* [NTransceivers];

	for (i = 0; i < NTransceivers; i++)
		Tcvs [i] = NULL;

	if (rshandle == NULL) {
		// Create the roster service process
		Station	*sts;
		Process	*spr;

		sts = TheStation;
		spr = TheProcess;

		TheStation = System;
		TheProcess = Kernel;

		rshandle = new RosterService;

		((RosterService*) rshandle)->zz_start ();
		((RosterService*) rshandle)->setup ();

		TheStation = sts;
		TheProcess = spr;
	}
}

Transceiver::~Transceiver () {
	excptn ("Transceiver: once created, a transceiver cannot be destroyed");
};

void Transceiver::setup (RATE r, int pre, double XP, double RP,
	double x, 
	double y
#if ZZ_R3D
      , double z
#endif
									) {
	Transceiver *p;
	int i;

	TRate = r;

#if ZZ_R3D
	assert (x >= 0.0 && y >= 0.0 && z >= 0.0, "Transceiver: "
		"%s, location coordinates (%f, %f, %f) must not be negative",
			TheStation->getSName (),
			x, y, z);
#else
	assert (x >= 0.0 && y >= 0.0, "setLocation: %s, location "
		"coordinates (%f, %f) must not be negative",
			TheStation->getSName (),
			x, y);
#endif

	X = duToItu (x);
	Y = duToItu (y);
#if ZZ_R3D
	Z = duToItu (z);
#endif

	Preamble = pre;
	XmtSig.Level = XP;
	// Receiver sensitivity
	RcvSig.Level = RP;
}

Transceiver::Transceiver (RATE r, int pre, double XP, double RP,
	double x,
	double y,
#if ZZ_R3D
	double z,
#endif
								char *nn) {
	Transceiver *p;
	int i;

	Assert (!zz_flg_started,
  "Transceiver: cannot create new transceivers after the protocol has started");

	Class = AIC_transceiver;
	// Moved to zz_start, didn't work here for static declarations
	// Owner = TheStation;
	nextp = NULL;
	RFC = NULL;
	SigThresholdLow = SigThresholdHigh = 0.0;
	TracedActivity = NULL;
	MinDistance = DISTANCE_inf;

	Neighbors = NULL;
	NNeighbors = 0;

	NActivities = 0;

	setup (r, pre, XP, RP, x, y
#if ZZ_R3D
					, z
#endif
						);
	RxOn = YES;
	AevMode = YESNO;
	Mark = NO;
	XmtSig.Tag = RcvSig.Tag = ANY;
	ErrorRun = -1;

	if (nn != NULL) {
		// Nickname assigned
		zz_nickname = new char [strlen (nn) + 1];
		strcpy (zz_nickname, nn);
	}

	if (zz_nctrans == NULL) {
		zz_nctrans = this;
	} else {
		for (p = zz_nctrans; p->nextp != NULL; p = p->nextp);
		p->nextp = this;
	}

	for (i = 0; i < N_TRANSCEIVER_EVENTS; i++)
		RQueue [i] = NULL;

	pool_in (this, TheProcess->ChList);
}

void    Transceiver::zz_start () {
/*
 * For create
 */
	int             srn;
	Transceiver     *p;

	Assert (TheStation != NULL, "Transceiver create: TheStation undefined");

	Assert (zz_nctrans == this,
	      "Transceiver create: internal error -- initialization corrupted");

	// Remove from the temporary list
	zz_nctrans = NULL;
	nextp = NULL;
	Activities = NULL;
	Activity = NULL;
	Owner = TheStation;

	if ((p = TheStation->Transceivers) == NULL) {
		TheStation->Transceivers = this;
		srn = 0;
	} else {
		// Find the end of the station's list
		for (srn = 1, p = TheStation->Transceivers; p -> nextp != NULL;
			p = p -> nextp, srn++);
		p -> nextp = this;
	}

	// Put into Id the station number combined with the port number
	// to be used for display and printing
	Id = MKCID (TheStation->Id, srn);
	RFC = NULL;
}

void	RFChannel::connect (Transceiver *xcv) {

	int i;

	assert (xcv->RFC == NULL, "Transceiver->connect: %s connected twice",
		xcv->getSName ());

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i] == NULL)
			break;

	assert (i < NTransceivers, "Transceiver->connect: %s, too many "
		"transceivers (%1d expected)",
			xcv->getSName (), NTransceivers);
	xcv->RFC = this;
	Tcvs [i] = xcv;

	if (xcv->Preamble < 0)
		xcv->setPreamble (DefPreamble);
	if (xcv->TRate == RATE_inf)
		xcv->setTRate (DefTRate);
	if (xcv->getXPower () < 0.0)
		xcv->setXPower (DefXPower);
	if (xcv->getRPower () < 0.0)
		xcv->setRPower (DefRPower);
	if (xcv->AevMode == YESNO)
		xcv->setAevMode (DefAevMode);
	if (xcv->getMinDistance () < 0.0)
		xcv->setMinDistance (DefMinDistance);
	if (xcv->getRTag () == ANY)
		xcv->setRTag (DefRTag);
	if (xcv->getXTag () == ANY)
		xcv->setXTag (DefXTag);
	if (xcv->ErrorRun < 0)
		xcv->setErrorRun (DefErrorRun);
}

void	Transceiver::connect (Long chn) {

	Assert (isRFChannelId (chn), "Transceiver->connect: %s, %1d is not a "
		"legal link id", getSName (), chn);

	idToRFChannel (chn) -> connect (this);
}

void	RFChannel::complete () {
/*
 * Complete the channel's structure before starting the protocol
 */
	int i;

	Assert (Tcvs [NTransceivers-1] != NULL, "initChannels: some of "
		"the %1d expected Transceivers haven't connected to RFChannel "
			"%s", NTransceivers, getSName ());

	for (i = 0; i < NTransceivers; i++)
		// Build the neighborhoods
		nei_bld (Tcvs [i]);
}

#if ZZ_R3D
void	Transceiver::setLocation (double x, double y, double z) {

	DISTANCE zz;
#else
void	Transceiver::setLocation (double x, double y) {
#endif

	DISTANCE xx, yy;

#if ZZ_R3D
	assert (x >= 0.0 && y >= 0.0 && z >= 0.0, "Transceiver->setLocation: "
		"%s, location coordinates (%f, %f, %f) must not be negative",
			TheStation->getSName (),
			x, y, z);
#else
	assert (x >= 0.0 && y >= 0.0, "Transceiver->setLocation: %s, location "
		"coordinates (%f, %f) must not be negative",
			TheStation->getSName (),
			x, y);
#endif

	xx = duToItu (x);
	yy = duToItu (y);
#if ZZ_R3D
	zz = duToItu (z);
#endif
	if (xx == X && yy == Y
#if ZZ_R3D
				&& zz == Z
#endif
						)
		return;

	X = xx; Y = yy;	
#if ZZ_R3D
	Z = zz;
#endif
	if (zz_flg_started == NO)
		// Just set it, we will do it right when we get to initializing
		// things
		return;

	assert (RFC != NULL,
		"Transceiver->setLocation: %s not interfaced to RFChannel",
			getSName ());

	// Bookkeeping
	RFC->nei_bld (this);
	RFC->nei_cor (this);
}

RATE	Transceiver::setTRate (RATE r) {

	RATE qr;

	if (r == RATE_inf && RFC)
		r = RFC->DefTRate;

	assert (r != RATE_inf,
		"Transceiver->setTRate: %s, illegal (infinite) rate",
			getSName ());
	qr = TRate;
	TRate = r;
	return qr;
}

RF_TAG_TYPE Transceiver::setXTag (RF_TAG_TYPE t) {

	RF_TAG_TYPE old;

	if (t == ANY && RFC)
		t = RFC->DefXTag;

	assert (t != ANY,
		"Transceiver->setXTag: %s, illegal (ANY) XTag value",
			getSName ());

	old = XmtSig.Tag;
	XmtSig.Tag = t;
	return old;
}

RF_TAG_TYPE Transceiver::setRTag (RF_TAG_TYPE t) {

	RF_TAG_TYPE old;

	if (t == ANY && RFC)
		t = RFC->DefRTag;

	assert (t != ANY,
		"Transceiver->setRTag: %s, illegal (ANY) RTag value",
			getSName ());

	old = RcvSig.Tag;
	RcvSig.Tag = t;

	if (zz_flg_started && t != old)
		reassess ();

	return old;
}

RF_TAG_TYPE Transceiver::setTag (RF_TAG_TYPE t) {
	setXTag (t);
	return setRTag (t);
}

Long	Transceiver::setErrorRun (Long e) {

	Long old;

	if (e < 0 && RFC)
		e = RFC->DefErrorRun;

	Assert (e > 0,
	    "Transceiver->setErrorRun: %s, illegal error run length %1d",
		getSName (), e);
	old = ErrorRun;
	ErrorRun = e;
	return old;
}

Long	Transceiver::setPreamble (Long t) {

	Long old;

	if (t < 0 && RFC)
		t = RFC->DefPreamble;

	assert (t >= 0,
		"Transceiver->setPreamble: %s, illegal (undefined) "
		"preamble length", getSName ());

	old = Preamble;
	Preamble = t;
	return old;
}

double	Transceiver::setXPower (double p) {

	double op;

	if (p < 0.0 && RFC)
		p = RFC->DefXPower;

	assert (p >= 0.0,
		"Transceiver->setXPower: %s, llegal (undefined) power (%f)",
			getSName (), p);

	op = XmtSig.Level;
	XmtSig.Level = p;

	if (zz_flg_started) {

		if (XmtSig.Level > op)
			RFC->nei_xtd (this);
		else if (XmtSig.Level < op)
			RFC->nei_trm (this);
	}

	return op;
}

double	Transceiver::setRPower (double p) {

	double op;

	if (p < 0.0 && RFC)
		p = RFC->DefRPower;

	assert (p >= 0.0,
		"Transceiver->setRPower: %s, illegal (undefined) power (%1f)",
			getSName (), p);

	op = RcvSig.Level;
	RcvSig.Level = p;

	if (zz_flg_started) {
		if (RcvSig.Level > op)
			RFC->nei_add (this);
		else if (RcvSig.Level < op)
			RFC->nei_del (this);
		reassess ();
	}

	return op;
}

void RFChannel::setTRate (RATE r) {

/* ================================ */
/* Set the rate of all transceivers */
/* ================================ */

	Long i;

	if (r == RATE_inf)
		r = DefTRate;
	else
		DefTRate = r;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			// Can be called at initialization
			Tcvs [i] -> TRate = r;
}

void RFChannel::setPreamble (Long t) {

/* ============================================ */
/* Set the preamble length for all transceivers */
/* ============================================ */

	Long i;

	if (t < 0)
		t = DefPreamble;
	else
		DefPreamble = t;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			// Can be called before the network has been initialized
			Tcvs [i] -> setPreamble (t);
}

void RFChannel::setXTag (RF_TAG_TYPE t) {

/* ================================== */
/* Set the X Tags of all transceivers */
/* ================================== */

	Long i;

	DefXTag = t;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> setXTag (t);
}

void RFChannel::setRTag (RF_TAG_TYPE t) {

/* ================================== */
/* Set the R Tags of all transceivers */
/* ================================== */

	Long i;

	DefRTag = t;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> setRTag (t);
}

void RFChannel::setTag (RF_TAG_TYPE t) {

/* ================================ */
/* Set the Tags of all transceivers */
/* ================================ */

	setXTag (t);
	setRTag (t);
}

void RFChannel::setTag () {

	assert (DefRTag == DefXTag,
	    "RFChannel->setTag: %s, DefXTag != DefRTag", getSName ());

	setTag (DefXTag);
}

void RFChannel::setErrorRun (Long e) {

	Long i;

	if (e < 0)
		e = DefErrorRun;
	else
		DefErrorRun = e;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> setErrorRun (e);
}

void RFChannel::setXPower (double p) {

	Long i;

	if (p < 0.0)
		p = DefXPower;
	else
		DefXPower = p;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> XmtSig.Level = p;

	if (zz_flg_started) {
		// Redo all neighborhoods
		for (i = 0; i < NTransceivers; i++)
			// Build the neighborhoods
			nei_bld (Tcvs [i]);
	}
}

void RFChannel::setRPower (double p) {

	Long i;

	if (p < 0.0)
		p = DefRPower;
	else
		DefRPower = p;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> RcvSig.Level = p;

	if (zz_flg_started) {
		// Redo all neighborhoods
		for (i = 0; i < NTransceivers; i++)
			// Build the neighborhoods
			nei_bld (Tcvs [i]);
	}
}

void RFChannel::setMinDistance (double d) {

	int i;

	assert (d >= 0.0,
	    "RFChannel->setMinDistance: %s, distance (%f) cannot be less "
		"than 0.0", getSName (), d);

	if (d < 0.0)
		d = DefMinDistance;
	else
		DefMinDistance = d;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> setMinDistance (d);
}

void RFChannel::setAevMode (Boolean b) {

	int i;

	if (b != YES && b != NO)
		b = DefAevMode;
	else
		DefAevMode = b;

	for (i = 0; i < NTransceivers; i++)
		if (Tcvs [i])
			Tcvs [i] -> setAevMode (b);
}

Long RFChannel::errors (RATE r, const SLEntry *sl, const SLEntry *rs,
	const IHist *ih) {

/* ========================================================================== */
/* Returns the number of bit errors in the histogram. Assumes that RFC_erb is */
/* defined (it isn't mandatory).                                              */
/* ========================================================================== */

	int i;
	double ir;
	Long nb, ne;

	ne = 0;
	for (i = 0; i < ih->NEntries; i++) {
		ih->entry (i, r, nb, ir);
		ne += RFC_erb (r, sl, rs, ir, nb);
	}

	return ne;
}

Long RFChannel::errors (RATE r, const SLEntry *sl, const SLEntry *rs,
	const IHist *h, Long sb, Long nb) {
/*
 * Returns the number of bit errors in the histogram starting at the indicated
 * bit position and extending for the specified number of bits.
 */
	double ra, rb, ts, ta, ci, lb, ft, fr;
	Long ne, iv;
	int i;

	ra = (double) r;		// Rate in double
	ne = 0;				// Accumulated number of error bits
	// double limit on the number of bits
	lb = (nb >= 0) ? (double) nb : HUGE;

	if (sb < 0) {
		// This means backwards: nb == the number of bits at the end
		if (nb < 0)
			// Fall back to the default
			return errors (r, sl, rs, h);
		for (i = h->NEntries - 1; i >= 0; i--) {
			ta = (double) (h->History [i] . Interval);
			ci = h->History [i] . Value;
			rb = ta / ra;
			if (rb >= lb) {
				// The last segment
				rb = lb;
				i = 0;
			}
			ft = trunc (rb);	// Integral number of bits
			fr = rb - ft;		// The fraction
			iv = (Long) ft;		// Integer number of bits
			if (fr > 0.0 && rnd (SEED_toss) < fr)
				iv ++;		// Adjust probabilistically
			if (iv)			// Calculate errors
				ne += RFC_erb (r, sl, rs, ci, iv);
		}
		return ne;
	}

	ts = ra * (double) sb;		// Starting time in double
	ta = 0.0;			// Accumulated time
	
	for (i = 0; i < h->NEntries; i++) {
		// Locate the first entry of interest
		ta += (double) (h->History [i] . Interval);
		if (ta > ts) {
			ta -= ts;	// Effective duration of this entry
			ci = h->History [i] . Value;
			break;
		}
	}

	if (i == h->NEntries)
		// Starting bit behind the activity
		return 0;

	do {
		rb = ta / ra;			// bit count
		if (rb >= lb) {	
			// This is the last segment
			rb = lb;
			// Force exit
			i = h->NEntries;
		} else {
			lb -= rb;
		}

		ft = trunc (rb);	// Integral number of bits
		fr = rb - ft;		// The fraction
		iv = (Long) ft;		// Integer number of bits
		if (fr > 0.0 && rnd (SEED_toss) < fr)
			iv ++;		// Adjust probabilistically
		if (iv)			// Calculate errors
			ne += RFC_erb (r, sl, rs, ci, iv);

		if (++i >= h->NEntries)
			return ne;

		// New entry
		ta = (double) (h->History [i] . Interval);
		ci = h->History [i] . Value;
	} while (1);
}

Boolean RFChannel::error (RATE r, const SLEntry *sl, const SLEntry *rs,
	const IHist *ih) {

/* ================================================ */
/* Returns YES if the historam contains a bit error */
/* ================================================ */

	int i;
	double ir;
	Long nb;

	for (i = 0; i < ih->NEntries; i++) {
		ih->entry (i, r, nb, ir);
		if (RFC_erb (r, sl, rs, ir, nb))
			return YES;
	}

	return NO;
}

Boolean RFChannel::error (RATE r, const SLEntry *sl, const SLEntry *rs,
	const IHist *h, Long sb, Long nb) {
/*
 * Returns YES if the histogram portion starting at the indicated bit position
 * and extending for the specified number of bits contains at least one bit
 * error.
 */
	double ra, rb, ts, ta, ci, lb, ft, fr;
	Long iv;
	int i;

	ra = (double) r;		// Rate in double
	// double limit on the number of bits
	lb = (nb >= 0) ? (double) nb : HUGE;

	if (sb < 0) {
		// This means backwards: nb == the number of bits at the end
		if (nb < 0)
			// Fall back to the default
			return error (r, sl, rs, h);
		for (i = h->NEntries - 1; i >= 0; i--) {
			ta = (double) (h->History [i] . Interval);
			ci = h->History [i] . Value;
			rb = ta / ra;
			if (rb >= lb) {
				// The last segment
				rb = lb;
				i = 0;
			}
			ft = trunc (rb);	// Integral number of bits
			fr = rb - ft;		// The fraction
			iv = (Long) ft;		// Integer number of bits
			if (fr > 0.0 && rnd (SEED_toss) < fr)
				iv ++;		// Adjust probabilistically
			if (iv)	{
				if (RFC_erb (r, sl, rs, ci, iv))
					return YES;
			}
		}
		return NO;
	}

	ts = ra * (double) sb;		// Starting time in double
	ta = 0.0;			// Accumulated time
	
	for (i = 0; i < h->NEntries; i++) {
		// Locate the first entry of interest
		ta += (double) (h->History [i] . Interval);
		if (ta > ts) {
			ta -= ts;	// Effective duration of this entry
			ci = h->History [i] . Value;
			break;
		}
	}

	if (i == h->NEntries)
		// Starting bit behind the activity
		return NO;

	do {
		rb = ta / ra;			// bit count
		if (rb >= lb) {	
			// This is the last segment
			rb = lb;
			// Force exit
			i = h->NEntries;
		} else {
			lb -= rb;
		}

		ft = trunc (rb);	// Integral number of bits
		fr = rb - ft;		// The fraction
		iv = (Long) ft;		// Integer number of bits
		if (fr > 0.0 && rnd (SEED_toss) < fr)
			iv ++;		// Adjust probabilistically
		if (iv)	{
			if (RFC_erb (r, sl, rs, ci, iv))
				return YES;
		}

		if (++i >= h->NEntries)
			return NO;

		// New entry
		ta = (double) (h->History [i] . Interval);
		ci = h->History [i] . Value;
	} while (1);
}

static ZZ_NEIGHBOR *nei_sort;

static void sort_nei (int lo, int up) {

	int		i, j;
	ZZ_NEIGHBOR	s;
	DISTANCE	t;

	while (up > lo) {

		s = nei_sort [i = lo];
		t = s . Distance;
		j = up;

		while (i < j) {
			while (nei_sort [j] . Distance > t)
				j--;
			nei_sort [i] = nei_sort [j];
			while ((i < j) && (nei_sort [i] . Distance <= t))
				i++;
			nei_sort [j] = nei_sort [i];
		}

		nei_sort [i] = s;

		sort_nei (lo, i-1);
		lo = i + 1;
	}
}

#if ZZ_R3D
double RFChannel::getRange (double &lx, double &ly, double &lz,
			    double &hx, double &hy, double &hz    ) {

	DISTANCE LX, LY, LZ, HX, HY, HZ;
#else
double RFChannel::getRange (double &lx, double &ly, double &hx, double &hy) {

	DISTANCE LX, LY, HX, HY;
#endif
	Transceiver *t;
	int i;

	assert (NTransceivers > 0, "RFChannel->getRange: %s, no transceivers"
		" interfaced to the channel", getSName ());

	LX = LY = DISTANCE_inf;
	HX = HY = DISTANCE_0;

#if ZZ_R3D
	LZ = DISTANCE_inf;
	HZ = DISTANCE_0;
#endif

	for (i = 0; i < NTransceivers; i++) {
		t = Tcvs [i];
		if (t->X < LX)
			LX = t->X;
		if (t->X > HX)
			HX = t->X;
		if (t->Y < LY)
			LY = t->Y;
		if (t->Y > HY)
			HY = t->Y;
#if ZZ_R3D
		if (t->Z < LZ)
			LZ = t->Z;
		if (t->Z > HZ)
			HZ = t->Z;
#endif
	}

	lx = ituToDu (LX);
	hx = ituToDu (HX);
	ly = ituToDu (LY);
	hy = ituToDu (HY);

#if ZZ_R3D
	lz = ituToDu (LZ);
	hz = ituToDu (HZ);
#endif
	return sqrt ( 	  (hx - lx) * (hx - lx)
			+ (hy - ly) * (hy - ly)
#if ZZ_R3D
			+ (hz - lz) * (hz - lz)
#endif
						);
}

inline double RFChannel::rfc_cut (const Transceiver *f, const Transceiver *t) {
	return RFC_cut (f->XmtSig.Level, t->RcvSig.Level);
}

void RFChannel::nei_bld (Transceiver *T) {

/* ======================== */
/* Rebuild the neighborhood */
/* ======================== */

	Transceiver *t;
	double d;
	int i, n;

	if (T->Neighbors != NULL)
		delete [] T->Neighbors;

	nei_sort = new ZZ_NEIGHBOR [NTransceivers-1];

	for (n = i = 0; i < NTransceivers; i++) {
		if ((t = Tcvs [i]) == T)
			// Ignore yourself
			continue;
		// Distance to the node
		d = T->qdst (t);
		if (d > rfc_cut (T, t))
			continue;
		// Yep, this one qualifies
		nei_sort [n] . Neighbor = t;
		nei_sort [n] . Distance = duToItu (d);
		n++;
	}

	if ((T->NNeighbors = n) == 0) {
		T->Neighbors = NULL;
	} else {
		sort_nei (0, n-1);
		T->Neighbors = new ZZ_NEIGHBOR [n];
		memcpy (T->Neighbors, nei_sort, sizeof (ZZ_NEIGHBOR) * n);
	}

#if 0
	print (form ("Neighborhood of %s\n", T->getSName ()));
	for (i = 0; i < T->NNeighbors; i++)
		print (form (" %f  [%s]\n",
			ituToDu (T->Neighbors [i] . Distance),
				T->Neighbors [i] . Neighbor -> getSName ()));
#endif

	delete [] nei_sort;
}

void RFChannel::nei_xtd (Transceiver *T) {

/* ==================================================== */
/* Extend our own neighborhood (distances don't change) */
/* ==================================================== */

	int i, nw;
	double d;
	Transceiver *t;
	ZZ_NEIGHBOR *nei;

	nei = new ZZ_NEIGHBOR [NTransceivers - 1];

	for (i = 0; i < T->NNeighbors; i++)
		// Mark those that are already present on the list
		T->Neighbors [i] . Neighbor -> Mark = YES;

	for (nw = i = 0; i < NTransceivers; i++) {
		if ((t = Tcvs [i]) == T || t -> Mark)
			// Ignore self and those already present
			continue;
		d = T->qdst (t);
		if (d > rfc_cut (T, t))
			continue;
		nei [nw] . Neighbor = t;
		nei [nw] . Distance = duToItu (d);
		nw++;
	}

	for (i = 0; i < T->NNeighbors; i++)
		// Remove marks from ports
		T->Neighbors [i] . Neighbor -> Mark = NO;

	if (nw == 0) {
		// No extension
		delete [] nei;
		return;
	}

	// Extend
	nei_sort = new ZZ_NEIGHBOR [T->NNeighbors + nw];
	if (T->NNeighbors) {
		memcpy (nei_sort, T->Neighbors,
			sizeof (ZZ_NEIGHBOR) * T->NNeighbors);
		delete [] T->Neighbors;
	}

	memcpy (nei_sort + T->NNeighbors, nei, sizeof (ZZ_NEIGHBOR) * nw);

	delete [] nei;

	sort_nei (0, (T->NNeighbors += nw) - 1);
	T->Neighbors = nei_sort;
}

void RFChannel::nei_trm (Transceiver *T) {

/* ================================================== */
/* Trim our own neighborhood (distances don't change) */
/* ================================================== */

	Transceiver *t;
	ZZ_NEIGHBOR *ne, *nf;
	int i, j;

	for (i = 0, ne = T->Neighbors; i < T->NNeighbors; i++, ne++) {
		if (ne->Distance > duToItu (rfc_cut (T, ne->Neighbor)))
			// Start trimming
			break;
	}
	if (i >= T->NNeighbors)
		// Nothing to do
		return;

	for (j = i + 1, nf = ne + 1; j < T->NNeighbors; j++, nf++) {
		if (nf->Distance > duToItu (rfc_cut (T, nf->Neighbor)))
			continue;
		*ne++ = *nf;
		i++;
	}

	if (i == 0) {
		delete [] T->Neighbors;
		T->Neighbors = NULL;
	}

	// We do not resize the array at this stage
	T->NNeighbors = i;
}

void RFChannel::nei_add (Transceiver *T) {

/* ============================================================= */
/* Check if the transceiver should be added to any neighbor list */
/* (distances haven't changed)                                   */
/* ============================================================= */

	int i, j;
	double d;
	DISTANCE D;
	Transceiver *t;
	ZZ_NEIGHBOR *ne;

	for (i = 0; i < NTransceivers; i++) {
		if ((t = Tcvs [i]) == T)
			// Don't add yourself to your own list
			continue;
		d = t->qdst (T);
		if (d > rfc_cut (t, T))
			continue;
		D = duToItu (d);
		for (j = 0; j < t->NNeighbors; j++) {
			ne = t->Neighbors + j;
			if (ne->Distance < D)
				continue;
			if (ne->Neighbor == T)
				// Present already
				goto Next;
			if (ne->Distance > D)
				// Must be added right here
				break;
		}
		nei_sort = new ZZ_NEIGHBOR [t->NNeighbors + 1];
		if (j)
			memcpy (nei_sort, t->Neighbors, sizeof (ZZ_NEIGHBOR)*j);
		nei_sort [j] . Neighbor = T;
		nei_sort [j] . Distance = D;
		if (j < t->NNeighbors)
			memcpy (nei_sort + j + 1, t->Neighbors + j,
				sizeof (ZZ_NEIGHBOR) * (t->NNeighbors - j));
		t->NNeighbors++;
		if (t->Neighbors != NULL)
			delete [] t->Neighbors;
		t->Neighbors = nei_sort;
Next:
		NOP;
	}
}

void RFChannel::nei_del (Transceiver *T) {

/* ================================================================= */
/* Check if the transceiver should be removed from any neighbor list */
/* (distances haven't changed)                                       */
/* ================================================================= */

	int i, j;
	Transceiver *t;

	for (i = 0; i < NTransceivers; i++) {
		if ((t = Tcvs [i]) == T)
			continue;
		for (j = 0; j < t->NNeighbors; j++) {
			if (t->Neighbors [j] . Neighbor == T) {
				if (t->Neighbors [j] . Distance >
				    duToItu (rfc_cut (t, T))) {
					// Remove - but the array doesn't shrink
					while (++j < t->NNeighbors)
						t->Neighbors [j-1] =
							t->Neighbors [j];
					if (--(t->NNeighbors) == 0) {
						delete [] t->Neighbors;
						t->Neighbors = NULL;
					}
				}
				goto Next;
			}
		}
Next:
		NOP;
	}
}

void RFChannel::nei_cor (Transceiver *T) {

/* =========================================== */
/* Correct for this Transceiver after its move */
/* =========================================== */

	int i, j, k;
	Transceiver *t;
	double d;
	DISTANCE D;
	ZZ_NEIGHBOR ne;

	for (i = 0; i < NTransceivers; i++) {
		if ((t = Tcvs [i]) == T)
			continue;
		d = t->qdst (T);
		if (d > rfc_cut (t, T)) {
			// Remove if present
			for (j = 0; j < t->NNeighbors; j++) {
				if (t->Neighbors [j] . Neighbor == T) {
					// Remove (the array doesn't shrink, but
					// we remove it if the list of neighbors
					// ends up empty)
					while (++j < t->NNeighbors)
						t->Neighbors [j-1] =
							t->Neighbors [j];
					if (--(t->NNeighbors) == 0) {
						delete [] t->Neighbors;
						t->Neighbors = NULL;
					}
					goto Next;
				}
			}
			// Not found, good!
		} else {
			// Fix or add
			D = duToItu (d);
			for (j = 0; j < t->NNeighbors; j++) {
				if (t->Neighbors [j] . Neighbor == T) {
					if (j == t->NNeighbors - 1 ||
					  D <= t->Neighbors [j+1] . Distance) {
						// Stays where it was
						t->Neighbors [j] . Distance = D;
						goto Next;
					}
					// Delete it and insert further, the
					// array won't have to be resized
					while (++j < t->NNeighbors) {
					  if (t->Neighbors [j] . Distance >= 
					   D) {
					    // Insert the new one
					    t->Neighbors [j-1].Neighbor = T;
					    t->Neighbors [j-1].Distance = D;
					    // And we are done
					    goto Next;
					  }
					  t->Neighbors [j-1] = t->Neighbors [j];
					}
					// Add it at the very end
				        t->Neighbors [j-1].Neighbor = T;
				        t->Neighbors [j-1].Distance = D;
					goto Next;
				}
				// Find the slot to put it in
				if (t->Neighbors [j] . Distance >= D) {
					// Here it goes
					for (k = j+1; k < t->NNeighbors; k++) {
					  if (t->Neighbors [k].Neighbor == T) {
					    // Found it, no need to resize the
					    // array
					    while (k-- > j)
					      t->Neighbors [k+1] =
						t->Neighbors [k];
					    t->Neighbors [j].Neighbor = T;
					    t->Neighbors [j].Distance = D;
					    goto Next;
					  }
					}
					// Not found, resize
					nei_sort =
					  new ZZ_NEIGHBOR [t->NNeighbors + 1];
					if (j)
					  memcpy (nei_sort, t->Neighbors,
					    sizeof (ZZ_NEIGHBOR) * j);
					nei_sort [j] . Neighbor = T;
					nei_sort [j] . Distance = D;
					while (j++ < t->NNeighbors)
					  // The trailer
					  nei_sort [j] = t->Neighbors [j-1];
					if (t->Neighbors != NULL)
						delete [] t->Neighbors;
					t->Neighbors = nei_sort;
					t->NNeighbors++;
					goto Next;
				}
			}
			// At the end + resize
			nei_sort = new ZZ_NEIGHBOR [t->NNeighbors + 1];
			if (t->NNeighbors) {
				memcpy (nei_sort, t->Neighbors,
					sizeof (ZZ_NEIGHBOR) * t->NNeighbors);
				delete [] t->Neighbors;
			}
			nei_sort [t->NNeighbors  ] . Neighbor = T;
			nei_sort [t->NNeighbors++] . Distance = D;
			t->Neighbors = nei_sort;
		}
Next:
		NOP;
	}
}

void Transceiver::handle_ifv () {
/*
 * Handle interference threshold events
 */
	TIME		t;
	ZZ_REQUEST	*rq, *rm;
	ZZ_EVENT	*ev;
	double		sigl;
	int		qt;
#if ZZ_TAG
	int		q;
#endif
	assert (TracedActivity != NULL,
		"Transceiver->handle_ifv: %s, no traced activity",
			getSName ());

	if (RQueue [BERROR]) {

		// Process(es) waiting for BERROR

		t = Time + (TIME) TRate * RFC->RFC_erd (TRate,
			&(TracedActivity->RSS), &RcvSig,
				TracedActivity->INT.cur (), ErrorRun);

		for (rq = RQueue [BERROR]; rq != NULL; rq = rq->next) {
			ev = rq->event;
			if (rq->when < t) {
				// Postpone
#if	ZZ_TAG
				rq->when . set (t);
#else
				rq->when = t;
#endif
				if (ev->chain == rq) {
					// Find new minimum
					rm = rq->min_request ();
					ev->new_top_request (rm);
				}
			} else {
#if	ZZ_TAG
				rq->when . set (t);
				if (((q = ev->waketime.cmp (rq->when)) < 0) ||
				    ((q == 0) && FLIP))
					continue;
#else
				rq->when = t;
				if ((ev->waketime < t) ||
				    ((ev -> waketime == t) && FLIP))
					continue;
#endif
				ev->new_top_request (rq);
			}
		}
	}

	if ((sigl = TracedActivity->INT.cur ()) <= SigThresholdLow) {

		for (rq = RQueue [INTLOW]; rq != NULL; rq = rq->next) {
			if (rq->when == Time && FLIP)
				// Already scheduled on another occasion
				continue;
			rq->Info01 = &(TracedActivity->RFA->Pkt);
#if     ZZ_TAG
			rq->when . set (Time);
			if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
				continue;
#else
			rq->when = Time;
			if ((ev = rq->event) -> waketime <= Time && FLIP)
				continue;
#endif
			ev->new_top_request (rq);
		}
	}

	if (sigl > SigThresholdHigh) {

		for (rq = RQueue [INTHIGH]; rq != NULL; rq = rq->next) {
			if (rq->when == Time && FLIP)
				continue;
			rq->Info01 = &(TracedActivity->RFA->Pkt);
#if     ZZ_TAG
			rq->when . set (Time);
			if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
				continue;
#else
			rq->when = Time;
			if ((ev = rq->event) -> waketime <= Time && FLIP)
				continue;
#endif
			ev->new_top_request (rq);
		}
	}
}

void Transceiver::rcvOn () {
/*
 * Switch on the receiver. Start collecting signal statistics.
 */
	ZZ_RSCHED	*a;
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;
	double		sigl;

	if (RxOn)
		// Already on
		return;
	RxOn = YES;
	for (a = Activities; a != NULL; a = a->next)
		a->initSS ();
	updateIF ();
	reschedule_thh (sigl = sigLevel ());

	// Process events and packet, i.e., bring the state of the port
	// to sanity

	for (a = Activities; a != NULL; a = a -> next) {
		if (a->in_packet ())
			// Kill them
			a->Killed = YES;
		// Note: we don't trigger ANYEVENT, even if some events are
		// scheduled to occur. This will be taken care of by
		// handleEvent. Note that we are allowed to miss ANYEVENT
		// falling into the same ITU as rcvOn with probability 1/2.
	}
	// Activity
	reschedule_act (sigl);
}

void Transceiver::rcvOff () {

	RxOn = NO;
	reschedule_thl (0.0);
	reschedule_sil (0.0);
}

void Transceiver::startTransmit (Packet *p) {

	int i;
	ZZ_RSCHED *ro;
	ZZ_NEIGHBOR *ne;
	double sigl;

	if_from_observer ("Transceiver->startTransmit: called from an "
			  "observer");

	assert (Activity == NULL,
		"Transceiver->startTransmit: %s, multiple transmissions",
			getSName ());

	assert (p->TLength > 0,
		"Transceiver->startTransmit: %s, illegal packet length %1d",
			getSName (), p->TLength);

	if (RFC->FlgSPF == ON)
		RFC->NTAttempts++;

	// ====================================================================

	Activity = (ZZ_RF_ACTIVITY*) new char [sizeof (ZZ_RF_ACTIVITY) -
		sizeof (Packet) +
			p->frameSize ()];
	Activity->Pkt = *p;

	Assert (TRate != RATE_0 && TRate != RATE_inf,
		"Transceiver->startTransmit: %s, TRate undefined",
			getSName ());
	Assert (Preamble >= 0, "Transceiver->startTransmit: %s, preamble "
		"length is undefined", getSName ());

	Activity->TRate = (Activity->Tcv = this)->TRate;

	Activity->BOTTime = Activity->EOTTime = TIME_inf;  // EOT not known yet
	Activity->Aborted = NO;
	/*
	 * Create the schedule roster
	 */
	if ((Activity->NNeighbors = NNeighbors) == 0) {
		// Note that the set of neighbors can be empty. In such a case,
		// the roster is NULL. The activity is formally created
		// nonetheless, so things are always consistent from the
		// transmitter's point of view.
		Activity->RE = Activity->RF = NULL;
		Activity->Roster = NULL;

	} else {

		Activity->Roster = new ZZ_RSCHED [NNeighbors];
		// Start from the last neighbor, so the list is sorted in the
		// increasing order of distance (it is constructed in reverse
		// order)
		Activity->SchBOP = Activity->SchBOT = Activity->SchEOT = NULL;
		for (i = NNeighbors; i > 0; ) {
			i--;
			ne = Neighbors + i;
			ro = Activity->Roster + i;
			ro->RFA = Activity;
			ro->INT.start ();
			ro->Destination = ne->Neighbor;
			if ((ro->Distance = ne->Distance) < MinDistance)
				ro->Distance = MinDistance;
			ro->Schedule = Time + ne->Distance;
			ro->LastEvent = TIME_inf;
			ro->Killed = ro->Done = NO;
			ro->Stage = RFA_STAGE_BOP;
			ro->Next = Activity->SchBOP;
			ro->RSS.Tag = XmtSig.Tag;
			// This will be adjusted by initAct to represent the
			// perceived signal level at the receiver
			ro->RSS.Level = XmtSig.Level;
			Activity->SchBOP = ro;
		}
		// Schedule the initial event to advance the roster
		Activity->RE = new ZZ_EVENT (
			Activity->SchBOP->Schedule,
			System,			// Station
			(void*)Activity,	// Info01
			NULL,			// Info02
			rshandle,		// Process
			this->RFC,		// AI - the channel
			DO_ROSTER,		// Event
			HandleRosterSchedule,	// State
			NULL			// No request chain
		);
		// Schedule the end of preamble event to start the packet
		Activity->RF = new ZZ_EVENT (
			Time + getPreambleTime (),
			System,			// Station
			(void*)Activity,	// Info01
			NULL,			// Info02
			rshandle,		// Process
			this->RFC,		// AI
			BOT_TRIGGER,		// Event
			TriggerBOT,		// State
			NULL			// No request chain
		);

	}

	// ====================================================================

	// Include transmitter's interference
	if (RxOn) {
		updateIF ();
		reschedule_thh (sigl = sigLevel ());
		reschedule_act (sigl);
	}

	tpckt = &(Activity -> Pkt);      		// ThePacket (Info01)
	txcvr = this;
}

void Transceiver::term_xfer (int evnt) {

	ZZ_RSCHED *ro;
	double sigl;

	if_from_observer ("Tranceiver->stop/abort: called from an observer");

	assert (Activity != NULL, "Transceiver->stop/abort: %s, no activity "
		"to stop", getSName ());

	assert (Activity->EOTTime == TIME_inf, "Transceiver->stop/abort: "
		"activity already stopped, internal error");

	tpckt = &(Activity -> Pkt);       		// ThePacket
	Info02 = (void*) (IPointer) (tpckt->TP);        // TheTraffic

	if (evnt == EOT) {
		RFC->spfmPTR (tpckt);
		if (tpckt->isStandard () && tpckt->isLast ())
			RFC->spfmMTR (tpckt);
	}

	if (Activity->Roster == NULL) {
		// There was no roster; the event is of no consequence.
		// Schedule event to remove the activity; this will give the
		// caller a chance to perceive Info01, if he really wants to.
		Activity->RE = new ZZ_EVENT (
			Time,
			System,			// Station
			(void*)Activity,	// Info01
			NULL,			// Info02
			rshandle,		// Process
			this->RFC,		// AI
			RACT_PURGE,		// Event
			PurgeActivity,		// State
			NULL			// No request chain
		);
	} else {
		// Check if we are terminating the preamble (if so, it must
		// be abort)
		if (Activity->RF != NULL) {
			// This will be reset to NULL when the preamble has
			// ended
			Activity->Aborted = YES;	// Preamble only
			Activity->RF->cancel ();
			delete (Activity->RF);
			Activity->RF = NULL;
			// handleEvent will know what to do
			Activity->triggerBOT ();
		} else {
			// Trigger EOT
			Activity->EOTTime = Time;
			if (Activity->SchEOT != NULL) {
				// Schedules for these become definite
				for (ro = Activity->SchEOT; ro != NULL;
				    ro = ro->Next) {
					ro->Schedule = ro->Distance + Time;
					if (evnt != EOT)
						// This is an abort
						ro->Killed = YES;
				}
				// Check if should reschedule the service event
#if ZZ_TAG
				if (Activity->RE->waketime.cmp (
				  Activity->SchEOT->Schedule) > 0) {
					Activity->RE->waketime.set (
						Activity->SchEOT->Schedule);
#else
				if (Activity->RE->waketime >
				    Activity->SchEOT->Schedule) {
					Activity->RE->waketime =
						Activity->SchEOT->Schedule;
#endif
					Activity->RE->reschedule ();
				}
			}
		}
	}

	Activity = NULL;
	// At the current Transceiver ...
	if (RxOn) {
		updateIF ();
		reschedule_thl (sigl = sigLevel ());
		reschedule_sil (sigl);
	}
}

void Transceiver::updateIF () {

	int na;
	ZZ_RSCHED *a;
	SLEntry xm;
	const SLEntry *ac [NActivities];

	if (RxOn == NO)
		// Do nothing if the receiver is switched off
		return;

	txcvr = this;

	for (na = 0, a = Activities; a != NULL; a = a->next) {
		if (a->Done)
			continue;
		Assert (na < NActivities, "Transceiver->updateIF: "
			"wrong number of activities - internal error");
		ac [na++] = &(a->RSS);
	}

	xm.Tag = XmtSig.Tag;
	xm.Level = (Activity != NULL) ? XmtSig.Level : 0.0;

	for (na = 0, a = Activities; a != NULL; a = a->next) {
		if (a->Done)
			continue;
		a->INT.update (RFC->RFC_add (NActivities, na, ac, &xm));
		if (a->Destination->TracedActivity == a)
			handle_ifv ();
		na++;
	}
}

void Transceiver::reschedule_thh (double sigl) {
/*
 * Handles signal level going up
 */
	ZZ_REQUEST	*rq;
	ZZ_EVENT	*ev;

	if (sigl > SigThresholdHigh) {
		for (rq = RQueue [SIGHIGH]; rq != NULL; rq = rq->next) {
#if     ZZ_TAG
			rq->when . set (Time);
			if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 &&
			    FLIP)
				continue;
#else
			rq->when = Time;
			if ((ev = rq->event) -> waketime <= Time && FLIP)
				continue;
#endif
			ev->new_top_request (rq);
		}
	}
}

void Transceiver::reschedule_thl (double sigl) {
/*
 * Handles signal level going down
 */
	ZZ_REQUEST	*rq;
	ZZ_EVENT	*ev;

	if (sigl <= SigThresholdLow) {
		for (rq = RQueue [SIGLOW]; rq != NULL; rq = rq->next) {
#if     ZZ_TAG
			rq->when . set (Time);
			if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 &&
			    FLIP)
				continue;
#else
			rq->when = Time;
			if ((ev = rq->event) -> waketime <= Time && FLIP)
				continue;
#endif
			ev->new_top_request (rq);
		}
	}
}

void Transceiver::reschedule_bop () {
/*
 * Handles the beginning of a preamble. Assumes that Rcv is On; we won't call
 * it otherwise.
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	for (rq = RQueue [BOP]; rq != NULL; rq = rq->next) {
#if     ZZ_TAG
		rq->when . set (Time);
		if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
			continue;
#else
		rq->when = Time;
		if ((ev = rq->event) -> waketime <= Time && FLIP)
			continue;
#endif
		ev->new_top_request (rq);
	}
	reschedule_act (sigLevel ());
	reschedule_aev (BOP);
}

void Transceiver::reschedule_eop () {
/*
 * Handles the end of preamble, including aborted preambles (not thresholded)
 */
	ZZ_REQUEST	*rq;
	ZZ_EVENT	*ev;

	for (rq = RQueue [EOP]; rq != NULL; rq = rq->next) {
#if     ZZ_TAG
		rq->when . set (Time);
		if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
			continue;
#else
		rq->when = Time;
		if ((ev = rq->event) -> waketime <= Time && FLIP)
			continue;
#endif
		ev->new_top_request (rq);
	}

	reschedule_aev (EOP);
}

void Transceiver::reschedule_aev (int act) {
/*
 * Any event
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	for (rq = RQueue [ANYEVENT]; rq != NULL; rq = rq->next) {
		if (rq->when == Time && FLIP)
			// ANYEVENT is already scheduled on another occasion
			continue;
		if (act == BOT || act == EOT)
			rq -> Info01 = (void*) tpckt;
		else
			rq -> Info01 = NULL;

		rq -> Info02 = (void*)(IPointer)act;
#if     ZZ_TAG
		rq->when . set (Time);
		if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
			continue;
#else
		rq->when = Time;
		if ((ev = rq->event) -> waketime <= Time && FLIP)
			continue;
#endif
		ev->new_top_request (rq);
	}
}

Packet *Transceiver::findRPacket () {
/*
 * Finds a receivable packet being currently heard by the port. Assumes that
 * RxOn is set - we won't be calling it otherwise.
 */
	ZZ_RSCHED	*a;

	txcvr = this;

	for (a = Activities; a != NULL; a = a->next) {
		if (a->within_packet () && !a->Killed) {
			a->INT.update ();
			tpckt = &(a->RFA->Pkt);
			if (RFC->RFC_eot (a->RFA->TRate, &(a->RSS), &RcvSig,
			    &(a->INT)))
				return tpckt;
		}
	}
	return NULL;
}

void Transceiver::reschedule_sil (double sigl) {
/*
 * Silence
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	txcvr = this;

	if (RxOn && RFC->RFC_act (sigl, &RcvSig))
		return;

	// RxOn == NO implies silence
	
	for (rq = RQueue [SILENCE]; rq != NULL; rq = rq->next) {

		if (rq->when == Time && FLIP)
			// Already scheduled on another occasion
			continue;
#if     ZZ_TAG
		rq->when . set (Time);
		if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
			continue;
#else
		rq->when = Time;
		if ((ev = rq->event) -> waketime <= Time && FLIP)
			continue;
#endif
		ev->new_top_request (rq);
	}
}

void Transceiver::reschedule_act (double sigl) {
/*
 * Activity
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	txcvr = this;

	if (RFC->RFC_act (sigl, &RcvSig) == NO)
		return;

	for (rq = RQueue [ACTIVITY]; rq != NULL; rq = rq->next) {

		if (rq->when == Time && FLIP)
			// ACTIVITY is already scheduled on another occasion
			continue;

		// Find a receivable packet, if any
		rq->Info01 = findRPacket ();

#if     ZZ_TAG
		rq->when . set (Time);
		if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 && FLIP)
			continue;
#else
		rq->when = Time;
		if ((ev = rq->event) -> waketime <= Time && FLIP)
			continue;
#endif
		ev->new_top_request (rq);
	}
}

void Transceiver::reschedule_bot (ZZ_RSCHED *rfa) {
/*
 * Beginning of packet
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	txcvr = this;

	if (RFC->RFC_bot (rfa->RFA->TRate, &(rfa->RSS), &RcvSig, &(rfa->INT))) {
		// Receivable
		for (rq = RQueue [BOT]; rq != NULL; rq = rq->next) {

			if (rq->when == Time && FLIP)
				// Already scheduled on another occasion
				// (unlikely)
				continue;

			rq->Info01 = tpckt;
			TracedActivity = rfa;
#if     ZZ_TAG
			rq->when . set (Time);
			if ((ev = rq->event) -> waketime.cmp (rq->when) <= 0 &&
			    FLIP)
				continue;
#else
			rq->when = Time;
			if ((ev = rq->event) -> waketime <= Time && FLIP)
				continue;
#endif
			ev->new_top_request (rq);
		}

		if (tpckt->isMy (Owner)) {
			for (rq = RQueue [BMP]; rq != NULL; rq = rq->next) {

				if (rq->when == Time && FLIP)
					// Already scheduled on another occasion
					// (unlikely)
					continue;

				rq->Info01 = tpckt;
				TracedActivity = rfa;
#if     ZZ_TAG
				rq->when . set (Time);
				if ((ev = rq->event) -> waketime.cmp (rq->when)
				    <= 0 && FLIP)
					continue;
#else
				rq->when = Time;
				if ((ev = rq->event) -> waketime <= Time &&
				    FLIP)
					continue;
#endif
				ev->new_top_request (rq);
			}
		}

	} else {
		// Not receivable
		rfa->Killed = YES;
	}

	reschedule_aev (BOT);
}

void Transceiver::reschedule_eot (ZZ_RSCHED *rfa) {
/*
 * End of packet
 */
	ZZ_REQUEST      *rq;
	ZZ_EVENT        *ev;

	if (!rfa->Killed) {

		txcvr = this;

		if (RFC->RFC_eot (rfa->RFA->TRate, &(rfa->RSS), &RcvSig,
								&(rfa->INT))) {
			// Receivable
			for (rq = RQueue [EOT]; rq != NULL; rq = rq->next) {

				if (rq->when == Time && FLIP)
					// Already scheduled on another occasion
					// (unlikely)
					continue;

				rq->Info01 = tpckt;
#if     ZZ_TAG
				rq->when . set (Time);
				if ((ev = rq->event) -> waketime.cmp (rq->when)
				    <= 0 && FLIP)
					continue;
#else
				rq->when = Time;
				if ((ev = rq->event) -> waketime <= Time &&
				    FLIP)
					continue;
#endif
				ev->new_top_request (rq);
			}

			if (tpckt->isMy (Owner)) {

				for (rq = RQueue [EMP]; rq != NULL;
				    rq = rq->next) {

					if (rq->when == Time && FLIP)
						// Already scheduled on another
						// occasion
						continue;

					rq->Info01 = tpckt;
#if     ZZ_TAG
					rq->when . set (Time);
					if ((ev = rq->event) ->
					   waketime.cmp (rq->when) <= 0 && FLIP)
						continue;
#else
					rq->when = Time;
					if ((ev = rq->event) -> waketime <=
					    Time && FLIP)
						continue;
#endif
					ev->new_top_request (rq);
				}
			}
		} else {
			// Not receivable: just in case
			rfa->Killed = YES;
		}
	}

	// For AEV, we also look at killed packets
	reschedule_aev (EOT);
	reschedule_sil (sigLevel ());
}

void ZZ_RF_ACTIVITY::handleEvent () {

	TIME tm;
	ZZ_RSCHED *ro;
	Transceiver *D;
	double sigl;

	assert (Roster != NULL, "RF_ACTIVITY->handleEvent: no roster");

	tpckt = &Pkt;
	tm = TIME_inf;
	while (SchBOP != NULL && SchBOP->Schedule <= Time) {
		assert (SchBOP->Schedule == Time,
			"RF_ACTIVITY->HandleEvent: time warp (p)");
		// Initialize the activity
		SchBOP->initAct ();
		D = SchBOP->Destination;
		// Covers BOP, AEV, ACT...
		if (D->RxOn) {
			D->reschedule_bop ();
			D->reschedule_thh (D->sigLevel ());
		}
		// Move it to BOT queue
		ro = SchBOP;
		SchBOP = SchBOP->Next;
		ro->Next = NULL;
		ro->LastEvent = Time;
		ro->Schedule = (BOTTime == TIME_inf) ? TIME_inf :
			BOTTime + ro->Distance;
		if (SchBOT == NULL)
			SchBOT = ro;
		else
			SchBOTL->Next = ro;
		SchBOTL = ro;
		// Scheduled for BOT
		ro->Stage = RFA_STAGE_BOT;
	}

	while (SchBOT != NULL && SchBOT->Schedule <= Time) {
		assert (SchBOT->Schedule == Time,
			"RF_ACTIVITY->HandleEvent: time warp (b)");
		D = SchBOT->Destination;
		if (Aborted) {
			// No packet, aborted preamble
			SchBOT->Done = SchBOT->Killed = YES;
			D->NActivities--;
			SchBOT->Stage = RFA_STAGE_APR;
			if (D->RxOn) {
				D->updateIF ();
				D->reschedule_thl (sigl = D->sigLevel ());
				D->reschedule_eop ();
				D->reschedule_sil (sigl);
			}
			SchBOT = SchBOT->next;
			continue;
		} 

		// We have a packet
		if (D->RxOn) {
			// Covers BOT, BMP, AEV, ...
			SchBOT->INT.update ();
			D->reschedule_bot (SchBOT);
			D->reschedule_eop ();
			// Reset measurements for the packet
			SchBOT->initSS ();
		} else {
			// Make sure it won't be received if the receiver
			// now goes on
			SchBOT->Killed = YES;
		}
		
		// Move it to the EOT queue
		ro = SchBOT;
		SchBOT = SchBOT->Next;
		ro->Next = NULL;
		ro->LastEvent = Time;
		ro->Schedule = (EOTTime == TIME_inf) ? TIME_inf :
			EOTTime + ro->Distance;
		if (SchEOT == NULL)
			SchEOT = ro;
		else
			SchEOTL->Next = ro;
		SchEOTL = ro;
		// Scheduled for EOT
		ro->Stage = RFA_STAGE_EOT;
	}

	while (SchEOT != NULL && SchEOT->Schedule <= Time) {
		assert (SchEOT->Schedule == Time,
			"RF_ACTIVITY->HandleEvent: time warp (e)");
		D = SchEOT->Destination;
		// Remove the activity from the pool
		SchEOT->Done = YES;
		D->NActivities--;
		if (D->RxOn) {
			// This affects the remaining activities
			D->updateIF ();
			// Complete and close this one
			SchEOT->INT.update (0.0);
			D->reschedule_thl (D->sigLevel ());
			// This covers AEV, SILENCE, EOT, EMP ....
			D->reschedule_eot (SchEOT);
		}

		// Remove it from the roster, it will be deallocated later
		// when the whole activity disappears
		SchEOT->Stage = RFA_STAGE_OFF;
		SchEOT->LastEvent = Time;
		SchEOT = SchEOT->Next;
	}

	// Schedule new event

	if (SchBOP != NULL)
		tm = SchBOP->Schedule;
	else
		tm = TIME_inf;

	if (SchBOT != NULL && tm > SchBOT->Schedule)
		tm = SchBOT->Schedule;

	if (SchEOT == NULL) {
		if (SchBOT == NULL && tm == TIME_inf) {
			// This means that we are done: schedule removal for
			// the whole thing
			RE = new ZZ_EVENT (
				Time + Tcv->RFC->PurgeDelay,
				System,		// Station
				(void*)this,	// Info01
				NULL,		// Info02
				rshandle,	// Process
				zz_ai,		// AI
				RACT_PURGE,	// Event
				PurgeActivity,	// State
				NULL		// No request chain
			);
			return;
		}
	} else {
		if (tm > SchEOT->Schedule)
			tm = SchEOT->Schedule;
	}

	if (tm == TIME_inf)
		RE = new ZZ_EVENT (
			System,			// Station
			(void*)this,		// Info01
			NULL,			// Info02
			rshandle,		// Process
			zz_ai,			// AI
			DO_ROSTER,		// Event
			HandleRosterSchedule,	// State
			NULL			// No request chain
		);
	else
		RE = new ZZ_EVENT (
			tm,
			System,			// Station
			(void*)this,		// Info01
			NULL,			// Info02
			rshandle,		// Process
			zz_ai,			// AI
			DO_ROSTER,		// Event
			HandleRosterSchedule,	// State
			NULL			// No request chain
		);
}

void ZZ_RF_ACTIVITY::triggerBOT () {

	ZZ_RSCHED *ro;

	// This also means we are past the preamble
	RF = NULL;
	BOTTime = Time;
	if (SchBOT != NULL) {

		// Schedules for these become definite
		for (ro = SchBOT; ro != NULL; ro = ro->Next)
			ro->Schedule = ro->Distance + Time;

		// Check if should reschedule the service event
#if ZZ_TAG
		if (RE->waketime.cmp (SchBOT->Schedule) > 0) {
			RE->waketime . set (SchBOT->Schedule);
#else
		if (RE->waketime > SchBOT->Schedule) {
			RE->waketime = SchBOT->Schedule;
#endif
			RE->reschedule ();
		}
	}
}

int Transceiver::activities (int &packets) {

/* ----------------------------------------------------- */
/* Return the number of activities perceived at the port */
/* ----------------------------------------------------- */

	ZZ_RSCHED	*a;
	int		act;
	Packet		*p;

	apevn = NONE;

	// For anotherPacket: report all packets currently heard
	zz_temp_event_id = NONE;

#if	ZZ_ASR
	Info02 = (void*) (IPointer) (appid = (int) GYID (Id));
#else
	Info02 = (void*) (IPointer) GYID (Id);
#endif
	Info01 = NULL;

	if (!RxOn)
		return 0;

	packets = act = 0;

	for (a = Activities; a != NULL; a = a->next) {

		if (a->Done)
			continue;
		act++;

		if (a->within_packet ()) {
			packets++;
			if (Info01 == NULL)
				// This must be the first one
				Info01 = (void*) (&(a->RFA->Pkt));
		}
	}

	return act;
}

int Transceiver::anotherActivity () {

	Info01 = NULL;

	if (!RxOn)
		return NONE;

	if (zz_npre != apevn) {
		// First time around
#if     ZZ_ASR
		if (apevn == NONE)
			// Port id already known
			assert (appid == GYID (Id),
 	    		       "Transceiver->anotherActivity: %s, called in an "
				     "illegal context", getSName ());
		else
			appid = (int) GYID (Id);
#endif
		apca = Activities;
		apevn = zz_npre;
	} else {
		// Skip the last one
		apca = apca->next;
	}

	// This and subsequent calls

	assert (appid == GYID (Id),
	       "Transceiver->anotherActivity: %s, called in an illegal context",
		      getSName ());

	switch (zz_temp_event_id) {

	    case BOP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (bop_now (apca))
				return PREAMBLE;
		}
		return NONE;

	    case EOP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (eop_now (apca))
				return PREAMBLE;
		}
		return NONE;

	    case BOT:

		for ( ; apca != NULL; apca = apca -> next) {
			if (bot_now (apca)) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				return TRANSFER;
			}
		}
		return NONE;

	    case EOT:
				
		for ( ; apca != NULL; apca = apca -> next) {
			if (eot_now (apca)) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				return TRANSFER;
			}
		}
		return NONE;

	    case BMP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (bot_now (apca) && apca->RFA->Pkt.isMy ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				return TRANSFER;
			}
		}
		return NONE;

	    case EMP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (eot_now (apca) && apca->RFA->Pkt.isMy ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				return TRANSFER;
			}
		}
		return NONE;

	    default:

		// No particular event - look at all activities

		for ( ; apca != NULL; apca = apca -> next) {
			if (apca->Done)
				continue;
			if (apca->within_packet ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				return TRANSFER;
			}
			return PREAMBLE;
		}
	}
	return NONE;
}

Packet	*Transceiver::anotherPacket () {

	Info01 = NULL;

	if (!RxOn)
		return NULL;

	if (zz_npre != apevn) {
		// First time around
#if     ZZ_ASR
		if (apevn == NONE)
			// Port id already known
			assert (appid == GYID (Id),
	 	    		"Transceiver->anotherPacket: %s, called in an "
					"illegal context", getSName ());
		else
			appid = (int) GYID (Id);
#endif
		apca = Activities;
		apevn = zz_npre;
	} else {
		apca = apca->next;
	}

	// This and subsequent calls

	assert (appid == GYID (Id),
		"Transceiver->anotherPacket: %s, called in an illegal context",
			getSName ());

	switch (zz_temp_event_id) {

	    case BOT:

		for ( ; apca != NULL; apca = apca -> next) {
			if (bot_now (apca)) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				goto Ret;
			}
		}
		goto Ret;

	    case EOT:
				
		for ( ; apca != NULL; apca = apca -> next) {
			if (eot_now (apca)) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				goto Ret;
			}
		}
		goto Ret;

	    case BMP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (bot_now (apca) && apca->RFA->Pkt.isMy ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				goto Ret;
			}
		}
		goto Ret;

	    case EMP:

		for ( ; apca != NULL; apca = apca -> next) {
			if (eot_now (apca) && apca->RFA->Pkt.isMy ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				goto Ret;
			}
		}
		goto Ret;

	    default:

		// No particular packet-related event - look at all packets

		for ( ; apca != NULL; apca = apca -> next) {
			if (apca->Done)
				continue;
			if (apca->within_packet ()) {
				Info01 = (void*) (&(apca->RFA->Pkt));
				goto Ret;
			}
		}
	}
Ret:
	return ((Packet*) Info01);
}

int Transceiver::events (int etype) {

/* -------------------------------------------------------- */
/* Returns the number of overlapping events of a given type */
/* -------------------------------------------------------- */

	ZZ_RSCHED	*a;
	int		count;


	Info01 = NULL;
	Info02 = (void*) (IPointer) GYID (Id);

	if (!RxOn)
		// Don't look any further
		return 0;

	a = Activities;
	apevn = NONE;
	count = 0;

#if	ZZ_ASR
	appid = (int) GYID (Id);
#endif
	switch (etype) {

		case BOP:

			zz_temp_event_id = BOP;
			for ( ; a != NULL; a = a->next)
				if (bop_now (a))
					count++;
			return count;

		case BOT:

			zz_temp_event_id = BOT;
			for ( ; a != NULL; a = a->next)
				if (bot_now (a))
					if (count++ == 0)
						Info01 =
						    (void*) (&(a->RFA->Pkt));
			return count;

		case EOP:

			zz_temp_event_id = EOP;
			for ( ; a != NULL; a = a->next)
				if (eop_now (a))
					count++;
			return count;

		case EOT:

			zz_temp_event_id = EOT;
			for ( ; a != NULL; a = a->next)
				if (eot_now (a))
					if (count++ == 0)
						Info01 =
						    (void*) (&(a->RFA->Pkt));
			return count;

		default:
			excptn ("Transceiver->events: %s, illegal event type "
				"%1d", getSName (), etype);
	}
}

Boolean Transceiver::dead (const Packet *p) {

	ZZ_RSCHED	*a;

	if (!RxOn)
		// Nothing is receivable
		return YES;

	if (p) {
		for (a = Activities; a != NULL; a = a->next) {
			if (&(a->RFA->Pkt) == p) {
Redo:
				// Got it
				if (a->Killed)
					return YES;
				if (!a->in_packet ())
					// Not yet
					return NO;
				a->INT.update ();

				tpckt = (Packet*) p;
				txcvr = this;

				return !RFC->RFC_eot (a->RFA->TRate, &(a->RSS),
					&RcvSig, &(a->INT));
			}
		}
		// FIXME: I am not sure if this is the right thing to
		// return if we can't find the packet
		return NO;
	}

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca == NULL)
		return NO;

	a = apca;
	goto Redo;
}

IHist *Transceiver::iHist (const Packet *p) {

	ZZ_RSCHED	*a;

	if (!RxOn)
		// Nothing is receivable
		return NULL;

	if (p) {
		for (a = Activities; a != NULL; a = a->next) {
			if (&(a->RFA->Pkt) == p) {
				a->INT.update ();
				return &(a->INT);
			}
		}
		return NULL;
	}

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca) {
		apca->INT.update ();
		return &(apca->INT);
	}

	return NULL;
}

Long Transceiver::errors (Packet *p) {

	ZZ_RSCHED	*a;

	if (!RxOn)
		// What to do with this ?
		return ERROR;

	if (p) {
		for (a = Activities; a != NULL; a = a->next) {
			if (&(a->RFA->Pkt) == p) {
				a->INT.update ();
				return errors (&(a->RSS), &(a->INT));
			}
		}
		return ERROR;
	}

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca) {
		apca->INT.update ();
		return errors (&(apca->RSS), &(apca->INT));
	}

	return ERROR;
}

Boolean Transceiver::error (Packet *p) {

	ZZ_RSCHED	*a;

	if (!RxOn)
		// What to do with this ?
		return NO;

	if (p) {
		for (a = Activities; a != NULL; a = a->next) {
			if (&(a->RFA->Pkt) == p) {
				a->INT.update ();
				return error (&(a->RSS), &(a->INT));
			}
		}
		return NO;
	}

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca) {
		apca->INT.update ();
		return error (&(apca->RSS), &(apca->INT));
	}

	return NO;
}

double 	Transceiver::sigLevel (const Packet *p, int which) {

	ZZ_RSCHED	*a;

	if (!RxOn)
		// We hear nothing
		return 0.0;

	for (a = Activities; a != NULL; a = a->next) {
		if (&(a->RFA->Pkt) == p) {
			switch (which) {
				case SIGL_OWN:
					return a->RSS.Level;
				case SIGL_IFM:
					a->INT.update ();
					return a->INT.max ();
				case SIGL_IFA:
					a->INT.update ();
					return a->INT.avg ();
				case SIGL_IFC:
					return a->INT.cur ();
				default:
					excptn ("Transceiver->sigLevel: %s, "
					  "illegal ordinal %1d",
					    getSName (), which);
			}
		}
	}
	// Not found, let's be lenient
	return -1.0;
}

Boolean	Transceiver::follow (Packet *p) {

	ZZ_RSCHED 	*a;

	if (!RxOn)
		return ERROR;

	if (p == NULL) {
		// Currently scanned activity
		if (zz_npre != apevn || apca == NULL || apca->Done) {
			TracedActivity = NULL;
			return ERROR;
		}
		TracedActivity = apca;
		Info01 = (void*) (&(apca->RFA->Pkt));
		return OK;
	}

	for (a = Activities; a != NULL; a = a->next) {
		if (&(a->RFA->Pkt) == p) {
			// Check stage
			if (!a->Done) {
				TracedActivity = a;
				return OK;
			}
			break;
		}
	}

	TracedActivity = NULL;
	return ERROR;
}

Packet  *Transceiver::thisPacket () {
/*
 * Packet of the current activity
 */
	if (!RxOn)
		return NULL;

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca == NULL)
		return NULL;

	Info01 = (void*) (&(apca->RFA->Pkt));

	return (Packet*) Info01;
}

double	Transceiver::sigLevel (int which) {
/*
 * This applies to the current activity (produced by anotherActivity)
 */
	if (!RxOn)
		return 0.0;

	if (zz_npre != apevn)
		// Start it off
		anotherActivity ();

	if (apca == NULL)
		return -1.0;

	switch (which) {
		case SIGL_OWN:
			return apca->RSS.Level;
		case SIGL_IFM:
			apca->INT.update ();
			return apca->INT.max ();
		case SIGL_IFA:
			apca->INT.update ();
			return apca->INT.avg ();
		case SIGL_IFC:
			return apca->INT.cur ();
		default:
			excptn ("Transceiver->sigLevel: %s, "
			  "illegal ordinal %1d", getSName (), which);
	}
}

double Transceiver::setSigThresholdHigh (double t) {

	assert (t >= 0.0,
		"Transceiver->setSigThresholdHigh: %s, threshold (%f) "
			"cannot be negative", getSName (), t);

	double old;

	old = SigThresholdHigh;
	SigThresholdHigh = t;

	if (RxOn) {
		if (t < old)
			reschedule_thh (sigLevel ());
	}

	return old;
}

double Transceiver::setSigThresholdLow (double t) {

	assert (t >= 0.0,
		"Transceiver->setSigThresholdLow: %s, threshold (%f) "
			"cannot be negative", getSName (), t);

	double old;

	old = SigThresholdLow;
	SigThresholdLow = t;

	if (RxOn) {
		if (t > old)
			reschedule_thl (sigLevel ());
	}

	return old;
}
			
double Transceiver::getMinDistance () {

	if (MinDistance == DISTANCE_inf)
		return -1.0;

	return ituToDu (MinDistance);
}

double Transceiver::setMinDistance (double d) {

	double old;

	if (d < 0.0 && RFC)
		d = RFC->DefMinDistance;

	assert (d >= 0.0,
          	"Transceiver->setMinDistance: %s, illegal (undefined) distance "
			"(%f)", getSName (), d);

	old = getMinDistance ();

	if (d == 0.0) {
		MinDistance = DISTANCE_0;
		return old;
	}

	// At least 1 ITU
	if ((MinDistance = duToItu (d)) == DISTANCE_0)
		MinDistance = DISTANCE_1;

	return old;
}

Boolean Transceiver::setAevMode (Boolean b) {

	Boolean old;

	if (b == YESNO && RFC)
		b = RFC->DefAevMode;

	assert (b == YES || b == NO,
          	"Transceiver->setAevMode: %s, illegal (undefined) mode (%1d)",
			getSName (), b);
		
	old = AevMode;
	AevMode = b;
	return old;
}

#if  ZZ_TAG
void    Transceiver::wait (int ev, int pstate, LONG tag) {
	int q;
#else
void    Transceiver::wait (int ev, int pstate) {
#endif

	TIME            t, ts;
	ZZ_RSCHED	*a;
	int             evid;

	if_from_observer ("Transceiver->wait: called from an observer");

	evid = ev;
	tpckt = NULL;

	if (zz_c_first_wait) {

		zz_c_other = NULL;    // No requests so far

		// Create new simulation event
		zz_c_wait_event = new ZZ_EVENT;
		zz_c_wait_event -> station = TheStation;
		zz_c_wait_event -> process = TheProcess;
	}

	rinfo = (IPointer) this;
	t = TIME_inf;
	txcvr = this;

	if (RxOn) {

	  switch (ev)     {

	    case SILENCE:

		if (!RFC->RFC_act (sigLevel (), &RcvSig))
			t = Time;
		break;

	    case ACTIVITY:

		if (RFC->RFC_act (sigLevel (), &RcvSig)) {
			t = Time;
			// Find the first receiveable packet. This is pretty
			// much useless, isn't it?
			tpckt = findRPacket ();
		}
		break;

	    case ANYEVENT:

		if (AevMode) {
			// If AevMode is NO, the event will never occur
			// immediately - only if something happens subsequently
			// to the wait request
			for (a = Activities; a != NULL; a = a->next) {
				if ((rinfo = a->any_event ()) != NONE) {
					t = Time;
					break;
				}
			}
		}
		break;

	    case BOP:

		for (a = Activities; a != NULL; a = a->next) {
			if (bop_now (a)) {
				t = Time;
				break;
			}
		}

		break;

	    case BOT:

		for (a = Activities; a != NULL; a = a->next) {

			if (a->Killed)
				continue;

			if (a->Schedule == Time && a->Stage == RFA_STAGE_BOT) {
				// Hasn't been processed by handleEvent yet
				a->INT.update ();
				tpckt = &(a->RFA->Pkt);
				if (RFC->RFC_bot (a->RFA->TRate, &(a->RSS),
				    &RcvSig, &(a->INT))) {
					TracedActivity = a;
					t = Time;
					break;
				}
			} else if (a->LastEvent == Time && a->Stage ==
			    RFA_STAGE_EOT) {
				// Has been processed (and not killed)
				TracedActivity = a;
				tpckt = &(a->RFA->Pkt);
				t = Time;
				break;
			}
		}
		break;

	    case BMP:

		for (a = Activities; a != NULL; a = a->next) {

			if (a->Killed)
				continue;

			if (!(a->RFA->Pkt.isMy ()))
				continue;

			if (a->Schedule == Time && a->Stage == RFA_STAGE_BOT) {
				// Hasn't been processed by handleEvent yet
				a->INT.update ();
				tpckt = &(a->RFA->Pkt);
				if (RFC->RFC_bot (a->RFA->TRate, &(a->RSS),
				    &RcvSig, &(a->INT))) {
					TracedActivity = a;
					t = Time;
					break;
				}
			} else if (a->LastEvent == Time && a->Stage ==
			    RFA_STAGE_EOT) {
				// Has been processed (and not killed)
				TracedActivity = a;
				tpckt = &(a->RFA->Pkt);
				t = Time;
				break;
			}
		}
		break;

	    case EOP:

		// End of preamble
		for (a = Activities; a != NULL; a = a->next) {
			if (eop_now (a)) {
				t = Time;
				break;
			}
		}

		break;

	    case EOT:

		for (a = Activities; a != NULL; a = a->next) {

			if (a->Killed)
				continue;

			if (a->Schedule == Time && a->Stage == RFA_STAGE_EOT) {
				// Hasn't been process by handleEvent yet
				a->INT.update ();

				tpckt = &(a->RFA->Pkt);
	    			if (RFC->RFC_eot (a->RFA->TRate, &(a->RSS),
				    &RcvSig, &(a->INT))) {
					t = Time;
					break;
				}
			} else if (a->LastEvent == Time && a->Stage ==
			    RFA_STAGE_OFF) {
				// Has been processed (and not killed)
				tpckt = &(a->RFA->Pkt);
				t = Time;
				break;
			}
		}
		break;

	    case EMP:

		for (a = Activities; a != NULL; a = a->next) {

			if (a->Killed)
				continue;

			if (!(a->RFA->Pkt.isMy ()))
				continue;

			if (a->Schedule == Time && a->Stage == RFA_STAGE_EOT) {
				// Hasn't been process by handleEvent yet
				a->INT.update ();

				tpckt = &(a->RFA->Pkt);
	    			if (RFC->RFC_eot (a->RFA->TRate, &(a->RSS),
				    &RcvSig, &(a->INT))) {
					t = Time;
					break;
				}
			} else if (a->LastEvent == Time && a->Stage ==
			    RFA_STAGE_OFF) {
				// Has been processed (and not killed)
				tpckt = &(a->RFA->Pkt);
				t = Time;
				break;
			}
		}
		break;

	    case BERROR:

		if (TracedActivity != NULL && !TracedActivity->Done) {

			t = Time + (TIME) TRate * RFC->RFC_erd (TRate,
				&(TracedActivity->RSS), &RcvSig,
				TracedActivity->INT.cur (), ErrorRun);

		} else {
			t = TIME_inf;
		}
		break;

	    case INTLOW:

		if (TracedActivity != NULL && !TracedActivity->Done) {
			TracedActivity->INT.update ();
			if (TracedActivity->INT.cur () <= SigThresholdLow) {
				tpckt = &(TracedActivity->RFA->Pkt);
				t = Time;
			}
		}
		break;

	    case INTHIGH:

		if (TracedActivity != NULL && !TracedActivity->Done) {
			TracedActivity->INT.update ();
			if (TracedActivity->INT.cur () > SigThresholdHigh) {
				tpckt = &(TracedActivity->RFA->Pkt);
				t = Time;
			}
		}
		break;

	    case SIGLOW:

		if (sigLevel () <= SigThresholdLow)
			t = Time;
		break;

	    case SIGHIGH:

		if (sigLevel () > SigThresholdHigh)
			t = Time;
		break;

	    default:

		excptn ("Transceiver->wait: %s, illegal event %1d", getSName (),
			ev);

	  }	// END switch

	} else {
		// RxOn == NO, SILENCE only
		if (ev == SILENCE)
			t = Time;
	}

	// Create the request and queue it at the Transceiver

	new ZZ_REQUEST (&(RQueue [evid]), this, ev, pstate,
		TheStation->Id, (void*) tpckt, (void*) rinfo, (void*) this);

	assert (zz_c_other != NULL,
		"Transceiver->wait: internal error -- null request");

	if (zz_c_first_wait) {

		zz_c_wait_event -> pstate = pstate;
		zz_c_wait_event -> ai = this;
		zz_c_wait_event -> event_id = ev;
#if  ZZ_TAG
		zz_c_wait_tmin . set (t, tag);
		zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
		zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
		zz_c_wait_event -> chain = zz_c_other;
		zz_c_wait_event -> Info01 = (void*) tpckt;
		zz_c_wait_event -> Info02 = (void*) rinfo;

		zz_c_whead = zz_c_other;
		zz_c_other -> when = zz_c_wait_tmin;
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;

		zz_c_first_wait = NO;

		if (def (t))
			zz_c_wait_event->enqueue ();
		else
			zz_c_wait_event->store ();

	} else {

#if     ZZ_TAG
		if (def (t) && ((q = zz_c_wait_tmin . cmp (t, tag)) > 0 ||
			(q == 0 && FLIP))) {
#else
		if (def (t) && (zz_c_wait_tmin > t || (zz_c_wait_tmin == t &&
			FLIP))) {
#endif

			zz_c_wait_event -> pstate = pstate;
			zz_c_wait_event -> ai = this;
			zz_c_wait_event -> event_id = ev;
#if     ZZ_TAG
			zz_c_wait_tmin . set (t, tag);
			zz_c_wait_event -> waketime = zz_c_wait_tmin;
#else
			zz_c_wait_event -> waketime = zz_c_wait_tmin = t;
#endif
			zz_c_wait_event -> chain = zz_c_other;
			zz_c_wait_event -> Info01 = (void*) tpckt;
			zz_c_wait_event -> Info02 = (void*) rinfo;

			zz_c_wait_event -> reschedule ();
		}
#if     ZZ_TAG
		zz_c_other -> when . set (t, tag);
#else
		zz_c_other -> when = t;
#endif
		zz_c_other -> event = zz_c_wait_event;
		zz_c_other -> other = zz_c_whead;
	}
}

void	ZZ_SYSTEM::makeTopologyR () {

	int i, j;
	Station *s;
	Transceiver *Tcv;
	RFChannel *Rfc;

	for (i = 0; i < NStations; i++) {
		s = idToStation (i);
		for (j = 0, Tcv = s->Transceivers; Tcv != NULL; j++,
	  	    Tcv = Tcv->nextp)
			Assert (Tcv->RFC != NULL, "makeTopology: Transceiver "
				"%s is left unregistered", Tcv->getSName ());
	}

	for (i = 0; i < NRFChannels; i++)
		idToRFChannel (i) -> complete ();
}

sexposure (RFChannel)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr, (int) SId);	// Request queue

		sexmode (1)

			exPrint1 (Hdr);			// Performance

		sexmode (2)

			exPrint2 (Hdr, (int) SId);	// Activities

		sexmode (3)

			exPrint3 (Hdr);			// Topology
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ((int) SId);		// Request queue

		sexmode (1)

			exDisplay1 ();			// Performance

		sexmode (2)

			exDisplay2 ((int) SId);		// Activities

		sexmode (3)

			exDisplay3 ((int) SId);		// Topology
	}
}

void Transceiver::dspEvnt (int *elist) {

/* ====================================================== */
/* Return the event status of the transceiver for display */
/* ====================================================== */

	ZZ_RSCHED	*a;
	int 		ev;
/*
 *  ACT, BOP, BOT, EOT, BMP, EMP, ANY, PRE, PAC, KIL, OWN
 *   0    1    2    3    4    5    6    7    8    9   10
 */
	for (ev = 0; ev < 11; ev++)
		elist [ev] = 0;

	txcvr = this;

	if (Activity)
		elist [10] = def (Activity->BOTTime) ? 2 : 1;

	if (RxOn == NO)
		return;

	for (a = Activities; a != NULL; a = a->next) {

		if (a->Done)
			continue;

		elist [0] ++;

		if ((ev = a->any_event ()) != NONE) {
			elist [6] ++;
			if (ev == BOP) {
				elist [1] ++;
				elist [7] ++;
				// No more for this one
				continue;
			}
			if (a->Killed) {
				elist [9] ++;
				continue;
			} else {
				// Assessed events
				tpckt = &(a->RFA->Pkt);
				if (ev == BOT) {
					// Assessment needed
					a->INT.update ();
					if (RFC->RFC_bot (a->RFA->TRate,
					    &(a->RSS), &RcvSig, &(a->INT))) {
						elist [2] ++;
						elist [8] ++;
						if (a->RFA->Pkt.isMy (Owner))
							elist [4] ++;
					}
					continue;
				}
				if (ev == EOT) {
					a->INT.update ();
	    				if (RFC->RFC_eot (a->RFA->TRate,
					    &(a->RSS), &RcvSig, &(a->INT))) {
						elist [3] ++;
						if (a->RFA->Pkt.isMy (Owner))
							elist [5] ++;
					}
					continue;
				}
			}
		}

		if (a->in_packet ()) {
			elist [8] ++;
		} else {
			elist [7] ++;
		}
	}
}

void RFChannel::exPrint0 (const char *hdr, int sid) {

/* ----------------------- */
/* Print the request queue */
/* ----------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid)->getOName ();
		Ouf << ":\n\n";
	}

	if (isStationId (sid))
		Ouf << "           Time ";
	else
		Ouf << "       Time   St";

	Ouf << "    Process/Idn     LState      Event         AI/Idn" <<
		"      State\n";

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || (e->station)->Id != sid))
				continue;

		// Only Transceivers can trigger events

		if ((r = e->chain) == NULL) {
			// This is a system event
			if (zz_flg_nosysdisp) continue;

			if (e->ai->Class != AIC_transceiver ||
			    ((Transceiver*)(e->ai))->RFC != this)
				continue;

			ptime (e->waketime, 11);
			Ouf << "* --- ";
			print (e->process->getTName (), 10);
			Ouf << form ("/%03d ", zz_trunc (e->process->Id, 3));
			// State name
			print (e->process->zz_sn (e->pstate), 10);

			Ouf << ' ';
			print (e->ai->zz_eid (e->event_id), 10);

			Ouf << ' ';
			print (e->ai->getTName (), 10);
			if ((l = e->ai->zz_aid ()) != NONE)
				Ouf << form ("/%03d ", zz_trunc (l, 3));
			else
				Ouf << "     ";

			print (e->process->zz_sn (e->pstate), 10);

			Ouf << '\n';
			continue;

		}

		while (1) {
			if (r->ai->Class == AIC_transceiver &&
			    ((Transceiver*)(r->ai))->RFC == this) {

				if (!isStationId (sid))
					ptime (r->when, 11);
				else
					ptime (r->when, 15);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Uncertain
				else
					Ouf << '?';

				if (!isStationId (sid)) {
					// Identify the station
					if (e->station != NULL &&
					   ident (e->station) >= 0)
						Ouf << form ("%4d ",
						zz_trunc (ident (e->station),
						   3));
					else
						Ouf << " Sys ";
				}
				print (e->process->getTName (), 10);
				Ouf << form ("/%03d ", zz_trunc (e->process->Id,
					3));
				print (e->process->zz_sn (r->pstate), 10);
				Ouf << ' ';
				print (r->ai->zz_eid (r->event_id), 10);
				Ouf << ' ';
				print (e->ai->getTName (), 10);
				if ((l = e->ai->zz_aid ()) != NONE)
					Ouf << form ("/%03d ", zz_trunc (l, 3));
				else
					Ouf << "     ";
				print (e->process->zz_sn(e->chain->pstate), 10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    RFChannel::exPrint1 (const char *hdr) {

/* ------------------------------ */
/* Print the performance counters */
/* ------------------------------ */

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Performance measures:\n\n";
	}

	print (NTAttempts     , "    Number of transmit attempts:    ");
	print (NTPackets      , "    Number of transmitted packets:  ");
	print (NTBits         , "    Number of transmitted bits:     ");
	print (NRPackets      , "    Number of received packets:     ");
	print (NRBits         , "    Number of received bits:        ");
	print (NTMessages     , "    Number of transmitted messages: ");
	print (NRMessages     , "    Number of received messages:    ");
	print (Etu * ((double)NRBits / (Time == TIME_0 ? TIME_1 : Time)),
				"    Throughput (by received bits):  ");
	print (Etu * ((double)NTBits / (Time == TIME_0 ? TIME_1 : Time)),
				"    Throughput (by trnsmtd bits):   ");

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void RFChannel::exPrtRfa (int sid, const RFChannel *which) {

/* ======================================================================== */
/* Print the list of activities in rosters: shared by RFChannel and Station */
/* exposures                                                                */
/* ======================================================================== */

	ZZ_EVENT *e;
	ZZ_RF_ACTIVITY *rfa;
	Transceiver *tcv;
	int sn;
	Boolean st = isStationId (sid);

	if (!st)
		Ouf << "   St ";				// 6

	if (which == NULL)
		Ouf << "  RFC ";				// 6

	Ouf << "Tcv ";						// 4
	Ouf << "          BTime           FTime  ";		// 15 + 15 + 3
#if ZZ_DBG
	Ouf << " Rcvr   TP     Length  Signature\n";		// 32
#else
	Ouf << " Rcvr   TP     Length\n";			// 21
#endif

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		// We are looking for roster events
		if (e->process != rshandle || (rfa =
		    (ZZ_RF_ACTIVITY*) (e->Info01)) == NULL)
			continue;

		if (e->event_id == BOT_TRIGGER)
			// Avoid duplicate listing
			continue;

		tcv = rfa->Tcv;
		if (which != NULL && tcv->RFC != which)
			// Not our RFChannel
			continue;

		if (st && (sn = GSID (tcv->Id)) != sid)
			// Not our station
			continue;

		if (!st)
			Ouf << form ("%5d ", sn);

		if (which == NULL)
			Ouf << form ("%5d ", tcv->RFC->getId ());

		Ouf << form ("%3d ", zz_trunc (GYID (tcv->Id), 3));

		ptime (rfa->BOTTime, 15);
		Ouf << ' ';
		ptime (rfa->EOTTime, 15);
		if (rfa->Aborted)
			Ouf << "* ";
		else
			Ouf << "  ";

		if (isStationId (rfa->Pkt.Receiver))
			print (zz_trunc (rfa->Pkt.Receiver, 5), 5);
		else if (rfa->Pkt.Receiver == NONE)
			print ("none", 5);
		else
			print ("bcst", 5);
		
		print (zz_trunc (rfa->Pkt.TP, 4), 5);
		print (zz_trunc (rfa->Pkt.TLength, 10), 11);
#if ZZ_DBG
		print (zz_trunc (rfa->Pkt.Signature, 10), 11);
#endif
		Ouf << '\n';
	}	
}

void	RFChannel::exPrint2 (const char *hdr, int sid) {

/* ======================================= */
/* Print the list of activities in rosters */
/* ======================================= */

	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") List of activities";
		if (isStationId (sid))
			Ouf << " for " << idToStation (sid) -> getOName ();
		Ouf << ':';
	}

	Ouf << "\n\n";

	exPrtRfa (sid, this);

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void RFChannel::exPrtTop (Boolean full) {
/* ==================================================================== */
/* The common code of SYSTEM's Topology exposure and RFChannel exPrint3 */
/* ==================================================================== */

	int j, k, fl;
	Transceiver *t, *u;
	ZZ_NEIGHBOR *n;

	Ouf <<
          "    Stat/Tcv     Rate Prmbl  XPower  RPower Nghbrs        X        Y"
#if ZZ_R3D
	     "        Z"
#endif
		"\n";

	for (j = 0; j < NTransceivers; j++) {
		t = Tcvs [j];
		print (GSID (t->Id), 8);
		Ouf << form ("/%03d ", zz_trunc (GYID (t->Id), 3));
		print (t->TRate, 8);
		Ouf << ' ';
		print ((int)(t->Preamble), 5);
		Ouf << ' ';
		print (t->XmtSig.Level, 7);
		Ouf << ' ';
		print (t->RcvSig.Level, 7);
		Ouf << ' ';
		print (t->NNeighbors, 6);
		Ouf << ' ';
		print (ituToDu (t->X), 8);
		Ouf << ' ';
		print (ituToDu (t->Y), 8);
#if ZZ_R3D
		Ouf << ' ';
		print (ituToDu (t->Z), 8);
#endif
		Ouf << '\n';
	}

	if (!full)
		return;

	Ouf << "\n    Transceiver defaults:\n\n";
	print (DefXPower        , "        XPower:      ");
	print (DefRPower        , "        RPower:      ");
	print (DefTRate         , "        TRate:       ");
	print (DefPreamble      , "        Preamble:    ");
	print (DefMinDistance   , "        MinDistance: ");
	print (DefErrorRun      , "        ErrorRun:    ");
	print (DefAevMode       , "        AevMode:     ");
	print (DefXTag          , "        XTag:        ");
	print (DefRTag          , "        RTag:        ");

	Ouf << "\n    Neighbor lists:\n\n";

	for (j = 0; j < NTransceivers; j++) {
		t = Tcvs [j];

		if (t->NNeighbors == 0)
			continue;

		Ouf << "    [";
		print (GSID (t->Id), 5);
		Ouf << form ("/%03d]:", zz_trunc (GYID (t->Id), 3));

		fl = 0;

		for (k = 0; k < t->NNeighbors; k++) {
			u = (n = t->Neighbors + k) -> Neighbor;
			if (fl > 1) {
				fl = 0;
				Ouf << "\n                ";
			}
			Ouf << form ("  <%5d/%03d,", GSID (u->Id), GYID(u->Id));
			print (ituToDu (n->Distance), 12);
				Ouf << '>';
			fl++;
		}
		Ouf << "\n\n";
	}
}

void RFChannel::exPrint3 (const char *hdr) {

	double diam, minx, maxx, miny, maxy;
#if ZZ_R3D
	double minz, maxz;
#endif
	if (hdr != NULL) {
		Ouf << hdr;
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Node distribution";
	}

#if ZZ_R3D
	diam = getRange (minx, miny, minz, maxx, maxy, maxz);
#else
	diam = getRange (minx, miny, maxx, maxy);
#endif

	Ouf << form ("NTransceivers %4d    [<", NTransceivers);
	Ouf << minx << ", " << miny <<
#if ZZ_R3D
		", " << minz <<
#endif
		">, <" << maxx << ", " << maxy <<
#if ZZ_R3D
		", " << maxz <<
#endif
		">], Diam = " << diam << "\n\n";

	exPrtTop (YES);

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void RFChannel::exDisplay0 (int sid) {

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (isStationId (sid) && (e->station == NULL ||
			e->chain == NULL || (e->station)->Id != sid))
				continue;

		// Only Transceivers can trigger events

		if ((r = e->chain) == NULL) {
			// This is a system event
			if (zz_flg_nosysdisp) continue;

			if (e->ai->Class != AIC_transceiver ||
			    ((Transceiver*)(e->ai))->RFC != this)
				continue;

			dtime (e->waketime);
			display ('*');
			display ("---");
			display (e->process->getTName ());
			display (e->process->Id);
			display (e->process->zz_sn (e->pstate));
			display (e->ai->zz_eid (e->event_id));
			display (e->ai->getTName ());
			if ((l = e->ai->zz_aid ()) != NONE)
				display (l);
			else
				display (' ');

			display (e->process->zz_sn (e->pstate));
			continue;
		}

		while (1) {
			if (r->ai->Class == AIC_transceiver &&
			    ((Transceiver*)(r->ai))->RFC == this) {

				dtime (r->when);

				if (pless (e->waketime, r->when))
					display (' ');
				else if (e->chain == r)
					display ('*');
				else
					display ('?');

				if (!isStationId (sid)) {
					// Identify the station
					if (e->station != NULL &&
					   ident (e->station) >= 0)
						display (ident (e->station));
					else
						display ("Sys");
				}

				display (e->process->getTName ());
				display (e->process->Id);
				display (e->process->zz_sn (r->pstate));
				display (r->ai->zz_eid (r->event_id));
				Ouf << ' ';
				display (e->ai->getTName ());
				if ((l = e->ai->zz_aid ()) != NONE)
					display (l);
				else
					display (' ');
				display (e->process->zz_sn (e->chain->pstate));
			}

			if ((r = r->other) == e->chain) break;
		}
	}
}

void    RFChannel::exDisplay1 () {

	display (Etu * ((double)NRBits / (Time == TIME_0 ? TIME_1 : Time)));
	display (Etu * ((double)NTBits / (Time == TIME_0 ? TIME_1 : Time)));

	display (NTAttempts);
	display (NTPackets);
	display (NTBits);
	display (NRPackets);
	display (NRBits);
	display (NTMessages);
	display (NRMessages);
}

void RFChannel::exDspRfa (int sid, const RFChannel *which) {

	ZZ_EVENT *e;
	ZZ_RF_ACTIVITY *rfa;
	Transceiver *tcv;
	int sn;
	Boolean st = isStationId (sid);

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		// We are looking for roster events
		if (e->process != rshandle || (rfa =
		    (ZZ_RF_ACTIVITY*) (e->Info01)) == NULL)
			continue;

		if (e->event_id == BOT_TRIGGER)
			// Avoid duplicate listing
			continue;

		tcv = rfa->Tcv;
		if (which != NULL && tcv->RFC != which)
			// Not our RFChannel
			continue;

		if (st && (sn = GSID (tcv->Id)) != sid)
			// Not our station
			continue;

		if (!st)
			display (sn);

		if (which == NULL)
			display (tcv->RFC->getId ());

		display (GYID (tcv->Id));

		dtime (rfa->BOTTime);
		dtime (rfa->EOTTime);
		if (rfa->Aborted)
			display ('*');
		else
			display (' ');

		if (isStationId (rfa->Pkt.Receiver))
			display (rfa->Pkt.Receiver);
		else if (rfa->Pkt.Receiver == NONE)
			display ("none");
		else
			display ("bcst");
		
		display (rfa->Pkt.TP);
		display (rfa->Pkt.TLength);
#if ZZ_DBG
		display (rfa->Pkt.Signature);
#else
		display ("---");
#endif
	}	
}

void	RFChannel::exDisplay2 (int sid) {

/* ======================================= */
/* Print the list of activities in rosters */
/* ======================================= */

	exDspRfa (sid, this);
}

void RFChannel::exDisplay3 (int sid) {

	int i, j, n;
	Transceiver *t, *u;
	double dx, dy, minx, maxx, miny, maxy;

#if ZZ_R3D
	// We make it a projection along the Z axis
	double minz, maxz;

	getRange (minx, miny, minz, maxx, maxy, maxz);
#else
	getRange (minx, miny, maxx, maxy);
#endif

	dx = (maxx - minx) / 20.0;
	if (dx == 0)
		dx = 1.0;
	dy = (maxy - miny) / 20.0;
	if (dy == 0)
		dy = 1.0;

	startRegion (minx - dx, maxx + dx, miny - dy, maxy + dy);

	if (isStationId (sid)) {
		// Do the selected ports and their neighborhoods first
		for (i = 0; i < NTransceivers; i++) {
			t = Tcvs [i];
			if (GSID (t->Id) != sid || t->Mark)
				continue;
			t->Mark = YES;
			// Points
			startSegment ((0x02 << 0)	// Points
				     +(0x07 << 2)	// Thickness
				     +(0x0b << 6)	// Color == red
			);
			displayPoint (ituToDu (t->X), ituToDu (t->Y));
			endSegment ();
			// Now go through the neighbors
			if ((n = t->NNeighbors) == 0)
				continue;
			startSegment ((0x02 << 0)	// Points
				     +(0x07 << 2)	// Thickness
				     +(0x05 << 6)	// Color == green
			);
			for (j = 0; j < n; j++) {
				u = t->Neighbors [j] . Neighbor;
				if (u->Mark)
					continue;
				u->Mark = YES;
				displayPoint (ituToDu (u->X), ituToDu (u->Y));
			}
			endSegment ();
		}
	}

	startSegment ((0x02 << 0)	// Points
		     +(0x07 << 2)	// Thickness
		     +(0x00 << 6)	// Color == black
	);

	for (i = 0; i < NTransceivers; i++) {
		t = Tcvs [i];
		if (t->Mark) {
			t->Mark = NO;
			continue;
		}
		displayPoint (ituToDu (t->X), ituToDu (t->Y));
	}

	endSegment ();
	endRegion ();

	display (minx);
	display (miny);
	display (maxx);
	display (maxy);
}

sexposure (Transceiver)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);		// Request queue

		sexmode (1)

			exPrint1 (Hdr);		// Activities

		sexmode (2)

			exPrint2 (Hdr);		// Neighborhood
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ();		// Request queue

		sexmode (1)

			exDisplay1 ();		// Activities

		sexmode (2)

			exDisplay2 ();		// Neighborhood
	}
}

void Transceiver::exPrint0 (const char *hdr) {

/* ------------------------ */
/* Prints the request queue */
/* ------------------------ */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;
	Station                         *s;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Full AI wait list:\n\n";
	}

	Ouf << "           Time ";

	Ouf << "    Process/Idn     PState      Event      State\n";

	s = idToStation (GSID (Id));

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->station != s || (r = e->chain) == NULL) continue;

		while (1) {

			if (r->ai == this) {

				ptime (r->when, 15);

				if (pless (e->waketime, r->when))
					Ouf << ' ';     // Obsolete
				else if (e->chain == r)
					Ouf << '*';     // Current
				else
					Ouf << '?';

				print (e->process->getTName (), 10);
				Ouf << form ("/%03d ",
						zz_trunc (e->process->Id, 3));
				print (e->process->zz_sn (r->pstate), 10);

				Ouf << ' ';
				print (r->ai->zz_eid (r->event_id), 10);

				Ouf << ' ';

				print (e->process->zz_sn (e->chain->pstate),10);
				Ouf << '\n';
			}

			if ((r = r->other) == e->chain) break;
		}
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Transceiver::exPrint1 (const char *hdr) {

/* ---------- */
/* Activities */
/* ---------- */

	Packet	*p;
	ZZ_RF_ACTIVITY *rfa;
	ZZ_RSCHED *a;
	char st;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Activities:\n\n";
	}

	Ouf << "Total incoming:" << form ("%4d", NActivities) <<
		",    Siglevel: ";
	print ((double) sigLevel (), 8);

	if (Activity)
		Ouf << ",    XMITTING";

	Ouf << '\n';

	Ouf << " Sndr   Rcvr Tp Length ST           Time   LvlOwn   LvlCur" <<
		"   LvlMax   LvlAve";
#if ZZ_DBG
	Ouf << "  Signature";
#endif
	Ouf << '\n';

	if (Activity) {
		// Own activity
		p = &(Activity -> Pkt);

		if (isStationId (p->Sender))
			print (zz_trunc (p->Sender, 5), 5);
		else if (p->Sender == NONE)
			print ("none", 5);
		else
			print ("inv", 5);

		Ouf << ' ';

		if (isStationId (p->Receiver))
			print (zz_trunc (p->Receiver, 5), 6);
		else if (p->Receiver == NONE)
			print ("none", 6);
		else
			print ("bcst", 6);

		print (zz_trunc (p->TP, 2), 3);
		print (zz_trunc (p->TLength, 6), 7);

		Ouf << ' ';
		if (Activity->Aborted)
			Ouf << '*';
		else
			Ouf << ' ';

		// 'O' for 'own'
		Ouf << 'O';

		Ouf << ' ';
		ptime (Activity->BOTTime, 14);
		Ouf << ' ';

		print (XmtSig.Level, 8);
		Ouf << "   ------   ------   ------";
#if ZZ_DBG
		print (zz_trunc (p->Signature, 10), 11);
#endif
		Ouf << '\n';
	}

	for (a = Activities; a != NULL; a = a -> next) {

		p = &((rfa = a->RFA) -> Pkt);

		if (isStationId (p->Sender))
			print (zz_trunc (p->Sender, 5), 5);
		else if (p->Sender == NONE)
			print ("none", 6);
		else
			print ("inv", 6);

		Ouf << ' ';

		if (isStationId (p->Receiver))
			print (zz_trunc (p->Receiver, 5), 6);
		else if (p->Receiver == NONE)
			print ("none", 6);
		else
			print ("bcst", 6);

		print (zz_trunc (p->TP, 2), 3);
		print (zz_trunc (p->TLength, 6), 7);

		Ouf << ' ';

		if (a->Killed)
			Ouf << '*';
		else
			Ouf << ' ';

		if (a->Done)
			st = 'D';
		else if (a->Stage == RFA_STAGE_BOP)
			st = 'P';
		else if (a->Stage == RFA_STAGE_BOT)
			st = 'T';
		else if (a->Stage == RFA_STAGE_APR)
			st = 'A';
		else
			st = 'E';

		Ouf << st << ' ';

		ptime (a->Schedule, 14);
		Ouf << ' ';

		a->INT.update ();

		print (a->RSS.Level , 8); Ouf << ' ';
		print (a->INT.cur (), 8); Ouf << ' ';
		print (a->INT.max (), 8); Ouf << ' ';
		print (a->INT.avg (), 8); Ouf << ' ';
#if ZZ_DBG
		print (zz_trunc (p->Signature, 10), 10);
#endif
		Ouf << '\n';
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Transceiver::exPrint2 (const char *hdr) {

/* ====================== */
/* Print the neighborhood */
/* ====================== */

	int i;
	ZZ_NEIGHBOR *n;
	Transceiver *t;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Neighborhood:\n\n";
	}

	Ouf << "Location: [" << ituToDu (X) << ", " << ituToDu (Y) <<
#if ZZ_R3D
		", " << ituToDu (Z) <<
#endif
					"]\n";

	Ouf <<

#if ZZ_R3D
	 "Station   Port     Distance            X            Y            Z\n";
#else
	 "Station   Port     Distance            X            Y\n";
#endif

	for (i = 0; i < NNeighbors; i++) {
		t = (n = Neighbors + i) -> Neighbor;
		print (GSID (t->Id), 7);
		print (GYID (t->Id), 7);
		Ouf << ' ';
		print (ituToDu (n->Distance), 12);
		Ouf << ' ';
		print (ituToDu (t->X), 12);
		Ouf << ' ';
		print (ituToDu (t->Y), 12);
#if ZZ_R3D
		Ouf << ' ';
		print (ituToDu (t->Z), 12);
#endif
		Ouf << '\n';
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
}

void    Transceiver::exDisplay0 () {

/* ------------------------- */
/* Display the request queue */
/* ------------------------- */

	ZZ_REQUEST                      *r;
	ZZ_EVENT                        *e;
	Long                            l;
	Station                         *s;

	s = idToStation (GSID (Id));

	for (e = zz_eq; e != zz_sentinel_event; e = e->next) {

		if (e->station != s || (r = e->chain) == NULL) continue;

		while (1) {

			if (r->ai == this) {

				dtime (r->when);

				if (pless (e->waketime, r->when))
					display (' ');
				else if (e->chain == r)
					display ('*');
				else
					display ('?');

				display (e->process->getTName ());
				display (e->process->Id);
				display (e->process->zz_sn (r->pstate));
				display (r->ai->zz_eid (r->event_id));
				display (e->process->zz_sn (e->chain->pstate));
			}
			if ((r = r->other) == e->chain) break;
		}
	}
}

void    Transceiver::exDisplay1 () {

/* ---------- */
/* Activities */
/* ---------- */

	Packet	*p;
	ZZ_RF_ACTIVITY *rfa;
	ZZ_RSCHED *a;
	char st;

	display (NActivities);
	display (sigLevel ());
	if (Activity)
		display ("XMTG");
	else
		display ("IDLE");

	if (Activity) {
		// Own activity
		p = &(Activity -> Pkt);

		if (isStationId (p->Sender))
			display (p->Sender);
		else if (p->Sender == NONE)
			display ("none");
		else
			display ("inv");

		if (isStationId (p->Receiver))
			display (p->Receiver);
		else if (p->Receiver == NONE)
			display ("none");
		else
			display ("bcst");

		display (p->TP);
		display (p->TLength);

		if (Activity->Aborted)
			display ('*');
		else
			display (' ');

		display ('O');

		dtime (Activity->BOTTime);

		display (XmtSig.Level);
		display ("---");
		display ("---");
		display ("---");
#if ZZ_DBG
		display (p->Signature);
#else
		display ("---");
#endif
	}

	for (a = Activities; a != NULL; a = a -> next) {
		p = &((rfa = a->RFA) -> Pkt);

		if (isStationId (p->Sender))
			display (p->Sender);
		else if (p->Sender == NONE)
			display ("none");
		else
			display ("inv");

		if (isStationId (p->Receiver))
			display (p->Receiver);
		else if (p->Receiver == NONE)
			display ("none");
		else
			display ("bcst");

		display (p->TP);
		display (p->TLength);

		if (a->Killed)
			display ('*');
		else
			display (' ');

		if (a->Done)
			st = 'D';
		else if (a->Stage == RFA_STAGE_BOP)
			st = 'P';
		else if (a->Stage == RFA_STAGE_BOT)
			st = 'T';
		else if (a->Stage == RFA_STAGE_APR)
			st = 'A';
		else
			st = 'E';

		display (st);

		dtime (a->Schedule);

		a->INT.update ();
		display (a->RSS.Level);
		display (a->INT.cur ());
		display (a->INT.max ());
		display (a->INT.avg ());
#if ZZ_DBG
		display (p->Signature);
#else
		display ("---");
#endif
	}
}

void    Transceiver::exDisplay2 () {

/* ======================== */
/* Display the neighborhood */
/* ======================== */

	int i;
	ZZ_NEIGHBOR *n;
	Transceiver *t;

	display (X);
	display (Y);
#if ZZ_R3D
	display (Z);
#else
	display ('-');
#endif
	for (i = 0; i < NNeighbors; i++) {
		t = (n = Neighbors + i) -> Neighbor;
		display (GSID (t->Id));
		display (GYID (t->Id));
		display (ituToDu (n->Distance));
		display (ituToDu (t->X));
		display (ituToDu (t->Y));
#if ZZ_R3D
		display (ituToDu (t->Z));
#else
		display ('-');
#endif
	}
}

/* ================================================= */
/* Default reception assessment methods of RFChannel */
/* ================================================= */

double RFChannel::RFC_att (const SLEntry *sl, double dst, Transceiver *s) {
/* =========== */
/* Attenuation */
/* =========== */

	// No attenuation
	return sl->Level;
}

double RFChannel::RFC_add (int n, int own, const SLEntry **sl,
	const SLEntry *xmt) {

/* ============================ */
/* Simple additive interference */
/* ============================ */

	double tsl;

	tsl = xmt->Level;

	while (n--)
		if (n != own)
			tsl += sl [n] -> Level;
	return tsl;
}

Boolean RFChannel::RFC_act (double sl, const SLEntry *rp) {

/* ============== */
/* Activity sense */
/* ============== */

	return (sl > 0);
}

Boolean RFChannel::RFC_bot (RATE tr, const SLEntry *rl, const SLEntry *sen,
	const IHist *h) {

/* ============== */
/* BOT assessment */
/* ============== */

	Long nbits;

	if ((nbits = h->bits (tr)) < 8)
		return NO;

	if (h->max (tr, nbits - 8, 8) > 0.0)
		return NO;

	return YES;
}

Boolean RFChannel::RFC_eot (RATE tr, const SLEntry *rl, const SLEntry *sen,
	const IHist *h) {

	return (h->max () == 0.0);
}

double RFChannel::RFC_cut (double xp, double rp) {

	// Infnite
	return Distance_inf;
}

Long RFChannel::RFC_erb (RATE tr, const SLEntry *rl, const SLEntry *sen,
	double ir, Long nb) {

	// Absent
	return excptn ("RFChannel->RFC_erb: %s, must define this method",
		getSName ());
}

Long RFChannel::RFC_erd (RATE tr, const SLEntry *rl, const SLEntry *sen,
	double ir, Long nb) {

	// Absent
	return excptn ("RFChannel->RFC_erd: %s, must define this method",
		getSName ());
}

TIME RFChannel::RFC_xmt (RATE tr, Packet *p) {

	return (TIME) tr * (p->TLength);
}

#endif	/* NOR */
