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

/* ---------------------------------------------------------- */
/* Bursty traffic against uniform background. In this traffic */
/* pattern,  there  is  a  uniform background traffic and one */
/* selected station generates occasionally a fixed-size burst */
/* of messages arriving all at the same time. This pattern is */
/* used to determine the network fairness.                    */
/* ---------------------------------------------------------- */

#ifdef  EXTENDED_MTYPE
traffic BUUTraffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic BUUTraffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  EXTENDED_MTYPE
traffic BUBTraffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic BUBTraffic {
#endif
	RVariable *BUBWPAcc;    // Weighted packet access time
	BUBTraffic ();
	virtual void pfmPTR (Packet*);
	virtual void pfmMTR (Packet*);
};

#ifdef  LOCAL_MEASURES

station USTAT virtual {
	RVariable *BUUAPAcc,    // Absolute packet access time
		  *BUUWPAcc,    // Weighted packet access time
		  *BUUAMDel;    // Absolute message delay
};

#endif

void    initBUTraffic (double, double, int, double, double, int, double);
void    printBUPFM ();
