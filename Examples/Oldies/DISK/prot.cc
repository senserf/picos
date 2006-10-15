/* ------------------------ */
/* A simple model of a disk */
/* ------------------------ */

#define         READ            0               // Operation codes
#define         WRITE           1

identify (Disk simulator);

message Request {
  int  SecAddr, Type;                           // from the client
};

packet Preamble {
  int  SecAddr, Type;                           // request
    void setup (Message *m) {
      SecAddr = ((Request*) m) -> SecAddr;      // Sector address
      Type = ((Request*) m) -> Type;            // READ / WRITE
    };
};

packet Response {

};                                              // No non-standard attributes

packet Sector {

};

traffic ReadPattern (Request, Preamble) {

};

traffic WritePattern (Request, Preamble) {

};

RATE ChannelBandwidth;                          // The channel capacity

ReadPattern  *RTraffic;
WritePattern *WTraffic;

int          DiskSize,                          // The total number of sectors
	     SecPerTrack,                       // Sectors per track
	     SecPerCyl,                         // Sectors per cyllinder
	     SecLength,                         // Sector length in bits
	     PreLength,                         // Request preamble length
	     ResLength;                         // Response length

TIME         SecAdvTime,    // The time to rotate to the next sector on track
	     SeekStTime,    // Seek startup time
	     SeekTmTime,    // Seek braking time
	     SeekPerCyl;    // Seek time per cylinder sweeped

RVariable    *Delay;        // For measuring service delay

station Disk {                  // The disk drive
  int      CurCyl, CurSec;      // Current position
  Port     *CtrPort;            // Port to the controller
  Sector   SecBuf;              // Sector buffer
  Response ResBuf;              // Response packet to be sent to controller
  Mailbox  *NewSector,          // Event triggered at each sector boundary
	   *Positioned;         // Event triggered when the head is positioned
    void setup () {
      NewSector = create Mailbox (0);
      Positioned = create Mailbox (0);
      CtrPort = create Port (ChannelBandwidth);
      CurCyl = CurSec = 0;      // Initial positionning
    };
    TIME seekTime (int cyl);    // Determines seek time to the given cylinder
};

station Controller {            // The controller
  int      CurCyl;              // Controller's notion of current cylinder ...
				// (not used - reserved for seek optimization)
  Port     *DskPort;            // Port to the disk
  Preamble RqsBuf;              // Request buffer
  Sector   SecBuf;              // Sector buffer
    void setup () {
      DskPort = create Port (ChannelBandwidth);
      CurCyl = 0;
    };
};

Controller   *TheController;
Disk         *TheDisk;

TIME Disk::seekTime (int cyl) {
  // This is a somewhat simplistic function for determining the seek time
  // from the current cylinder to the new cylinder
  return (SeekStTime + SeekTmTime + SeekPerCyl *
    (cyl>CurCyl ? cyl-CurCyl : CurCyl-cyl));
};

process Drive (Disk) {
  // This process models the disk drive (spindle) by advancing to the
  // next sector every SecAdvTime interval
  states {WaitSector, NextSector};
  perform;
};

Drive::perform {
  state WaitSector:
    Timer->wait (SecAdvTime, NextSector);
  state NextSector:
    S->NewSector->put ();
    S->CurSec = ++S->CurSec % SecPerTrack;
    proceed (WaitSector);
};

process Service;

process Seek (Disk, Service) {
  // An instance of this process is created whenever a seek operation is
  // to be performed
  int   TargetCyl;
    void setup (int CylNum) {
      TargetCyl = CylNum;
    };
    states {Start, Done};
    perform;
};

Seek::perform {
  state Start:
    Timer->wait (S->seekTime (TargetCyl), Done);
  state Done:
    S->Positioned->put ();
    terminate ();
};

process Find (Disk, Service) {
  // An instance of this process is created after the seek is done,
  // to wait until the requested sector appears under the head
  int   TargetSec;
    void setup (int SecNum) {
      TargetSec = SecNum;
    };
    states {WaitSec, CheckSec};
    perform;
};

Find::perform {
  state WaitSec:
    S->NewSector->wait (NEWITEM, CheckSec);
  state CheckSec:
    if (S->CurSec == TargetSec) terminate (); else proceed (WaitSec);
};

process Service (Disk) {
  // This process is run by the disk. It services the requests arriving
  // from the controller
  Port     *CtrPort;
  Seek     *Seeker;
  Find     *Finder;
  Response *ResBuf;
  Sector   *SecBuf;
  int       RqType, CylNum, SecNum;
    void setup () {
      CtrPort = S->CtrPort;
      ResBuf = &(S->ResBuf);
      ResBuf->fill (S, TheController, ResLength);
      SecBuf = &(S->SecBuf);
      SecBuf->fill (S, TheController, SecLength);
    };
    states {WaitRqst, NewRqst, WaitForMore, SecReady, SeekDone, Found,
	    Completed, SendResponse, AllDone};
    perform {
	state WaitRqst:
	  CtrPort->wait (EMP, NewRqst);
	state NewRqst:
	  RqType = ((Preamble*) ThePacket)->Type;
	  CylNum = ((Preamble*) ThePacket)->SecAddr / SecPerCyl;
	  SecNum = ((Preamble*) ThePacket)->SecAddr % SecPerTrack;
	  Seeker = create Seek (CylNum);          // Start the seek operation
	  if (RqType == WRITE)
	    // The sector will follow
	    skipto (WaitForMore);
	  else
	    S->Positioned->wait (NEWITEM, SeekDone);
	state WaitForMore:
	  // Wait for the stuff to be written
	  CtrPort->wait (EMP, SecReady);
	state SecReady:
	  // Wait for the seek operation to complete
	  S->Positioned->wait (NEWITEM, SeekDone);
	state SeekDone:
	  Finder = create Find (SecNum);
	  Finder->wait (DEATH, Found);
	state Found:
	  if (RqType == WRITE)
	    // Write is completed right after the sector is gone
	    Timer->wait (SecAdvTime, SendResponse);
	  else
	    // Transmit the sector contents to the controller (we assume
	    // that the channel and the disk operate with the same speed
	    CtrPort->transmit (SecBuf, Completed);
	state Completed:
	  CtrPort->stop ();
	transient SendResponse:
	  CtrPort->transmit (ResBuf, AllDone);
	state AllDone:
	  CtrPort->stop ();
	  proceed (WaitRqst);
    };
};

process Handler (Controller) {
  // This process is run by the controller. It accepts request coming from
  // the client and transforms them into internal requests to be passed
  // to the disk.
  Port     *DskPort;
  Preamble *RqsBuf;
  Sector   *SecBuf;
  TIME      RQTime;
    void setup () {
      DskPort = S->DskPort;
      RqsBuf = &(S->RqsBuf);
      SecBuf = &(S->SecBuf);
      SecBuf->fill (S, TheDisk, SecLength);
    };
    states {TryNext, RqstSent, SectorRead, SectorWritten, Completed};
    perform {
	state TryNext:
	  if (Client->getPacket (RqsBuf)) {
	    // There is a pending request
	    RQTime = RqsBuf->QTime;
	    DskPort->transmit (RqsBuf, RqstSent);
	  } else {
	    // Wait for a new request
	    Client->wait (ARRIVAL, TryNext);
	  }
	state RqstSent:
	  DskPort->stop ();
	  RqsBuf->release ();
	  if (RqsBuf->Type == READ)
	    // Await the sector
	    DskPort->wait (EMP, SectorRead);
	  else
	    // Write a sector
	    DskPort->transmit (SecBuf, SectorWritten);
	state SectorRead:
	  // Wait for the response
	  DskPort->wait (EMP, Completed);
	state SectorWritten:
	  DskPort->stop ();
	  DskPort->wait (EMP, Completed);
	state Completed:
	  Delay->update ((Time - RQTime)*Itu);
	  proceed (TryNext);
    };
};

process RqstGen (Controller) {
  // This is a simple extension of the standard client. This process
  // intercepts messages coming from the client and adds the sector
  // number to each request.
    states {WaitRqst, NewRqst};
    perform {
	state WaitRqst:
	  Client->wait (INTERCEPT, NewRqst);
	state NewRqst:
	  ((Request*)TheMessage)->SecAddr = (int) toss (DiskSize);
	  ((Request*)TheMessage)->Type =
	    (idToTraffic (TheMessage->TP) == RTraffic) ? READ : WRITE;
	  proceed (WaitRqst);
    };
};

process Root {
  // The main process
  BIG   TimeLimit;
  void  Topology (), Traffic ();
    states {Start, Stop};
    perform {
	state Start:
	  setEtu (1000000000000.0);  // 1 ITU = 1 picosecond, 1 ETU = 1 second
	  setTolerance ();           // Synchronized clocks
	  readIn (DiskSize);
	  readIn (SecPerTrack);
	  readIn (SecPerCyl);
	  readIn (SecLength);
	  readIn (SecAdvTime);
	  readIn (SeekStTime);
	  readIn (SeekTmTime);
	  readIn (SeekPerCyl);
	  readIn (PreLength);    // Length of the request
	  readIn (ResLength);    // Length of the response
	  Topology ();  Traffic ();
	  Client->printDef ("Traffic parameters");
	  Kernel->wait (DEATH, Stop);
	  readIn (TimeLimit);
	  setLimit (0, TimeLimit);
	state Stop:
	  print (Time, "Simulated time: ");
	  print (cpuTime (), "Execution time: ");
	  Delay->printCnt ();
    };
};

void Root::Topology () {
  int  ChannelLength;
  Link *Channel;
    readIn (ChannelLength);
    ChannelBandwidth = SecAdvTime / SecLength;
    // Create the channel connecting disk to controller
    Channel = create Link (2);
    // Create the disk and the controller
    TheDisk = create Disk ();
    TheController = create Controller;
    // Conenct the ports
    TheDisk->CtrPort->connect (Channel);
    TheController->DskPort->connect (Channel);
    // Set the distance
    TheDisk->CtrPort->setDTo (TheController->DskPort, ChannelLength);
    // Create the processes
    TheStation = TheDisk;
    create Drive;
    create Service;
    TheStation = TheController;
    create Handler;
    create RqstGen;
    // The random variable to measure the service time
    Delay = create RVariable;
};

void Root::Traffic () {
  double  MnRIT, MnWIT;
    readIn (MnRIT);        // Mean read inter-arrival time
    readIn (MnWIT);        // Mean write inter-arrival time
    RTraffic = create ReadPattern (MIT_exp+MLE_unf+SPF_off, MnRIT,
				(double) PreLength, (double) PreLength);
    WTraffic = create WritePattern (MIT_exp+MLE_unf+SPF_off, MnWIT,
				(double) PreLength, (double) PreLength);
    RTraffic->addSender (TheController);
    WTraffic->addSender (TheController);
    RTraffic->addReceiver (TheDisk);
    WTraffic->addReceiver (TheDisk);
};
