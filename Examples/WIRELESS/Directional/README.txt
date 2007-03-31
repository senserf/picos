This is a model of a very naive wireless ad-hoc network with directional
antennas. Its purpose is not to be useful for any experiments, but rather
to provide stubs for implementing models of more decent ideas.

OK, so there is a bunch od nodes (type Node) distributed over some area.
We assume thet each node knows its coordinates. At some reasonably regular
but randomized intervals each node broadcasts a HELLO packet specifying
its coordinates. HELLO packets flood the network, with trivial containment
based on TTL and duplicate rejection. Having received a HELLO packet a node
does this:

If this packet arrives from an immediate neighbor, the node updates its
neighbor pool by adding the sender and its coordinates.

In any case, the node updates its "network map", i.e., the list of known
nodes in the network and their coordinates.

The neighbor pool is periodically cleared of obsolete entries. If a neighbor
does not report again within some interval, its entry will be deemed obsolete
and removed. Entries in the network map are never cleared - only updated,
possibly with new locations (nodes are allowed to move).

A node tells whether a packet arrives from its immediate neighbor by comparing
its MAC-layer sender to the "transport" sender. Here is the list of relevant
packet attributes:

	Sender (built-in)   : the transport sender (node number)
	SA                  : the MAC-layer sender
	Receiver (built-in) : the transport receiever (data packets only)
	TTL                 : time-to-live (hops left)
        TP (built-in)       : type (-1 HELLO, >= 0 data)
	ILength (built-in)  : payload length in bits
	TLength (built-in)  : total length in bits (logical, incl. header)

The Sender attribute is set by the packet originator and doesn't change while
the packet is hopping through the network. SA is set on every hop by the sender
(forwarder). Receiver is only relevant for data packets and denotes the
ultimate destination. When a data packet is forwarded, it is not addressed
to a specific neighbor (anybody can pick it up). The transmitter's antenna
is only aimed at the neighbor that offers the closest angle towards the
destination.

A shadowing channel model is used. This is the same model that we now use in
VUEE (for PICOS). I have put it into the standard SMURPH library (Examples/
IncLib): files wchansh.h, wchansh.cc + wchan.h, wchan.cc (the shadowing channel
is built on top of a generic channel model in wchan.[h,cc]).

You have to familiarize itself with the concept of assessment methods. This is
all described in SMURPH manual. Perhaps you should start by reading Yannis's
paper.

The MAC layer is also a bit naive. It actually closely mimics what we do in
PicOS for very small devices, but for the kind of networks you want to model,
it might make sense to use something a bit more involved. The MAC/physical
layer is programmed in files rfmodule.[h,cc]. Please go through them and read
the comments.
