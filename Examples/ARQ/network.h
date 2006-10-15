// Network types

packet	Frame {
	// One extra attribute in the header -> the serial number
	int	SN;
};

packet	Ack {
	// Just the serial number
	int	SN;
};

// The traffic generator and absorber
traffic	TrGen (Message, Frame);

extern	PLink	*Link1, *Link2;

extern	TrGen	*UpperLayer;

extern	int	MinFrameLength, MaxFrameLength, AckLength;
