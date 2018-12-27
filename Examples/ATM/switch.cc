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
identify "ATM Switch Model Exercise";


		    /* ------------------------------ */
		    /*   A sample ATM switch model    */
		    /*                                */
		    /* Written by PG, April 6-7, 1994 */
		    /* ------------------------------ */


// I  have  put  everything into a single file, although one can see here a few
// reasonably independent components. This way it should be easier to read  the
// program.

// I wrote this program from scratch. Srini has another program and we may want
// to  merge them later into a single (hopefully better) solution. There are no
// tricks aimed at improving the efficiency of this simulator, except for a few
// trivial ones. I understand that the efficiency of this  program  is  not  an
// issue.  SMURPH  is  a  specification  system,  not  just a simulator, so the
// protocol  should  be  legible  (and   reasonably   close   to   a   possible
// implementation).  Therefore,  links  are links, cells are cells, buffers are
// buffers, etc.

   // One simplification (we discussed it with Steve) is the elimination of VCI
   // switching).  A  connection  is identified by a global VCI selected by the
   // connection originator before  the  connection  request  is  issued.  This
   // simplifies  the  switch  operation  a bit without affecting the realistic
   // status of the model.

   // Another simplification (with respect to Srini's version of the simulator)
   // is  the  assumption  that every message exchanged during connection setup
   // consists of a single cell. Srini says that in real  life  one  should  be
   // able  to  handle  multiple-cell setup messages. I am not sure how serious
   // this problem is.

// I  model  here  an  ad-hoc connection setup algorithm. I don't know the Q93B
// standard (I guess it can be obtained from somewhere), but this  standard  is
// being  revised  and, most likely, several flexible versions of this standard
// will be adopted. Thus one can assume that  the  connection  setup  procedure
// belongs  to  the switch specification (which is intentionally exchangeable).
// The   switch   structure   is   defined   in   such   a   way    that    its
// implementation/vendor-specific   components  can  be  specified  separately.
// Unfortunately, most of the interesting elements of a switch  fit  into  this
// category.  I  am including the specification of a sample switch with buffers
// attached to output ports. This switch is completely specified (together with
// its connection setup algorithm) and the whole specification  is  executable.
// Of  course,  I have also defined a sample network. The network consists of a
// number of internal 4x4 switches and a number  of  access  (end)  nodes.  Two
// sample  traffic  patterns are included: file transfer + video session. These
// patterns are  a  bit  simplistic.  I  don't  want  to  trespass  on  Carey's
// territory.  Besides,  I  am  not  sure that I know how these patterns should
// realistically look.
//
// Have fun,
//
//                           P.G.

#define MAXCONNECTIONS 1024
   // This  gives  the  maximum  number  of  connections  that  can  be  opened
   // simultaneously in the entire network. Determines the size of some arrays.

#define CellSize    (53*8)    // Total cell size in bits
#define PayloadSize (44*8)    // Cell payload size (also in bits)
   // We need these to know how to turn messages into packets and vice versa.

#define DROP 0    // A cell that can be dropped
#define OOBD 1    // An "out-of-band" cell
#define PACK 2    // Positive acknowledgement
#define NACK 3    // Negative acknowledgement
   // These  are  used  as attributes (binary flags) associated with cells that
   // are exchanged by a pair of connected peers (and-nodes). We will call them
   // "data cells", but one should be aware that some of those data  cells  are
   // acknowledgements for file transfers, etc. The idea is that OOBD indicates
   // an  "out-of-band"  cell,  i.e.,  a  cell  that  doesn't  carry data but a
   // "special message". At present, such a cell can  only  be  a  positive  or
   // negative acknowledgement. DROP indicates a "red" cell that can be dropped
   // if buffer space becomes short.

#define DTC 0    // Data cell
#define CST 1    // Call-setup (connection request)
#define ACK 2    // Call-setup acknowledgement
#define CNK 3    // Call-setup rejection
#define DSC 4    // Disconnect request
#define DAK 5    // Disconnect acknowledgement
   // These  are  cell types used to tell apart various control cells exchanged
   // during connection negotiation. DTC indicates a "data cell" in  the  sense
   // of the above definition. Any other type identifies a control cell.

#define FREE  NO
#define USED  YES
   // VCI  slot  status  (used  by the external algorithm allocating global VCI
   // numbers to connections)

#define TheCell ((Cell*) ThePacket)
   // This  cast  is used to provide a handle to the "current cell". After some
   // events SMURPH returns in the global variable ThePacket the pointer to the
   // "packet" causing the event.  Being  of  the  general  type  Packet,  this
   // pointer must be cast to the proper actual packet type.

Boolean VCISlots [MAXCONNECTIONS];
   // This array gives the status of all potential VCI slots. A VCI is a number
   // from 0 to MAXCONNECTIONS-1. VCI unused <==> VCISlots [VCI] == FREE.
int     VCISlotLast = 0;    // This speeds up slot allocation a bit

message ATMMessage {
     // This   data  structure  represents  a  message,  e.g.,  a  file  to  be
     // transferred or a video frame. A message is a higher-level transfer unit
     // which is typically de-assembled into several cells. Messages represents
     // objects produced by traffic generators. Each message has a sender and a
     // receiver (let us forget for a while about  broadcast  messages)  and  a
     // definite  length  (in bits). All these attributes are standard and they
     // don't have to be declared here.
  int VCI,
	// Identifies the connection (VCI) for sending the message. As our VCIs
	// are  global,  we can view this attribute as an identification of the
	// virtual channel on which the message should be sent.
      Type;
	// Message  type.  Note  that  signaling  cells  originate  in  traffic
	// generators so they are launched as messages.
  Long SeqNum;
	// Sequence number (important if the message is part of a larger unit).
	// SeqNum  is also used to indicate bandwidth requirements in a control
	// message/cell.
  FLAGS Attributes;  // Flags: DROP, OOBD, PACK, NACK
};

packet Cell {
     // This represents a cell. Cells inherit some attributes from the messages
     // they are acquired from. Some attributes of a cell are implicit: Sender,
     // Receiver,  total  length,  payload  length.  These  attributes  are set
     // automatically when the cell is built (acquired from a message).
  int VCI,
      Type;
  Long SeqNum;
  FLAGS Attributes;
	// All  the  above  items  are  inherited  from the message the cell is
	// acquired from. The setup method below is called automatically when a
	// cell is built from a message (by getPacket -- see the Source process
	// below). Note that the SeqNum attribute of the message is incremented
	// so that it always represents the sequence number of the next cell to
	// be acquired from the message.
  void setup (Message *m) {
    VCI = ((ATMMessage*) m) -> VCI;
    Type = ((ATMMessage*) m) -> Type;
    SeqNum = (((ATMMessage*) m) -> SeqNum) ++;
    Attributes = ((ATMMessage*) m) -> Attributes;
  };
};

int getVCI () {
     // Allocates the first available VCI number and marks this number as USED
  for (int n = MAXCONNECTIONS; n; n--) {
    if (++VCISlotLast == MAXCONNECTIONS) VCISlotLast = 0;
    if (VCISlots [VCISlotLast] == FREE) {
      VCISlots [VCISlotLast] = USED;
      return VCISlotLast;
    }
  }
  excptn ("getVCI: run out of VCI slots");
};

void freeVCI (int vci) {
     // Returns the VCI number to the free pool
  VCISlots [vci] = FREE;
};

class CBuf {
     // This  simple  data  structure  represents  a  cell buffer. Actually, it
     // represents the common part of all conceivable cell buffers. To build an
     // actual cell buffer (whose structure may depend on how the switch  wants
     // to  organize cell buffers) one has to use CBuf as the base class of the
     // actual buffer type declaration (see type FifoItem below).
  char CellHolder [sizeof (Cell)];
	// CellHolder  is a raw storage for a Cell. I prefer to do it this way,
	// as opposed to "Cell CellHolder", because of some "features"  of  C++
	// and  SMURPH.  The  methods  below are used to load the buffer with a
	// cell and get hold of the cell in the buffer. The virtual method free
	// is to be called to mark the  buffer  as  free,  whatever  it  means,
	// depending on the actual buffer implementation. This method should be
	// redefined in a subtype of CBuf.
  public:
  inline void load (Cell *c) { *((Cell*) CellHolder) = *c; };
  inline Cell *cell () { return (Cell*) CellHolder; };
  virtual void free () { };
};

station ATMNode {
     // This  is  the  general  structure  of  every  station  connected to our
     // network, including both switches and end-nodes. Each node has a  number
     // of  input  ports  (may  be  different for different nodes) and the same
     // number of output ports. This number will be  stored  in  NPorts.  Array
     // Neighbors  gives the identity of the nodes reachable from this node via
     // the corresponding output ports.
  Port **IPorts, **OPorts;
  Long *Neighbors;
  int NPorts;
  void setup (int np) {
	// The setup method is called automatically when an ATMNode is created.
	// The  number  of  ports  must then be specified as an argument of the
	// create operation. The method creates  the  arrays,  but  it  doesn't
	// create  the  ports.  They will be created by a special function that
	// actually builds the network according to the user's prescription.
    int i;
    NPorts = np;
    IPorts = new Port* [NPorts];
    OPorts = new Port* [NPorts];
    Neighbors = new Long [NPorts];
    for (i = 0; i < NPorts; i++) {
      IPorts [i] = NULL;  // Flag: unallocated yet
      OPorts [i] = NULL;
      Neighbors [i] = NONE;
    }
  };
};

station Switch : ATMNode {
     // This  is  a generic ATM switch (its specification is still incomplete).
     // VCITable maps VCI numbers into ports. Given  an  allocated  VCI  number
     // known by the switch, VCITable [VCI] gives the pair of ports used by the
     // connection. (Note that every connection in ATM is bidirectional.)
  short VCITable [MAXCONNECTIONS] [2];
     // Below  we declare three stubs (virtual methods) that must be defined in
     // an actual implementation of the switch.
  virtual void csetup (Cell*, int) { };
	// This method is called for every special (signaling) cell arriving at
	// the  switch  (Type  != DTC). The second argument gives the number of
	// the port on which the cell has arrived. The role  of  csetup  is  to
	// implement  the  connection setup algorithm. The method, similarly as
	// the following two methods, is virtual so that  it  can  be  properly
	// defined in a subtype of Switch.
  virtual void enter (Cell*, int) { };
	// This  method  is called for every cell that must be transmitted from
	// the switch via the output port determined by  the  second  argument.
	// Again  the method must be defined specifically for every switch type
	// as it deals with buffers, policing, etc.
  virtual CBuf *acquire (int) {
    return NULL;
  };
	// This  method is called to determine whether there is a cell awaiting
	// transmission on the output port  identified  by  the  argument.  The
	// method  is  called by the process servicing the output port whenever
	// it completes a cell transmission and wants to find another  cell  to
	// take  care  of.  Note  that  acquire  returns  a pointer to the cell
	// buffer. When the transmitting process is  done  with  the  cell,  it
	// should  call  free to deallocate the buffer. If acquire returns NULL
	// (which means that there is no  cell  awaiting  transmission  on  the
	// indicated  output  port), the process should suspend itself until it
	// receives a dummy message on the mailbox associated  with  the  port.
	// The  array  of  mailboxes  for  all  output  ports  of the switch is
	// represented by the following pointer:
  Mailbox **Arrivals;
  void setup (int np) {
	// The  setup  method  of  Switch creates the mailboxes and initializes
	// VCITable to NONE. Note that  it  also  calls  the  setup  method  of
	// ATMNode -- to create the arrays declared there.
    int i;
    ATMNode::setup (np);
    Arrivals = new Mailbox* [np];
    for (i = 0; i < MAXCONNECTIONS; i++)
      VCITable [i][0] = VCITable [i][1] = NONE;
    // Create the mailboxes. Note that they are unstructured capacity-0
    // mailboxes to be used as simple signaling devices.
    for (i = 0; i < np; i++) Arrivals [i] = create Mailbox (0);
  };
};

process Input (Switch) {
     // This process services one input port of a generic ATM switch. Attribute
     // IPort  is the process' private copy of the input port pointer and PIndx
     // gives the index of this port in the input port array  (IPorts)  defined
     // in ATMNode.
  Port *IPort;
  int PIndx;
  void setup (int pn) {
    IPort = S->IPorts [PIndx = pn];
	// Note: S is the standard attribute of every process. It points to the
	// station at which the process is running.
  };
  states {WaitCell, Enter};
	// The  process  code  method is fully specified somewhere below. These
	// declarations just announce the states of this method.
  perform;
};

process Output (Switch) {
     // This process services one output port of a generic switch. This port is
     // pointed  to  by  OPort  and  PIndx gives its index in the switch's port
     // array.
  Port *OPort;
  int PIndx;
  CBuf *SBuf;
	// SBuf  is  a  scratch  pointer  to  the  cell buffer holding the cell
	// currently being transmitted.
  Mailbox *Arrival;
	// Arrival  is  a  local  pointer  to  the  Arrival mailbox (see above)
	// associated with the  output  port  serviced  by  the  process.  This
	// mailbox is used to signal an event whenever an outgoing cell for the
	// given port becomes available.
  void setup (int pn) {
    OPort = S->OPorts [PIndx = pn];
    Arrival = S->Arrivals [PIndx];
  };
  states {Acquire, XDone};
  perform;
};

Input::perform {
    // This is the code method of the input process
  int op;
  state WaitCell:
    // Wait  for cell arrival on the input port. BOT stands for a "beginning of
    // transmission". The event is triggered when  the  first  bit  of  a  cell
    // arrives at the port.
    IPort->wait (BOT, Enter);
  state Enter:
    if (TheCell->Type != DTC)
      S->csetup (TheCell, PIndx);
      // Any  cell  with  Type  !=  DTC  is  considered special. In the present
      // version of the program, such a cell belongs to  the  connection  setup
      // protocol  which  must be specified individually for every switch type.
      // Thus we just call csetup (a virtual method of the  switch  owning  the
      // process) which takes care of the connection setup algorithm.
    else {
      // This is a data cell which must be relayed according to its VCI number.
      // Note  that  there  is a pair of ports associated with every active VCI
      // known to the switch. These ports (their indexes) are kept in  VCITable
      // at  the  switch.  As  every  connection  is bidirectional, if the cell
      // arrives on one of these ports, it must be relayed on the other.  PIndx
      // gives the index of the input port serviced by the process.
      if ((op = S->VCITable [TheCell->VCI][0]) == PIndx)
	op = S->VCITable [TheCell->VCI][1];
      if (op != NONE) S->enter (TheCell, op);
      // The  (virtual)  enter  method  takes care of buffering the cell in the
      // right place, depending on the policy of the switch. At this moment  we
      // only  know  that  the  cell will have to be relayed on the output port
      // number op. Cells with unknown VCI numbers are ignored.
    }
    skipto WaitCell;
};
// Note:  we  don't  model  the time needed by the switch to recognize the cell
// type and to determine its fate. One can believe that this time  is  constant
// (it  must  be  if  the  cells  are to be processed on-line); thus, it can be
// modeled by increasing the link length by a small  amount.  In  other  words,
// this time is included in the propagation time.

Output::perform {
    // This is the code method of the Output process
  state Acquire:
    if (SBuf = S->acquire (PIndx))
      // We  have  managed  to acquire a cell for transmission. We initiate its
      // transfer by calling transmit. When the cell has been  transmitted,  we
      // will wake up in state XDone. The amount of time needed to transmit the
      // cell  is  determined as the product of the total cell length (in bits)
      // and  the  transmission  rate  of  the  port  (link).  In  SMURPH,  the
      // transmission  rate  of  a  port/link  is  specified  as  the number of
      // indivisible time units (ITUs) needed to insert a single bit  into  the
      // port  (thus  it  is  the  reciprocal  of what people normally call the
      // transmission rate). SMURPH typically models time  with  a  granularity
      // much finer than the smallest single bit insertion time for a port.
      OPort->transmit (SBuf->cell (), XDone);
    else
      // No outgoing cell is available. Now we have to wait for an event on the
      // Arrival  mailbox.  When this event is triggered, we will wake up again
      // in  state  Acquire  and  we  will  re-execute   acquire,   this   time
      // successfully.
      Arrival->wait (NEWITEM, Acquire);
  state XDone:
    // The  cell  has  been  transmitted.  In  SMURPH, we have to terminate its
    // transmission explicitly by calling stop. Then we free  the  cell  buffer
    // and   proceed   to  state  Acquire  to  get  a  new  outgoing  cell  for
    // transmission.
    OPort->stop ();
    SBuf->free ();
    proceed Acquire;
// Note:  there  is  a  subtle  difference  between  "proceed"  (used here) and
// "skipto" (which was used in the code method of the Input process). The first
// operation takes no simulated time and can be viewed as a  structured  "goto"
// to  the  indicated state. In contrast, skipto advances the simulated time by
// one ITU (indivisible time  unit)  and  is  typically  used  to  get  rid  of
// persisting  events  that  remain pending until the time is advanced. The BOT
// event sensed by the Input process in state WaitCell occurs  when  the  first
// bit of a cell is heard on the port. This condition remains pending until the
// time is advanced -- at least by one ITU.
};

station EndNode : ATMNode {
     // This station type represents an end-node, i.e., a "client" connected to
     // one  of the switches. We assume that such a node has only one input and
     // one output port providing a bidirectional connection to the switch.
  Cell CurrentCell;
	// CurrentCell  is  used  to  hold  a  cell acquired from a message for
	// transmission. Traffic generators are interfaced with end-nodes  only
	// and  CurrentCell  is  an important part of this interface (actually,
	// the only part directly visible to the generic processes run  by  the
	// station).
  TIME CTimeout;
	// This  gives  the  connection  timeout used by the node. A connection
	// request will be re-issued if a confirmation does not  arrive  within
	// this  interval.  This  is  a  local  attribute  of  EndNode  because
	// different end-nodes may want to use different timeout values.
  void setup ();
};
void EndNode::setup () {
    ATMNode::setup (1);     // A single pair of ports
};

process Source (EndNode) {
     // This  process models the source part of a generic end-node. It acquires
     // cells for transmission and transmits them on the output port. Note that
     // it is the responsibility of a traffic  generator  to  issue  connection
     // requests and interpret responses that may arrive from the network.
  Port *OPort;
  Cell *CurrentCell;
	// These  are  just  local  copies  of  the  relevant attributes of the
	// station owning the process
  void setup () {
    OPort = S->OPorts [0];
    CurrentCell = &(S->CurrentCell);
  };
  states {NewCell, XDone};
	// This  time  we  specify  the  code  method  within  the process type
	// declaration. A code method  has  the  same  rights  as  any  regular
	// method.
  perform {
    state NewCell:
      // In this state the process tries to acquire a cell for transmission. In
      // SMURPH,  the  union of all traffic generators (objects of type Traffic
      // -- see below) is represented by one object known as the Client. To see
      // if there is a message  awaiting  transmission,  the  process  executes
      // getPacket.  The first argument tells where the acquired cell should be
      // put and the remaining three arguments give "packetization parameters":
      // the minimum payload  length,  the  maximum  payload  length,  and  the
      // combined  length  of  all headers and trailers. The method returns YES
      // (nonzero)  if  a  packet  (cell)  has  been  acquired  and  NO  (zero)
      // otherwise.  The latter means that no message is queued at the station.
      // In such case, the process suspends itself awaiting a  message  arrival
      // which is a standard event delivered by the Client.
      if (Client->getPacket (CurrentCell, PayloadSize, PayloadSize,
       CellSize - PayloadSize))
	OPort->transmit (CurrentCell, XDone);
      else
	Client->wait (ARRIVAL, NewCell);
    state XDone:
      OPort->stop ();
      CurrentCell->release ();
      // Having completed the packet transmission we "release" the packet. This
      // makes sense for packets (cells) directly acquired from the Client (the
      // traffic  generator)  and,  among  other  things, updates some standard
      // performance measures. The release operation marks the moment when  the
      // packet departs from the source and this moment can be perceived by the
      // traffic generator (see below).
      proceed NewCell;
  };
};

process Destination (EndNode) {
     // This  process  models the destination portion of an end-node. It simply
     // receives packets (cells) arriving on the input port.
  Port *IPort;
  Long MySId;
  void setup () {
    MySId = S->getId ();
    IPort = S->IPorts [0];
  };
  states {WaitCell, Receive};
  perform;
};
Destination::perform {
    state WaitCell:
      IPort->wait (EOT, Receive);
      // This  time  we  are awaiting the EOT event (EOT means End Of Transfer)
      // which is triggered when the last bit of a cell arrives at the station.
      // At this moment we can assume that the cell has been received.
    state Receive:
      // Here  we  are  playing  a simple trick. The receive operation requires
      // that the standard Receiver attribute of the cell is equal to the Id of
      // the station owning this process. In other words, it is only  legal  to
      // receive  packets  that  are addressed to the current station. However,
      // for  some  signaling  cells,  the  Receiver  attribute  may   be   set
      // incorrectly.  As  we  will  shortly  see, we don't care much about the
      // Receiver attribute of a control  cell.  To  avoid  complaints  of  the
      // simulator,  we force the Receiver attribute to be correct, if the cell
      // happens to be special (Type != DTC). Data cells  have  their  Receiver
      // attributes set correctly.
      if (TheCell->Type != DTC) TheCell->Receiver = (int) MySId;
      Client->receive (TheCell, ThePort);
      // The  receive  operation  marks  the moment when the cell is completely
      // received at the destination. This operation can be  perceived  by  the
      // traffic generator (see below).
      skipto WaitCell;
};

// This ends the generic portion of the protocol. Now we will define a specific
// instance  of  a  switch  with  a  specific  buffering  policy  and  a sample
// connection-setup algorithm. Intentionally, the portion  defined  so  far  is
// fixed  and  independent  of  the switch type. The fragments below refer to a
// specific switch and may have to be adjusted to suit the customer's taste.

#define CR_PENDING        0
#define CR_ESTABLISHED    1
#define CR_CLEARING       2
   // The  constants  above  are  used  to  describe the status of a connection
   // request -- as perceived by the switch.

process ReSender;     // Just announcing the type

class CReq {
     // This  data  structure  describes a connection request. This description
     // consists of the bandwidth  requirements  (Bandwidth),  the  global  VCI
     // number, the pair of port indexes, and the status. Additionally, we need
     // a  process  pointer  which  occasionally points to the ReSender process
     // associated with the connection request. The role of this process is  to
     // periodically re-issue the request up the link after a timeout.
  public:
  Long Bandwidth;
  int   VCI,
     IPIndx,          // Input port index
     OPIndx;          // Output port index
  int  Status;
  ReSender *RS;
  CReq *next;
  inline CReq (int vc, Long bd, int ip, int op, ReSender *r) {
    // This  is  the  standard C++ constructor for the connection request. When
    // the request description is originally created, its status is  marked  as
    // PENDING (the request is just being issued).
    Bandwidth = bd; VCI = vc; IPIndx = ip; OPIndx = op; RS = r;
    Status = CR_PENDING;
  };
};

class CReqPool {
     // This  data  structure represents the pool of all outstanding connection
     // requests at one switch. Note that an  active  (ESTABLISHED)  connection
     // has  also  a description in the pool. The pool is organized as a simple
     // list anchored at Head. Connections are set up  and  cleared  relatively
     // seldom  (compared  to  cell  transmissions) and the efficiency of their
     // representation is not critical.
  CReq *Head;
  public:
  CReqPool () { Head = NULL; };  // The standard constructor
  inline void store (int vc, Long bd, int ip, int op, ReSender *r) {
    // This  method  stores  a  new connection description in the pool. Nothing
    // tricky here.
    CReq *cr = new CReq (vc, bd, ip, op, r);
    cr->next = Head;
    Head = cr;
  };
  inline CReq *find (int vc, int op) {
    // This  method  finds  a connection description that matches the specified
    // VCI number and the output port index.
    for (CReq *cr = Head; cr != NULL; cr = cr->next)
      if (cr->VCI == vc && cr->OPIndx == op) return cr;
    return NULL;
  };
  inline CReq *pending (int vc, int ip) {
    // This  method  finds  a "pending" connection description that matches the
    // specified VCI number and the input port index. The  connection  must  be
    // either PENDING or ESTABLISHED to be selected.
    for (CReq *cr = Head; cr != NULL; cr = cr->next)
      if (cr->VCI == vc && cr->IPIndx == ip &&
	 cr->Status != CR_CLEARING) return cr;
    return NULL;
  };
  void purge (int, int);
};
void CReqPool::purge (int vc, int op) {
    // This  method removes the indicated connection description from the pool,
    // based on its VCI number and the output port index.
    CReq *cr, *cq;
    for (cr = Head, cq = NULL; cr != NULL; cq = cr, cr = cr->next)
      if (cr->VCI == vc && cr->OPIndx == op) {
	if (cq == NULL)
	  Head = cr->next;
	else
	  cq->next = cr->next;
	delete cr;
	return;
      }
    excptn ("CReqPool->purge: request description not found");
};

class FifoItem : public CBuf {
     // Here  we have a data structure that represents a cell buffer. Note that
     // our cell buffer is derived from CBuf (see above).  In  particular,  the
     // new free method redefines the virtual method from the parent type.
  friend class Fifo;
  TIME DepoTime;
	// This gives the time when a cell was deposited in the buffer.
  FifoItem *next;
  inline FifoItem (Cell *cl) {
    // This  is the standard constructor called when a cell is deposited in the
    // buffer. In fact, the buffer is "created" when a cell is  deposited  into
    // it and "destroyed" when the cell is removed.
    load (cl);
    DepoTime = Time;
    next = NULL;
  };
  void free () { delete this; };
};

class Fifo {
     // This  represents  a  buffer  pool  organized  into a FIFO list of cells
     // reflecting their arrival order. The Head attribute points to the  first
     // cell buffer in the pool and the Tail attribute gives fast access to the
     // end of the list.
  FifoItem *Head,
	   *Tail;
  public:
  Fifo () { Head = Tail = NULL; };
	// The  standard  constructor  called  when  the  pool  is  created. It
	// initializes the pointers as for an empty pool.
  inline CBuf *head (TIME &dt) {
    // This  method  gives access to the first cell buffer in the pool. It also
    // returns (via the argument) the time when the buffer was filled.
    if (Head) {
      dt = Head->DepoTime;
      return Head;
    } else {
      dt = TIME_inf;
      return NULL;
    }
  };
  inline void remove () {
    // This  method  removes from the pool the first cell buffer. Note that the
    // memory used by the buffer is not deallocated at this moment.
    Head = Head->next;
  };
  inline Boolean purge () {
    // Here we actually remove and discard the first buffer from the pool. This
    // operation  corresponds  to  dropping  a cell when the pool becomes full.
    // Note that we don't say anything here about the  capacity  of  the  pool.
    // This will be taken care of later.
    if (Head) {
      FifoItem *th;
      if ((Head = (th = Head)->next) == NULL) Tail = NULL;
      delete th; // Deallocate memory
      return YES;
    } else
      return NO;
  };
  inline void put (Cell *cl) {
    // This  method stores a new cell in the pool. Note that the cell is stored
    // at the end of the pool.
    FifoItem *th;
    th = new FifoItem (cl);
    if (Head) {
      Tail->next = th;
      Tail = th;
    } else
      Tail = Head = th;
  };
};

class OutputBuffer {
     // Now we can see that the actual buffer consists of a pair of cell pools.
     // We  want  to keep red and green cells in separate pools so that when we
     // have to drop the oldest red cell we can  locate  it  fast.  The  buffer
     // capacity  (specified when the buffer is created) is defined for the Red
     // and Green pools combined. Attribute Fill gives the  current  number  of
     // cells stored in both pools.
  public:
  Fifo Red, Green;
  int Capacity, Fill;
  OutputBuffer (int bc) { Capacity = bc; Fill = 0; }
};

station ASwitch : Switch {
     // This  is  our  switch.  As far as the buffering policy is concerned, we
     // have a separate buffer (with a definite fixed capacity) associated with
     // every output port. The buffers are simply indexed by port numbers. Each
     // buffer is an object of type OutputBuffer, i.e., it consists of two cell
     // pools.
  OutputBuffer **Buffers;
  int *RouteSize;
	// This   array   gives   the  sizes  of  the  routing  tables  on  the
	// per-output-port basis.
  Long **Routes,
	// These  are the actual routing tables. A routing table is an array of
	// destination Ids reachable via the given output port. It is legal for
	// the same destination to appear in several routing  tables.  In  such
	// case,  the  switch  offers  multiple  alternative  paths to the same
	// destination. When the connection is set up,  however,  only  one  of
	// these paths must be decided upon.
       *Used,
       *Bandwidth;
	// Used  and Bandwidth are two arrays indexed by output ports that give
	// the "used" and "total" bandwidth of every port  in  some  (abstract)
	// units.  The  idea  is that every connection uses some portion of the
	// port bandwidth. This portion is specified with the  connection-setup
	// request.  To  be  admitted,  the  connection  must  not  exceed  the
	// available bandwidth of the port, i.e., its  bandwidth  must  not  be
	// greater than Bandwidth[port]-Used[port].
  CReqPool *Connections;
	// This  is  the  connection  pool  describing  all connection requests
	// currently being processed by the switch  (or  simply  being  active,
	// i.e., ESTABLISHED).
  TIME CTimeout;
	// This  is  the  connection  timeout. Any request for which the switch
	// expects a response will be reissued every CTimeout time units, until
	// the switch eventually gets the response.
  void setup (int np) {
    // In  this  setup method we create the (empty) connection pool and all the
    // arrays declared at the switch, but we don't fill these arrays. This will
    // be taken  care  of  by  special  functions  that  will  assign  specific
    // parameters to individual switches and also set up the routing tables.
    Connections = new CReqPool;
    Buffers = new OutputBuffer* [np];
    RouteSize = new int [np]; // Create room for routing tables
    Routes = new Long* [np];
    Used = new Long [np];
    Bandwidth = new Long [np];
    for (int i = 0; i < np; i++) Used [i] = 0;
    Switch::setup (np);
  };
  void csetup (Cell*, int);
	// The connection setup function: just announced here and defined below
  int admit (Cell*);
	// The call-setup admission function (defined below)
  void cancel (Cell*, int);
	// The  call  cancellation  function.  Releases  the  switch  resources
	// occupied by the connection.
  void enter (Cell *cl, int BIndx) {
    // This  is  the switch's version of enter -- the function called for every
    // incoming cell to store it into the buffer.
    OutputBuffer *Buf;
    Buf = Buffers [BIndx];    // The output buffer where the cell should go
    if (Buf->Fill == Buf->Capacity) {
      // The  buffer  is full so we have to drop a cell. First we try to drop a
      // red cell and, if it turns out that no red cell is  available,  we  are
      // forced to drop a green one.
      if (Buf->Red . purge () == NO)
	// No red cell -- drop a green one
	Buf->Green . purge ();
      (Buf->Fill) --;
    }
    if (flagSet (cl->Attributes, DROP))
      // Now, based on whether the cell is red (DROP) or green, it is stored in
      // the appropriate pool
      Buf->Red . put (cl);
    else
      Buf->Green . put (cl);
    // If  the  buffer  has  changed its status from empty to nonempty, we must
    // generate a mailbox event to wake up the Output process possibly  waiting
    // for  a  cell.  This  is accomplished by depositing a dummy item into the
    // mailbox associated with the buffer/port.
    if ((Buf->Fill) ++ == 0) Arrivals [BIndx] -> put ();
  };
  CBuf *acquire (int pi) {
    // This  method  is  called  by  the  Output  process to acquire a cell for
    // transmission
    CBuf *rc, *gc;
    TIME  rt,  gt;
    OutputBuffer *Buf;
    Buf = Buffers [pi];
    gc = Buf -> Green . head (gt);
    rc = Buf -> Red   . head (rt);
    // We  have  a  look  at two front cells in the two pools and compare their
    // arrival times. If both pools are nonempty, we select the cell  with  the
    // earlier arrival time.
    if (gc && (gt < rt || (gt == rt) && flip ())) {
      // If both cells arrived at exactly the same time, we flip a coin
      Buf->Green . remove ();
      (Buf->Fill) --;
      return gc;
    } else if (rc) {
      Buf->Red . remove ();
      (Buf->Fill) --;
      return rc;
    } else
      return NULL;
  };
};

process ReSender (ASwitch) {
     // This  simple  process  is  used to keep on re-sending a request until a
     // response eventually arrives.
  int OPIndx;
  CBuf SCBuf;
  TIME Timeout;
  void setup (Cell *c, int op, TIME tm) {
    // When  the  process  is created, we specify three items: the control cell
    // containing the request, the output port on which the request  should  be
    // issued, and the timeout.
    SCBuf . load (c);
    OPIndx = op;
    Timeout = tm;
  };
  states {Send, Sleep};
  perform {
    state Send:
      // The  process makes its private copy of the control cell and submits it
      // periodically (every Timeout time units) to the buffer associated  with
      // the output port. The response will be recognized by csetup (see below)
      // which  will  kill  the  process upon its arrival. Note: we assume that
      // control cells are subjected to the same buffering policies as  regular
      // data cells.
      S->enter (SCBuf . cell (), OPIndx);
      Timer->wait (Timeout, Send);
  };
};

int ASwitch::admit (Cell *sc) {
     // This  method  determines  whether a call can be admitted locally by the
     // switch based on its current configuration of  available  resources.  If
     // the answer is affirmative, the method returns the suggested output port
     // index for the connection. Otherwise, it returns NONE (-1).
  int i, j, op;
  Long ub, tb;
  for (ub = 0, i = 0; i < NPorts; i++)
    // Here  we go through all the output ports trying to find one which offers
    // a route to the destination  (sc->Receiver).  For  every  such  port,  we
    // determine  its  bandwidth  available and finally we select the port with
    // the maximum unused bandwidth. This way the switch tries to  balance  its
    // load over all output ports.
    if (Connections->find (sc->VCI, i) == NULL && 
	// If  the  connection  is  already present, it means that we have just
	// tried  this  route  and  failed  (the  switch   is   CLEARING   that
	// connection), so we shouldn't consider the same port again.
		  (tb = Bandwidth [i] - Used [i]) > ub)
      for (j = 0; j < RouteSize [i]; j++)
       if (Routes [i] [j] == sc->Receiver) {
	 ub = tb;
	 op = i;
	 break;
       }
  if (ub < sc->SeqNum)
    // The maximum bandwidth available is insufficient to accept the connection
    return NONE;
  else {
    Used [op] += sc->SeqNum;
    // Connection accepted -- reserve bandwidth
    return op;
  }
};

void ASwitch::cancel (Cell *sc, int ip) {
     // This  method  is called to close a connection in terms of releasing the
     // resources occupied by it
  CReq *cr;
  cr = Connections->pending (sc->VCI, ip);
  assert (cr, "cancel: connection not found");
  // The bandwidth is taken from the control cell
  Used [cr->OPIndx] -= sc->SeqNum;
};

void ASwitch::csetup (Cell *sc, int ip) {
     // This  method  completely  describes  the  connection-setup protocol. It
     // specifies what control cells  should  be  sent  in  response  to  other
     // control  cells. The method is called with a control cell pointer passed
     // as the first argument and the input port index  passed  as  the  second
     // argument.  The  port  index  refers  to  the port on which the cell has
     // arrived.
  int op;
  ReSender *rs;
  CReq *cr;
  switch (sc->Type) {
    case CST:
      // Connection-setup cell.
      if (cr = Connections->pending (sc->VCI, ip)) {
	// Already pending or established
	if (cr->Status == CR_ESTABLISHED) {
	  // Connection  already  established,  but  the originator seems to be
	  // unsure  (timeout,  lost  confirmation).  Thus  we   send   another
	  // confirmation.  Note  that  the  confirmation goes upstream, so the
	  // port index for enter is that of the input port.
	  sc->Type = ACK;
	  enter (sc, ip);
	}
	// The  connection is pending. We ignore the new setup request. We will
	// send a confirmation as soon as we receive it from the next switch up
	// the link.
      } else if ((op = admit (sc)) != NONE) {
	// A  new  connection  which is accepted locally. We will propagate the
	// request up the link. This will be taken care of  by  ReSender  which
	// will keep on re-sending the request until we get a response.
	rs = create ReSender (sc, op, CTimeout);
	// Store the connection description
	Connections->store (sc->VCI, sc->SeqNum, ip, op, rs);
      } else {
	// A  new connection which is rejected locally. We just send a negative
	// acknowledgement down the link.
	sc->Type = CNK;
	enter (sc, ip);
      }
      return;
    case ACK:
      // Positive acknowledgement
      cr = Connections->find (sc->VCI, ip);
      assert (cr, "csetup: ACK for an unknown connection request");
      if (cr->Status == CR_PENDING) {
	// The  connection  must  be  PENDING  for a positive ACK to make sense
	// (otherwise it is deemed redundant and  ignored).  We  terminate  the
	// ReSender  process, mark the connection as ESTABLISHED and set up the
	// VCI table.
	cr->RS->terminate ();
	cr->Status = CR_ESTABLISHED;
	enter (sc, cr->IPIndx);
	// Allocate VCI slot
	VCITable [cr->VCI][0] = cr->OPIndx;
	VCITable [cr->VCI][1] = cr->IPIndx;
      }
      return;
    case CNK:
      // Negative acknowledgement
      cr = Connections->find (sc->VCI, ip);
      assert (cr, "csetup: NAK for an unknown connection request");
      if (cr->Status == CR_PENDING) {
	// Again,  an acknowledgement only makes sense for a PENDING connection
	// request, so it is ignored if the status is  anything  else  (it  can
	// only  be CLEARING). We release the resources held by the connection,
	// terminate the ReSender process and (to confuse the competition) send
	// a disconnect request (DSC) up the link. Why do we do that? OK. It is
	// possible that while  we  have  been  waiting  to  receive  the  CNK,
	// ReSender  has reissued the connection request (possibly a few times)
	// up the link. It is also possible that one of those multiple requests
	// will be accepted although its predecessor  was  rejected.  With  the
	// disconnect  request  we  want  to  make  sure that the connection is
	// cleared for good. We will know that when  we  receive  a  disconnect
	// acknowledgement.  Until then we will keep on reissuing DSC (with the
	// help of another copy of ReSender).
	cancel (sc, cr->IPIndx);
	cr->RS->terminate ();
	cr->Status = CR_CLEARING;
	sc->Type = DSC;
	cr->RS = create ReSender (sc, ip, CTimeout);
	// Try alternative path
	if ((op = admit (sc)) != NONE) {
	  // We  have managed to find an alternative path. We send CST via that
	  // path and store a new pending connection description. At  the  same
	  // time  we  set  IPIndx of the previous description to NONE to avoid
	  // propagating the disconnect acknowledgement (when it arrives)  down
	  // to the source.
	  sc->Type = CST;
	  rs = create ReSender (sc, op, CTimeout);
	  Connections->store (sc->VCI, sc->SeqNum, cr->IPIndx, op, rs);
	  cr->IPIndx = NONE;   // Detach from source
	} else {
	  // No  alternative  path  -- send a negative acknowledgement down the
	  // link
	  sc->Type = CNK;
	  enter (sc, cr->IPIndx);
	}
      }
      return;
    case DAK:
      // Disconnect acknowledgement
      if ((cr = Connections->find (sc->VCI, ip)) != NULL && cr->Status ==
	CR_CLEARING) {
	// This  only  makes  sense  if we are CLEARING the connection. In such
	// case we terminate the ReSender (that has been  sending  DSC  up  the
	// link)  and  propagate  the  DAK  down  the  link,  but  only  if the
	// connection did not succeed via an alternative path  (in  which  case
	// cr->IPIndx is NONE).
	cr->RS->terminate ();
	if (cr->IPIndx != NONE) enter (sc, cr->IPIndx);
	Connections->purge (sc->VCI, ip);
      }
      return;
    case DSC:
      // Disconnect request
      if (cr = Connections->pending (sc->VCI, ip)) {
	// The  connection  must  be  PENDING  or  ESTABLISHED.  We  start from
	// releasing the resources. If the connection  is  PENDING,  DSC  means
	// that  the  source  has  changed  its  mind,  or  we  are  clearing a
	// connection that was first rejected and then accepted.
	cancel (sc, ip);
	if (cr->Status == CR_PENDING)
	  // ReSender present -- terminate it
	  cr->RS->terminate ();
	else
	  // Remove the VCI from the table
	  VCITable [cr->VCI][0] = VCITable [cr->VCI][1] = NONE;
	cr->Status = CR_CLEARING;
	// Propagate DSC request uplink
	cr->RS = create ReSender (sc, cr->OPIndx, CTimeout);
      } else {
	// No such connection -- send immediate disconnect acknowledgement
	sc->Type = DAK;
	enter (sc, ip);
      }
      return;
  }
};

// ################################################################### //

// Here  we  are  going  to  build a sample network configuration. In SMURPH, a
// network is built dynamically by calling functions and  methods  that  create
// its  components and interconnect them. This is very convenient if we want to
// investigate regular configurations (like  in  academic  research)  and  less
// convenient  if  we  have  to  specify a real configuration with an irregular
// geometry. Well, somebody must  provide  all  these  numbers  anyway,  so  we
// shouldn't  really  complain about SMURPH here. This is a general problem. To
// make things easy, we will build a nice regular configuration which has  very
// few parameters (so that our data set will be simple).

typedef int (*CFType) (Long, Long, DISTANCE&, RATE&);

// Below  we  define a function that will connect all nodes in our network (the
// switches as well as the end-nodes) according to the prescription supplied by
// the user. This prescription comes as  a  function  whose  type  is  declared
// above. This function accepts two numbers, indicating a pair of nodes, as the
// first  two  arguments  and  it  returns the number of bidirectional channels
// between the nodes (note that in principle two ATM switches can be  connected
// via  more  than  one  channel).  Thus,  if  the nodes are not to be directly
// connected, the function is supposed to return zero. If the two nodes are  to
// be  connected,  the  function  returns  via  the  last  two  parameters  the
// propagation length of the links and their transmission rate (multiple  links
// between  the  same pair of nodes are assumed to have the same length and the
// same transmission rate).

void makeATMGrid (CFType Connectivity) {
     // This  function builds the ports and links required to connect the nodes
     // of our network. Note that makeATMGrid is general, in the sense that  it
     // doesn't  assume any specific topology. The topology is described by the
     // incidence function passed as  the  argument  to  makeATMGrid  (see  the
     // comment above).
  Long i, j, nc;
  int p1, p2;
  RATE tr;
  ATMNode *S1, *S2;
  DISTANCE lg;
  PLink *lk1, *lk2;
  Port *IP1, *IP2, *OP1, *OP2;
  for (i = 0; i < NStations-1; i++) {
    S1 = (ATMNode*) idToStation (i);
    for (j = i+1; j < NStations; j++) {
      // We  are  looking  here  at  every pair of nodes <i,j> such that i < j.
      // Function Connectivity returns the  number  of  channels  (link  pairs)
      // between these nodes. Note that all channels in ATM are bidirectional.
      if (nc = Connectivity (i, j, lg, tr)) {
	// There is at least one link from i to j
	S2 = (ATMNode*) idToStation (j);
	// Locate the first free port in each node. Connections are always made
	// in pairs: from S1 to S2 and from S2 to S1. Thus, the first free port
	// index  in  S1  (or S2) should be the same for the input port and the
	// output port.
	for (p1 = 0; p1 < S1->NPorts && S1->OPorts [p1] != NULL; p1++);
	for (p2 = 0; p2 < S2->NPorts && S2->OPorts [p2] != NULL; p2++);
	while (nc--) {
	  Assert (p1 < S1->NPorts, "makeATMGrid: S1 out of ports");
	  Assert (p2 < S2->NPorts, "makeATMGrid: S2 out of ports");
	  Assert (S1->IPorts [p1] == NULL && S1->OPorts [p1] == NULL,
	    "makeATMGrid: S1 inconsistent port allocation");
	  Assert (S2->IPorts [p2] == NULL && S2->OPorts [p2] == NULL,
	    "makeATMGrid: S2 inconsistent port allocation");
	  // Now we create the two links that make the connection. The argument
	  // of  the  create  operation  (the  setup  argument)  for  PLink  (a
	  // unidirectional point-to-point channel) gives the number  of  ports
	  // to be connected to the link. In our case, every link has two ports
	  // one port at each end.
	  lk1 = create PLink (2);
	  lk2 = create PLink (2);
	  // Now  we  create the ports at the nodes to be connected. The output
	  // ports are assigned the transmission rate returned by  Connectivity
	  // for this connection. The input ports need no transmission rates as
	  // nothing will ever be transmitted into them.
	  OP1 = S1->OPorts [p1] = create (i) Port (tr);
	  IP1 = S1->IPorts [p1] = create (i) Port;
	  OP2 = S2->OPorts [p2] = create (j) Port (tr);
	  IP2 = S2->IPorts [p2] = create (j) Port;
	  // The ports must be attached to their links. This is accomplished by
	  // the connect operation. For a unidirectional link (type PLink), the
	  // order  in  which  ports  are  connected to the link determines the
	  // transfer direction: from the first  connected  port  to  the  last
	  // connected port.
	  OP1 -> connect (lk1);
	  IP2 -> connect (lk1);
	  OP2 -> connect (lk2);
	  IP1 -> connect (lk2);
	  // Finally,  we have to assign a length to the connection. Generally,
	  // this is done in SMURPH by specifying distances  between  pairs  of
	  // ports  connected to the link. Here we only have two ports per link
	  // which makes this operation very simple. Operation setDTo sets  the
	  // propagation distance from "this" port to the port specified as the
	  // argument.  The  distance  (lg)  is  expressed in time units (ITUs)
	  // needed for a signal to travel between the ports.
	  OP1 -> setDTo (IP2, lg);
	  OP2 -> setDTo (IP1, lg);
	  // We  also fill the proper entries in the Neighbors arrays. This way
	  // the nodes will know where they are connected.
	  S1 -> Neighbors [p1] = j;
	  S2 -> Neighbors [p2] = i;
	}
      }
    }
  }
  // Just  to  make  sure, we will now check whether all stations and all ports
  // have been connected to something.
  for (i = 0; i < NStations; i++) {
    S1 = (ATMNode*) idToStation (i);
    for (p1 = 0; p1 < S1->NPorts; p1++) {
      Assert (S1->IPorts [p1] != NULL,
	form ("Node %1d, input port %1d left dangling", i, p1));
      Assert (S1->OPorts [p1] != NULL,
	form ("Node %1d, output port %1d left dangling", i, p1));
    }
  }
};

// ################################################################### //

int Connected (Long, Long, DISTANCE&, RATE&);       // Just announcing
void setNodeParameters ();

DISTANCE LinkLength;
RATE TransferRate;
   // In  our  sample  network  that we define below, all links are of the same
   // length and they all have the same transmission rate. These parameters are
   // stored in the above variables.

Long SideLength, NSwitches, NEndNodes;
   // The  network  consists of a square grid of switches, each switch equipped
   // with four input and four output ports. SideLength  gives  the  number  of
   // switches   on   one   side   of  the  grid.  NSwitches  will  be  set  to
   // SideLength*SideLength which gives the total number  of  switches  in  the
   // network. Each of the 4*SideLength switches along the sides of the grid is
   // connected  to  one  end-node.  Each  end-node  has only one pair of ports
   // interfacing it to one switch. The total number of  end-nodes  (NEndNodes)
   // is  equal  to 4*SideLength. All links are of the same length (LinkLength)
   // and they all have the same transmission rate (TransferRate).

void makeNetwork () {
     // This function is called to read the numerical parameters describing the
     // network, build all its components and connect them.
  Long i;
  double r;
  // Here  we  set  the  so-called  "experimenter  time  unit" (ETU) which is a
  // multiple of the "indivisible time unit" used internally by the  simulator.
  // The  ETU will be used to specify time-related input data. We assume that 1
  // ETU corresponds to one second or real time (see the input data set).
  readIn (r);
  setEtu (r);
  // Next  we set the clock tolerance. SMURPH accounts for the limited accuracy
  // of independent clocks. All time delays (including  the  cell  transmission
  // time) will be measured with the specified tolerance.
  readIn (r);
  setTolerance (r);
  // Now we determine the network size (see above) ...
  readIn (SideLength);
  NSwitches = SideLength * SideLength;
  NEndNodes = 4 * SideLength;
  // And the link length and its transfer rate
  readIn (LinkLength);
  readIn (TransferRate); 
  // The following two for loops create the stations. As we remember, the setup
  // method  of  ASwitch  accepts  one  argument which gives the number of port
  // pairs of the switch. All our switches are 4x4.
  for (i = 0; i < NEndNodes; i++)
    create EndNode;
  for (i = 0; i < NSwitches; i++)
    create ASwitch (4);
  // Now we make all the connections ..
  makeATMGrid (Connected);
  // And  set  the  numerical  parameters of individual nodes (timeouts, buffer
  // sizes, etc. -- see below).
  setNodeParameters ();
};

void buildRoutingTables ();            // Just announcing

void setNodeParameters () {
     // This function sets the numerical parameters of each individual node and
     // also   builds   the   routing   tables  at  the  switches  (by  calling
     // buildRoutingTables)
  TIME CTimeout;
  double r;
  Long BandwidthPerPort, n;
  int i, BufferCapacity;
  EndNode *E;
  ASwitch *S;

  readIn (r);
  // Here  we  read  the  connection  timeout  (the  same for all end-nodes and
  // switches) and convert it to ITUs (we assume that the  user  specifies  the
  // timeout  in ETUs, i.e., seconds). The global variable Etu tells the number
  // of ITUs in one ETU.
  CTimeout = (TIME) (r * Etu);
  // All  output  ports of a switch have the same number of bandwidth units and
  // the same buffer capacity.  The  bandwidth  units  are  used  to  determine
  // whether  an  incoming  connection request should be accepted by the switch
  // (see the code of admit).
  readIn (BandwidthPerPort);
  readIn (BufferCapacity);

  for (n = 0; n < NEndNodes; n++) {
    // There  is  just one numerical parameter of an end-node -- the connection
    // timeout. Please note that there is nothing in the structure  of  EndNode
    // that  requires  all  nodes to use the same timeout value. We make it the
    // same for convenience.
    E = (EndNode*) idToStation (n);
    E->CTimeout = CTimeout;
  }
  for (n = NEndNodes; n < NStations; n++) {
    // This  loop  goes  through  all switches. Stations in SMURPH are assigned
    // numerical Id's starting from zero and reflecting the order in which  the
    // stations  have  been created (see makeNetwork). Thus, the Id's from 0 to
    // NEndNodes-1  identify  end-nodes  and  the  numbers  from  NEndNodes  to
    // NStations-1  refer  to switches. NStations is a standard global variable
    // that tell the number of all stations that  have  been  created  so  far.
    // Given  a station number, idToStation converts this number into a pointer
    // to the station structure (an object of type Station).
    S = (ASwitch*) idToStation (n);
    S->CTimeout = CTimeout;
    for (i = 0; i < S->NPorts; i++) {
      S->Buffers [i] = new OutputBuffer (BufferCapacity);
      S->Bandwidth [i] = BandwidthPerPort;
    }
  }
  // The routing tables at the switches are taken care of by a special function
  buildRoutingTables ();
};
  
int Connected (Long a, Long b, DISTANCE &l, RATE &r) {
     // This  function  describes  dynamically  the  incidence  matrix  of  our
     // network. It is called for all pairs of stations <a,b> such that a  <  b
     // and  is  supposed  to  return  the  number  of  channels  between these
     // stations.  In  our  network,  there  is  at  most  one  (bidirectional)
     // connection  between  a  given  pair  of  nodes, so the function returns
     // either 0 or 1, depending on whether the nodes are connected or not.  It
     // also sets l and r to the (fixed) link length and transmission rate.
  l = LinkLength;
  r = TransferRate;
  if (b < NEndNodes)
    // As  a < b, this means that both nodes are end-nodes. End-nodes are never
    // connected to one another.
    return 0;
  if (a < NEndNodes) {
    // We are connecting an end-node to a switch
    b -= NEndNodes;       // Turn it into a switch number starting from zero
    if (a < NEndNodes/4)
      // End-nodes  in the upper row (numbered 0 -- SideLength-1) are connected
      // to the switches numbered NEndNodes-1 -- NEndNodes+SideLength-1,  or  0
      // -- SideLength-1 in "transformed coordinates". The switches in the grid
      // are numbered by rows starting from the upmost row, from left to right.
      return a == b;
    else if (a < NEndNodes/2)
      // This  is  the  right  (vertical)  column of end-nodes connected to the
      // right edge of the grid.
      return (b+1) % SideLength == 0 &&
	a - SideLength == (b + 1) / SideLength - 1;
    else if (a < NEndNodes - SideLength)
      // The bottom row of end-nodes
      return b == NSwitches - (a - SideLength - SideLength) - 1;
    else
      // And the last left vertical column of end-nodes
      return b % SideLength == 0 && SideLength * 4 - a - 1 == b / SideLength;
  } else {
    // We get here to determine whether a pair of switches is connected or not.
    // Two switches are connected if they are neighbors in the grid which means
    // that  they  belong  to the same row [(a/SideLength)==(b/SideLength)] and
    // the larger switch number (b) is equal to the smaller number + 1, or they
    // belong to the same column [(a%SideLength)==(b%SideLength)] and  the  row
    // number of the switch with the higher Id [(b/SideLength)] is equal to the
    // row number of the switch with the lower Id [(a/SideLength)] + 1.
    a -= NEndNodes;
    b -= NEndNodes;
    return (a / SideLength) == (b / SideLength) && b == a + 1   ||
	   (a % SideLength) == (b % SideLength) && (b / SideLength) ==
	     (a / SideLength) + 1;
  }
};

void buildRoutingTables () {
    // This  function  builds  the routing tables at all switches (see the type
    // declaration of ASwitch). Each switch maintains a  two-dimensional  array
    // called Routes which for every port gives the list of end-nodes reachable
    // from  the  switch  via  this  port.  This list is built by examining the
    // length of the shortest path (in terms of the number of  hops)  from  the
    // switch  to every end-node. An end-node n is included in the routing list
    // for port p, if p offers the shortest path to n amongst  all  the  output
    // ports  of the switch. If several ports offer the same shortest path, the
    // end-node occurs on the lists of all  those  ports.  In  such  case,  the
    // switch offers multiple (alternative) routes to the node.
  short **IM,   // Mesh graph incidence matrix
	t1, t2, t;
  Long i, j, k, nr, p, *dest;
  ATMNode *M;
  ASwitch *S;
  // We  start  from  building  the  square  matrix  giving distances (in hops)
  // between all pairs of directly connected nodes. Clearly, this distance is 1
  // if the nodes are directly connected and 0 otherwise.
  IM = new short* [NStations];
  dest = new Long [NEndNodes];
  for (i = 0; i < NStations; i++) IM [i] = new short [NStations];
  for (i = 0; i < NStations; i++) {
    for (j = 0; j < NStations; j++)
      IM [i][j] = (i == j) ? 0 : MAX_short;
    M = (ATMNode*) idToStation (i);
    for (j = 0; j < M->NPorts; j++)
      IM [i][M->Neighbors [j]] = 1;
  }
  // Below is the well-known Floyd's algorithm for all-pairs shortest paths. We
  // calculate  the  shortest  paths between all pairs of nodes in our network,
  // including end-nodes as well as switches.
  for (k = 0; k < NStations; k++)
    for (i = 0; i < NStations; i++) {
      if ((t1 = IM [i][k]) < MAX_short)
	for (j = 0; j < NStations; j++) 
	  if ((t2 = IM [k][j]) < MAX_short && (t = t1 + t2) < IM [i][j])
	    IM [i][j] = t;
    }
  // Floyd's algorithm ends here
  for (i = NEndNodes; i < NStations; i++) {
    // This  loop  goes  through  all  switches. Note that end-nodes don't have
    // routing tables.
    S = (ASwitch*) idToStation (i);
    for (j = 0; j < S->NPorts; j++) {
      // This loop goes through all ports of the switch
      if (S->Neighbors [j] < NEndNodes) {
	// This port connects the switch to an end-node. Its routing table will
	// consist of a single entry identifying the end-node. We don't want to
	// relay to the end-node anything that is not addressed there.
	S->RouteSize [j] = 1;
	S->Routes [j] = new Long [1];
	S->Routes [j][0] = S->Neighbors [j];
      } else {
	// Here  we  have a port that takes us to another switch. We go through
	// all ports and for each port we look at the shortest  path  via  that
	// port  to  every  single  end-node. Then we select those end-nodes to
	// which the shorest route from our switch is via  port  number  j  and
	// store their Id's in dest.
	for (k = nr = 0; k < NEndNodes; k++) {
	  t1 = IM [S->Neighbors [j]][k];
	  for (p = 0; p < S->NPorts; p++)
	    if (IM [S->Neighbors [p]][k] < t1) break;
	  if (p >= S->NPorts)
	    // Add this route
	    dest [nr++] = k;
	}
	// At  this  moment  we  know  the size of the routing table, so we can
	// create it. Then we copy dest to the table.
	S->RouteSize [j] = (int) nr;
	S->Routes [j] = new Long [nr];
	for (k = 0; k < nr; k++) S->Routes [j][k] = dest [k];
      }
    }
  }
/*@@@
  // Uncomment this if you want the routing tables printed
  for (i = NEndNodes; i < NStations; i++) {
    S = (ASwitch*) idToStation (i);
    trace ("Routing tables at switch: %1d (%1d)", i, i - NEndNodes);
    for (j = 0; j < S->NPorts; j++) {
      trace ("Port %1d:", j);
      nr = S->RouteSize [j];
      for (k = 0; k < nr; k++) trace ("  --> %1d", S->Routes [j][k]);
    }
  }
@@@*/
  // Done,  now  we  deallocate  the temporary arrays which won't be needed any
  // more
  for (i = 0; i < NStations; i++)
    delete IM [i];
  delete IM;
  // And the scratch route array
  delete dest;
};

void startProtocol () {
     // This  function  starts  the  protocol processes running at switches and
     // end-nodes.  Each  end-node  runs  two  processes:  the  Source  process
     // responsible  for  inserting  traffic in the network and the Destination
     // process absorbing the cells addressed to the station. Each switch  runs
     // one  process for every port: one Input process for every input port and
     // one Output process for every output port.
  int i, j;
  for (i = 0; i < NEndNodes; i++) {
    // The  argument  of  create preceding the process type gives the number of
    // the station to which the created process is supposed to belong.
    create (i) Source;
    create (i) Destination;
  }
  for (i = (int) NEndNodes; i < NStations; i++)
    for (j = 0; j < ((ASwitch*) idToStation (i))->NPorts; j++) {
      create (i) Input (j);
      create (i) Output (j);
    }
};

// ################################################################### //

// In  this  section we define traffic conditions in our network. There are two
// simple traffic patterns: one  representing  file  transfers  and  the  other
// modeling  a  single  VBR  video  session.  In  SMURPH,  a traffic pattern is
// described by an object of type Traffic (or its  derived  subtype)  which  is
// possibly  supported  by one or more "pilot processes" that generate messages
// in a prescribed manner.

process HandShake (EndNode) {
    // This  process  is created at a source end-node to set up a connection to
    // the destination end-node or to cancel an existing connection. As we will
    // see in the code method (listed  somewhere  below),  the  process  issues
    // periodically  (at  timeout  intervals)  a control message representing a
    // request addressed to the  destination.  When  a  response  arrives,  the
    // process terminates itself.
  Long Sender, Receiver;
  int VCI, SCode, ECode, Bandwidth;
  void setup (Long rc, int vc, int sc, int ec, Long bw) {
    // When  the process is created, its creator specifies four parameters: the
    // destination Id (rc), the VCI  number  (for  a  call-setup  request  this
    // number  is  selected  by the originator and it identifies the connection
    // uniquely in the entire network), the control code (the Type attribute of
    // the control message/cell to be issued), the  control  code  expected  to
    // arrive  in  response,  and the number of bandwidth units associated with
    // the connection.
    Sender = S->getId ();
    Receiver = rc;
    VCI = vc;
    SCode = sc;
    ECode = ec;
    Bandwidth = (int) bw;
  };
  states {WaitConn, Reply};
  perform;
};

traffic SignalTP (ATMMessage, Cell) {
     // This  is  a special traffic pattern used for setting up connections. In
     // SMURPH, it makes sense if all packets (cells) belong to  some  "traffic
     // patterns". This way we can separate concerns and simplify the operation
     // of an end-node which only acquires cells from the Client and sends them
     // to  its switch without being concerned about their contents. As we will
     // see in a while, every message generated  by  this  traffic  pattern  is
     // turned  into  exactly  one  cell  representing a signaling request. The
     // names specified in parentheses  identify  the  types  of  messages  and
     // packets handled by the traffic pattern. All our traffic patterns handle
     // messages and cells of the same types.
  HandShake *WCON [MAXCONNECTIONS]; 
    // This array gives pointers to the active instances of HandShake processes
    // for given VCI numbers.
  void setup ();
  void pfmMRC (Cell *c) {
    // This  method is called automatically by SMURPH whenever a cell belonging
    // to this traffic pattern is received at any end-node. More precisely, the
    // method is called whenever the last packet (cell) of a message (which  in
    // principle  could have been turned into multiple packets) is received. In
    // this case, however, every control message is always turned into  exactly
    // one control cell. A cell is assumed to be "received" by an end-node when
    // a  process  running  at the end-node executes "receive" (see the code of
    // Destination) for the cell.
    ATMMessage *m;
    if (c->Type == CST || c->Type == DSC) {
      // This is a call-setup or disconnection request. A response is required.
      // Thus we generate a control message (genMSG is a standard method of the
      // traffic  pattern).  The  message  is  queued  for  transmission at the
      // receiver of the current control cell, is addressed to  the  sender  of
      // the cell, and its length is exactly equal to the payload length of the
      // ATM cell. Thus the message will be turned into exactly one cell.
      m = (ATMMessage*) genMSG (c->Receiver, (int)(c->Sender), PayloadSize);
      // The message is sent over the same VC as the incoming cell
      m->VCI = c->VCI;
      // The  sequence number for a control cell carries bandwidth information.
      // We copy it, as well as the Attributes, from the received control cell.
      m->SeqNum = c->SeqNum;
      m->Attributes = c->Attributes;
      // If  the  type  of  the  incoming  cell is "connection setup" (CST), we
      // respond with an acknowledgement (the connection request  has  made  it
      // through  the  network and we assume that connection requests are never
      // rejected by end-nodes); otherwise the type is "disconnection  request"
      // (DSC) and we respond with DAK (disconnect acknowledgement).
      m->Type = (c->Type == CST) ? ACK : DAK;
    } else {
      // The  cell  represents  a  response.  In  such  case  we  wake  up  the
      // corresponding HandShake process awaiting  this  response  (if  one  is
      // present).  This  is  why  we  need  WCON.  The  process is awakened by
      // receiving the cell type as a signal. Signal passing among processes is
      // one of the possible ways of communicating processes in SMURPH.
      if (WCON [c->VCI]) WCON [c->VCI] -> signal (c->Type);
    }
  }; 
};
void SignalTP::setup () {
    int i;
    // Initially,  WCON  is set to "all NULLs". Whenever a HandShake process is
    // created, it registers itself in WCON by storing a pointer to  itself  in
    // the corresponding entry.
    for (i = 0; i < MAXCONNECTIONS; i++) WCON [i] = NULL;
    // Now  we set up some standard parameters for the traffic pattern. SCL_off
    // says that we want to switch off the "standard Client processing" for the
    // traffic pattern. In other words, we don't want SMURPH  to  generate  any
    // messages  automatically  using its built-in collection of standard pilot
    // processes.  SPF_off  turns  off  collecting  the   standard   (built-in)
    // statistics for the traffic pattern (which wouldn't make much sense).
    Traffic::setup (SCL_off+SPF_off);
};

SignalTP *STP;
   // This  will  point to the actual object representing the signaling traffic
   // pattern, when we create it.

HandShake::perform {
     // This  is the code method of HandShake. After being created, the process
     // starts in state WaitConn.
  state WaitConn:
    ATMMessage *m;
    // The   process   generates  the  request  message  (based  on  the  setup
    // parameters) and queues it at the sender. Note that the  message  belongs
    // to the signaling traffic pattern and consists of a single cell.
    m = (ATMMessage*) (STP->genMSG (Sender, (int) Receiver, PayloadSize));
    m->VCI = VCI;
    m->Type = SCode;
    m->SeqNum = Bandwidth;
    m->Attributes = 0;
    // Now  the process prepares to wait for a signal from the pfmMRC method of
    // the traffic pattern (see above). It calls its own standard method  erase
    // that makes sure the process' signal repository is empty.
    erase ();
    // The process registers itself in WCON ...
    STP->WCON [VCI] = this;
    // And  issues  two  wait  requests: one for the timeout (this is the local
    // attribute of the end-node) and the other to itself  --  for  the  signal
    // indicating  the  arrival of a response. If the timer goes off first, the
    // process will wake up in state WaitConn where it  will  generate  another
    // request  message.  If the signal arrives first, the process will transit
    // to state Reply.
    Timer->wait (S->CTimeout, WaitConn);
    wait (SIGNAL, Reply);
  state Reply:
    // A response has arrived -- the process de-registers from WCON ...
    STP->WCON [VCI] = NULL;
    if ((IPointer)TheSignal == ECode)
      // This  is  the  response  we expected -- the process terminates itself.
      // Note that this event will be perceived by  the  process  that  created
      // HandShake (see below).
      terminate;
    else
      // This is not the response we expected -- keep on re-sending the request
      // message.
      proceed WaitConn;
};
      
Long SequenceNumbers [MAXCONNECTIONS], LostCells [MAXCONNECTIONS];
   // We   use   these  arrays  to  detect  lost  cells  at  the  destinations.
   // SequenceNumbers [vc] gives the expected sequence number of the next  cell
   // to  arrive  over the virtual channel number vc. LostCells [vc] counts how
   // many cells for the virtual channel number vc have been lost so far.

process FPilot (EndNode) {
     // This  is  the  pilot process for the "file transfer" traffic pattern. A
     // copy of this process will run at every end-node  participating  in  the
     // traffic pattern. At this moment we don't have to say which nodes are.
  int VCI;
  Long Sender, Receiver, Length, Retry;
	// The  names  of these attributes should be self-explanatory (they all
	// refer to the current  file  being  transmitted).  Retry  counts  the
	// retransmissions  of  the same file (after a negative acknowledgement
	// or timeout).
  void setup () { Sender = S->getId (); };
  states {Wait, Generate, GenMessage, MessageSent, GotReply, Disconnect};
  perform;
};

traffic FilesTP (ATMMessage, Cell) {
     // This is the traffic pattern representing file transfers.
  TIME ATimeout;
	// Acknowledgement  wait timeout. Having completed a file transmission,
	// the sender will await an  acknowledgement.  If  the  acknowledgement
	// does  not  arrive  before  the  timeout  expires,  the  file will be
	// retransmitted (entirely).
  double Backoff;
	// The   mean   value   of   the   backoff   delay   after  a  negative
	// acknowledgement. If the sender receives a  negative  acknowledgement
	// after  a file transmission (meaning that some cells have been lost),
	// it will wait for a randomized amount of time (with  the  mean  value
	// equal to Backoff) and retransmit the file. Note: the sender does not
	// retransmit  the  file immediately because it suspects congestion and
	// wants to back off for a while to reduce it.
  Long Bandwidth;
	// This represents the number of bandwidth units associated with a file
	// transfer.  We assume that all file transfers have the same bandwidth
	// requirements.
  FPilot **WACK;
	// This  is the array of pointers to the pilot processes servicing this
	// traffic pattern.
  RVariable *Delay;
	// A  random-variable  pointer.  The  random  variable  will be used to
	// calculate the delay statistics.
  Long NRetransmissions;
	// The number of file retransmissions ...
  BITCOUNT NRetransmittedBits;
	// And  the  total  number  of  retransmitted  bits  (to be counted and
	// printed out at the end of the experiment).
  void setup () {
    int i;
    double mit, mle, r;
    readIn (mit);                // Mean file interarrival time (per sender)
    readIn (mle);                // Mean file length in bits
    readIn (Bandwidth);          // The bandwidth used by a file transfer
    readIn (r);                  // Acknowledgement timeout
    ATimeout = (TIME) (r * Etu); // Converted from seconds to ITUs
    readIn (r);                  // The mean backoff time
    Backoff = r * Etu;           // Also converted to ITUs
    Delay = create RVariable;    // The random variable
    NRetransmissions = NRetransmittedBits = 0;
    // We  switch  off  the  standard  pilot  processes for the traffic pattern
    // (SCL_off) and  we  declare  that  we  don't  want  to  collect  standard
    // statistics.   However,  we  want  to  use  the  standard  random  number
    // generators to determine the interarrival time  (MIT_exp  ->  exponential
    // distribution)  and  the  file  length (MLE_exp). A note to Carey: I know
    // these things should not be Poisson. I  hope  you  will  change  them  to
    // something more decent.
    Traffic::setup (SCL_off+SPF_off+MIT_exp+MLE_exp, mit, mle);
    WACK = new FPilot* [NEndNodes];
    for (i = 0; i < NEndNodes; i++) {
      // In  this loop we add all end-nodes to the sender and receiver sets for
      // the traffic pattern with  equal  weights.  This  means  that  when  we
      // generate  a  receiver  at random, all stations (other than the sender)
      // will have the same chances for being selected.
      addSender (i);
      addReceiver (i);
      // Now  we  create  the pilot processes, one per each end-node. Note that
      // each pilot process runs at a specific end-node.
      WACK [i] = create (i) FPilot;
    };
  };
  // The  dilema of object orientedness: these operations belong to the traffic
  // pattern  as  well  as  to  a  specific  EndNode.  Where  should  they   be
  // implemented? SMURPH says: at the traffic pattern.
  void pfmMTR (Cell *c) {
    // This  method  is  called  automatically  when a message (file?) has been
    // completely transmitted (i.e., the last cell of a message  is  "released"
    // by the Source process).
    if (flagCleared (c->Attributes, OOBD)) WACK [c->Sender] -> signal ();
    // Besides  files,  this  traffic  pattern  will  also  be used to generate
    // acknowledgements. An acknowledgement message  (which  is  always  mapped
    // into  a  single  cell)  is  told  from  a  file  by the OOBD flag in its
    // Attributes. The above statement should read: if the transmitted  message
    // is  not an acknowledgement (which means that it is a file), send a dummy
    // signal to the pilot process running at the  sender.  The  pilot  process
    // wants  to  know  when the file has been transmitted so that it can start
    // awaiting an acknowledgement.
  };
  void pfmPRC (Cell *c) {
    // This  method  is called automatically for every cell received (operation
    // "receive") at the destination
    if (flagSet (c->Attributes, OOBD)) {
      // This is an acknowledgement
      if (flagSet (c->Attributes, PACK))
	// A positive one
	WACK [c->Receiver] -> signal ((void*)YES);
      else if (flagSet (c->Attributes, NACK))
	// A negative one
	WACK [c->Receiver] -> signal ((void*)NO);
	// In  either  case, the pilot process at the file sender (the receiver
	// of the acknowledgement cell) is awakened. It  tells  the  difference
	// between  the  two acknowledgement kinds by examining the signal that
	// wakes it up.
    } else {
      // This  is  a  regular  file  cell. Now we validate the sequence number.
      // Note: this operation really belongs to the traffic pattern as a single
      // EndNode may receive files from several sessions  (VCIs)  at  the  same
      // time.
      if (SequenceNumbers [c->VCI] != c->SeqNum) {
	// The cell appears to be out of sequence
	if ((SequenceNumbers [c->VCI] & 0xfff00000) == (c->SeqNum & 0xfff00000))
	  // The  upper  portion  (12  bits)  of  the sequence number gives the
	  // retransmission count of the file. This way we are able to  tell  a
	  // new  version of the same file retransmitted after a timeout. If we
	  // are here, the retransmission counts match  so  the  cell  actually
	  // arrives out of sequence.
	  LostCells [c->VCI] += c->SeqNum - SequenceNumbers [c->VCI];
	else
	  // If  we  are  here, the cell belongs to a new retransmission of the
	  // file. We start counting lost cells from scratch.
	  LostCells [c->VCI] = (c->SeqNum) & 0xfffff;
      }
      // At  the  end,  the  expected  sequence  number  is  always  set to the
      // last-received number plus one.
      SequenceNumbers [c->VCI] = c->SeqNum + 1;
    }
  };
  void pfmMRC (Cell *c) {
    // This  method  is  called  automatically for every last cell of a message
    // (file?) received at the destination.
    ATMMessage *m;
    if (flagCleared (c->Attributes, OOBD)) {
      // This  is  the  end  of  a file transmission, so we have to generate an
      // acknowledgement. The acknowledgement is queued for transmission at the
      // receiver of the file and is addressed to the sender. Its size  matches
      // the size of one cell payload.
      m = (ATMMessage*) genMSG (c->Receiver, (int)(c->Sender), PayloadSize);
      m->VCI = c->VCI;
      m->Type = DTC;
      m->SeqNum = 0;                    // This shouldn't be relevant
      m->Attributes = 0;
      setFlag (m->Attributes, OOBD);    // Mark it as special
      if (LostCells [c->VCI])
	// Some cells have been lost, so this is a NACK
	setFlag (m->Attributes, NACK);
      else {
	// Everything OK, so this is a positive acknowledgement
	setFlag (m->Attributes, PACK);
	// A file has been received successfully - update delay statistics
	Delay->update ((Time - c->QTime) * Itu);
	// Note:  in  SMURPH,  every packet (cell) carries an attribute (QTime)
	// that tells the time when the message from which the packet has  been
	// acquired was queued at the sender. This time is in ITUs. We can turn
	// it  to  ETUs  (seconds  in our case) by multiplying it by Itu (which
	// gives the number of ETUs in one ITU). We  could  have  postponed  it
	// until later to save one multiplication per file, but I told you that
	// we wouldn't care much for cheap efficiency tricks.
      }
    }
  };
  // Below  we  declare an exposure method that tells what specific information
  // related to the traffic pattern should be printed to the output file and/or
  // displayed on the screen in a window. I don't think it is very  interesting
  // at the moment.
  exposure;
};
FilesTP::exposure {
    Traffic::expose;
    onpaper {
      exmode 4:
        print ("File traffic statistics:\n\n");
        Delay->printCnt ("Delay");
        print (NRetransmissions,   "Number of retransmissions:");
        print (NRetransmittedBits, "Retransmitted bits:       ");
        print ("\n");
    }
    onscreen {
      exmode 4:
        Delay->displayOut (0);
        display (NRetransmissions);
        display (NRetransmittedBits);
    }
};


FilesTP *FTP;
   // This  will  point  to  the  actual  object  representing the file traffic
   // pattern, when we create it.

FPilot::perform {
     // This is the pilot process for the file traffic pattern
  HandShake *cn;
  ATMMessage *m;
  state Wait:
    // Here  we  wait for the file arrival event. We will get to state Generate
    // after the amount of  time  generated  as  an  exponentially  distributed
    // random  number  with  the mean specified in the setup method of FilesTP.
    // This is taken care of by genMIT which is  a  standard  method  of  every
    // traffic pattern.
    Timer->wait (FTP->genMIT (), Generate);
  state Generate:
    // We start from generating the receiver at random in such a way that it is
    // different  from  the  sender. This is accomplished by calling genCGR (to
    // exclude the sender) and then genRCV (to actually generate the receiver).
    FTP->genCGR (Sender);
    Receiver = FTP->genRCV ();
    // Now for the message (i.e. file) length. Another standard method, genMLE,
    // is used for this purpose.
    Length = FTP->genMLE ();
    // Now we get a VCI number for the transfer session
    VCI = getVCI ();
    // Initialize the number of retransmissions
    Retry = 0;
    // Start counting lost cells
    SequenceNumbers [VCI] = LostCells [VCI] = 0;
    // Create a HandShake process to set up the connection
    cn = create HandShake (Receiver, VCI, CST, ACK, FTP->Bandwidth);
    // And wait until the HandShake process terminates itself (meaning that the
    // connection has been established)
    cn -> wait (DEATH, GenMessage);
  state GenMessage:
    // We get here to generate the file, i.e., a message representing it
    m = (ATMMessage*) FTP->genMSG (Sender, (int) Receiver, Length);
    m->VCI = VCI;
    m->Type = DTC;
    // The sequence number starts from zero, but it includes the retransmission
    // count  in  the  upper  12  bits.  This way we are able to tell different
    // retransmissions apart at the receiver's end.
    m->SeqNum = (Retry << 20) & 0xfff00000;
    m->Attributes = 0;
    // Now  we  are going to wait for a signal from pfmMTR (see above) to learn
    // when the message (file) has been transmitted. We start from clearing the
    // process' signal repository -- just in case.
    erase ();
    // This wait request is addressed to the process itself
    wait (SIGNAL, MessageSent);
  state MessageSent:
    // The file has been transmitted. Now we will wait for another signal (this
    // time from pfmPRC) indicating an acknowledgement ...
    erase ();
    wait (SIGNAL, GotReply);
    // And  also  for  the timeout, in case the acknowledgement doesn't make it
    // (or  perhaps  the  last  cell  of  our  file  didn't  make  it  to   the
    // destination).
    Timer->wait (FTP->ATimeout, GenMessage);
  state GotReply:
    // We have received an acknowledgement
    if ((IPointer)TheSignal == NO) {
      // NACK -- increment counters
      FTP->NRetransmissions++;
      FTP->NRetransmittedBits += Length;
      Retry++;
      // Sleep  for  a  randomized  amount  of  time  and  send the file again.
      // tRndPoisson returns an exponentially distributed random number of type
      // TIME with the mean  specified  as  the  argument  (which  is  of  type
      // double).
      Timer->wait (tRndPoisson (FTP->Backoff), GenMessage);
    } else {
      // This  is a positive acknowledgement, so we are done with the file. Now
      // we create a HandShake  process  to  close  the  connection.  When  the
      // process is done, we will transit to state Disconnect.
      cn = create HandShake (Receiver, VCI, DSC, DAK, FTP->Bandwidth);
      cn -> wait (DEATH, Disconnect);
    }
  state Disconnect:
    // The  connection has been closed. We return to the VCI number to the free
    // pool and move to Wait to generate another file interarrival time.
    freeVCI (VCI);
    proceed Wait;
};

process VPilot (EndNode) {
     // This  is  the pilot process for the video session. This traffic pattern
     // is simpler than the file transfer pattern because  now  we  don't  care
     // about  acknowledgements. We just transmit the frames and count how many
     // of them have been lost.
  int VCI;
  Long Sender, Receiver, FirstFrameLength, NFrames, SequenceNumber, FCount;
  TIME StartTime;
  void setup (TIME ST, Long dest, Long FFL, Long NF) {
    // We  assume that the session starts at some definite time (StartTime) and
    // goes through a specific number of frames (NFrames). FCount  keeps  track
    // of  how many frames have been transmitted so far. SequenceNumber is used
    // to determine the sequence number of the first cell of every next frame.
    StartTime = ST;
    Sender = S->getId ();
    Receiver = dest;
    FirstFrameLength = FFL;
    NFrames = NF;
    SequenceNumber = FCount = 0;
  };
  states {Hibernate, Connect, FirstFrame, NextFrame, Disconnect};
  perform;
};

traffic VideoTP (ATMMessage, Cell) {
     // This  is  the  traffic  pattern representing the video session. Nothing
     // tricky here. The session has some specific bandwidth requirements.  The
     // length  of the first frame is determined explicitly (FirstFrameLength),
     // the remaining frames are supposed to be updates to the first frame. The
     // length of each such frame will be generated as a  uniformly-distributed
     // random  number between MinFrameLength and MaxFrameLength. The amount of
     // time  between  two  consecutive  frames  is  fixed  (within  the  clock
     // accuracy) and determined by FrameSpacing.
  Long Source, Destination, Bandwidth;
  TIME StartTime;
  Long FirstFrameLength, NFrames;
  double MinFrameLength, MaxFrameLength, FrameSpacing;
  int VCI;
  void setup () {
    double r;
    readIn (Source);
    readIn (Destination);
    readIn (r);
    StartTime = (TIME) (r * Etu);    // Convert from ETU to ITU
    readIn (FirstFrameLength);
    readIn (MinFrameLength);
    readIn (MaxFrameLength);
    readIn (FrameSpacing);
    readIn (Bandwidth);
    readIn (NFrames);
    VCI = 0;
    // The   following   reads:   interarrival  time  fixed,  length  uniformly
    // distributed
    Traffic::setup (SCL_off+SPF_off+MIT_fix+MLE_unf, FrameSpacing,
      MinFrameLength, MaxFrameLength);
    create (Source) VPilot (StartTime, Destination, FirstFrameLength, NFrames);
  };
  void pfmPRC (Cell *c) {
    // Called for every cell received (all cells are data cells)
    assert (c->VCI == VCI, "VideoTP: illegal VCI");
    if (SequenceNumbers [VCI] != c->SeqNum)
      LostCells [VCI] += c->SeqNum - SequenceNumbers [VCI];
    SequenceNumbers [VCI] = c->SeqNum + 1;
  };
  exposure;
};
VideoTP::exposure {
    onpaper {
      exmode 4:
        print ("Video traffic statistics:\n");
        print (SequenceNumbers [VCI] - 1, "Total number of cells:   ");
        print (LostCells [VCI]          , "Number of lost cells:    ");
        print ((double)(LostCells [VCI])/(SequenceNumbers [VCI] - 1)
                                        , "Lost/Total:              ");
        print ("\n");
    }
    onscreen {
      exmode 4:
        display (SequenceNumbers [VCI] - 1);
        display (LostCells [VCI]);
        display ((double)(LostCells [VCI])/(SequenceNumbers [VCI] - 1));
    }
};

VideoTP *VTP;

VPilot::perform {
  HandShake *cn;
  ATMMessage *m;
  Long fl;
  state Hibernate:
    // In  this  state  the pilot process hibernates until the video session is
    // launched. Then it gets to state Connect.
    Timer->wait (StartTime, Connect);
  state Connect:
    VTP->VCI = VCI = getVCI ();
    // Start counting lost cells
    SequenceNumbers [VCI] = LostCells [VCI] = 0;
    // As before, we establish the connection with the assistance of HandShake
    cn = create HandShake (Receiver, VCI, CST, ACK, VTP->Bandwidth);
    cn -> wait (DEATH, FirstFrame);
  state FirstFrame:
    // Generate and transmit the first frame
    m = (ATMMessage*) VTP->genMSG (Sender, (int)Receiver, FirstFrameLength);
    m->VCI = VCI;
    m->Type = DTC;
    m->SeqNum = SequenceNumber;
    m->Attributes = 0;
    setFlag (m->Attributes, DROP);  // Video frames are RED (droppable)
    SequenceNumber += (FirstFrameLength + PayloadSize - 1) / PayloadSize;
    // SequenceNumber is advanced by the number of cells in the frame
    FCount++;
    // And we wait for the arrival of another frame
    Timer->wait (VTP->genMIT (), NextFrame);
  state NextFrame:
    // Generate subsequent frames
    m = (ATMMessage*) VTP->genMSG (Sender, (int)Receiver, fl = VTP->genMLE ());
    m->VCI = VCI;
    m->Type = DTC;
    m->SeqNum = SequenceNumber;
    m->Attributes = 0;
    setFlag (m->Attributes, DROP);
    SequenceNumber += (fl + PayloadSize - 1) / PayloadSize;
/*@@@ trace ("Frame %1d of %1d", FCount, NFrames); */
    if (++FCount >= NFrames) {
      // That's it
      cn = create HandShake (Receiver, VCI, DSC, DAK, VTP->Bandwidth);
      cn -> wait (DEATH, Disconnect);
    } else
      Timer->wait (VTP->genMIT (), NextFrame);
  state Disconnect:
    freeVCI (VCI);
    // When the session ends, we terminate the simulation experiment by killing
    // the  system  process  Kernel.  This is the standard way of terminating a
    // SMURPH program.
    Kernel->terminate ();
};

void startTransmit () {
     // This method creates the three traffic patterns declared above
  STP = create SignalTP;
  FTP = create FilesTP;
  VTP = create VideoTP;
};

// ################################################################### //

process Root {
     // This  is  the  root  process  which  plays in SMURPH the role of a main
     // program. The process is started automatically in its first state at the
     // very beginning of the simulation.
  states {Start, Stop};
  perform {
    state Start:
      makeNetwork ();              // Build the network
      startProtocol ();            // Start the protocol processes
      startTransmit ();            // Create traffic patterns
      Kernel->wait (DEATH, Stop);  // Wait until the simulation is over
    state Stop:
      // Print the results
      System->printTop ("Network topology");
      Client->printDef ("Traffic conditions");
      FTP->printOut (4);
      VTP->printOut (4);
  };
};
