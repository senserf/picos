#ifndef	__traffic_h__
#define	__traffic_h__

traffic UTraffic (Message, Packet) {

	IPointer genRCV () {
		// There are no explicit receivers in this simple model
		return (IPointer) NONE;
	};
};

void initTraffic ();

#endif
