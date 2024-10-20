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
/* Simple  bursty  traffic  pattern. Messages of fixed length */
/* arrive in bursts of fixed size.                            */
/* ---------------------------------------------------------- */

#ifdef  EXTENDED_MTYPE
traffic BSTraffic (UMessage, UPacket) {
		// These types must be supplied in the including file
#else
traffic BSTraffic {
#endif

#ifdef  LOCAL_MEASURES
	// Virtual functions extending performance measures
	virtual void pfmMRC (Packet*);
	virtual void pfmPTR (Packet*);
#endif

};

#ifdef  LOCAL_MEASURES

station USTAT virtual {
	RVariable *BSAPAcc, *BSAMDel;
#ifdef	FLUSH_TIME
	// Used to measure the queue flush delay, i.e. the amount of time
	// it takes for the station message queue to be completely flushed.
	// Using FLUSH_TIME makes sense if there is a single burst and then
	// the simulation continues until the queues are exhausted (smurph
	// called with parameter -c).
	TIME	  BSFlushTime;
#endif
};

#endif

void    initBSTraffic (double, double, double);
void    printBSPFM ();
