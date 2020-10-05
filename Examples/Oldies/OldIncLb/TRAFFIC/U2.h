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
/* Two uniform traffic patterns with exponential distribution */
/* of arrival and length                                      */
/* ---------------------------------------------------------- */

#ifdef  EXTENDED_MTYPE
traffic U0Traffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic U0Traffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  EXTENDED_MTYPE
traffic U1Traffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic U1Traffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  LOCAL_MEASURES

station USTAT virtual {
	RVariable *U0APAcc, *U0AMDel, *U1APAcc, *U1AMDel, *UAPAcc, *UAMDel;
};

#endif

U0Traffic *U0TPattern;
U1Traffic *U1TPattern;

void    initU2Traffic (double, double, double, double);
void    printU2PFM ();
