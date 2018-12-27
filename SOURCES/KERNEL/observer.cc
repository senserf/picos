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

/* --- */



/* ---------------- */
/* Observer service */
/* ---------------- */

#include        "system.h"

#define SHF (BIL-1)

#define FxMASK LCS(0xffffffffffffffff)
#define SH_1 ((LONG)1)

#if     ZZ_OBS

static  Process *phandle = NULL,        // Handle for observer service process
	*PFound;                        // For pttrav

enum { Start, ObsTimeout };             // States made global within this file

/* ----------------------------------------- */
/* Observer service process type declaration */
/* ----------------------------------------- */

extern  int     zz_ObserverService_prcs;

class   ObserverService : public ZZ_SProcess {

	public:

	Station *S;

	ObserverService () {
		S = TheStation;
		zz_typeid = (void*) (&zz_ObserverService_prcs);
	};

	virtual char *getTName () { return ("ObserverService"); };

	ZZ_INSPECT *ins, *jns;

	void setup () {

		// Initialize state name list

		zz_ns = 2;
		zz_sl = new const char* [ObsTimeout + 1];
		zz_sl [0] = "Start";
		zz_sl [1] = "ObsTimeout";
	};

	void zz_code () {

	 switch (TheState) {

	  case Start:          // Void

	  break; case ObsTimeout:

		zz_current_observer = (Observer*) Info01;
		TheObserverState = ptrToInt (Info02);

		// Erase the inspects
		
		for (ins = zz_current_observer->inlistHead; ins != NULL;
			ins=jns) {

			jns = ins -> next;
			delete ((void*) ins);
		}

		zz_current_observer->inlistHead = zz_inlist_tail = NULL;

		// Clear inspect masks

		zz_current_observer->amask = zz_current_observer->pmask =
		zz_current_observer->nmask = zz_current_observer->smask = 0;

		zz_current_observer->tevent = NULL;     // No Timer event

		// Call the observer

		zz_observer_running = YES;

		zz_current_observer->zz_code ();
		if (DisplayActive) {
		    // Observer stepping check
		    ZZ_SIT   *ws;

		    for (ws = zz_steplist; ws != NULL; ws = ws->next) {
		      if (ws->obs != zz_current_observer) continue;
		      if (ws->station != NULL && ws->station != TheStation)
			continue;
		      if (ws->process != NULL && ws->process != TheProcess)
			continue;
		      if (ws->ai != NULL && ws->ai != zz_CE->ai)
			continue;

		      zz_events_to_display = 0;
		      zz_send_step_phrase = YES;
		      break;
		    }
		}

		zz_observer_running = NO;
	 }
	}

};

#endif

void    Observer::zz_start () {

#if     ZZ_OBS

	Class = AIC_observer;
	Id = sernum++;

	if (phandle == NULL) {

		Station *st;
		Process *tp;

		// Create the service process (belonging to the Kernel
		st = TheStation;
		tp = TheProcess;
		TheStation = System;
		TheProcess = Kernel;
		// phandle = create ObserverService;
		phandle = new ObserverService;
		((ObserverService*) phandle)->zz_start ();
		((ObserverService*) phandle)->setup ();
		TheStation = st;
		TheProcess = tp;
	}

	ChList = NULL;

	// Add the observer to the global observer list
	pool_in (this, zz_obslist);
	inlistHead = zz_inlist_tail = NULL;
	tevent = NULL;
	smask = pmask = nmask = amask = 0L;

	// Add the observer to the owner's list
	pool_in ((ZZ_Object*)this, TheProcess->ChList);
	zz_observer_running = YES;
	zz_current_observer = this;
	TheObserverState = 0;                   // First time around
	zz_code ();
	zz_observer_running = NO;

#endif
}

const char    *Observer::zz_sn (int) {

/* -------------------------------------- */
/* Returns the name of the state number i */
/* -------------------------------------- */

	// This is a virtual function
	return ("undefined");
}

void terminate (Observer *o) {

/* ---------------------- */
/* Terminates an observer */
/* ---------------------- */

#if     ZZ_OBS

	ZZ_INSPECT         *ins, *jns;
	ZZ_EVENT           *ev;

	// Erase any inspects that the observer may have created

	for (ins = o->inlistHead; ins != NULL; ins=jns) {
		jns = ins -> next;
		delete ((void*) ins);
	}

	// Remove pending timeout event

	if (o->tevent != NULL) {
		for (ev = zz_eq; ev != zz_sentinel_event; ev = ev -> next)
			if (ev == o->tevent) {
				ev->cancel ();
				delete ev;
				break;
			}
	}
	o->tevent = NULL;

	// Remove the observer from the observer pool
	Assert (o->ChList == NULL,
		"Can't terminate %s, ownership list not empty",
			o->getSName ());

	pool_out (o);
	pool_out ((ZZ_Object*)o);
        zz_DREM (o);
	if (o->zz_nickname != NULL)
		delete [] o->zz_nickname;
	delete ((void*) o);
#endif
}

void zz_inspect_rq (Station *s, void *p, char *n, int pstate, int os) {

/* ---------------------------------------------------------- */
/* Issues an inspect request from an observer (called through */
/* a macro)                                                   */
/* ---------------------------------------------------------- */

#if  ZZ_OBS

	ZZ_INSPECT         *ins;

	if_not_from_observer ("inspect: called not from an observer");

	if (isStationId ((LPointer) s))
		// Again this dirty trick, but there seems to be no way out
		s = idToStation ((Long) ((LPointer) s));

	// Update observer masks
	//
	// Note: the following instructions assume that words are (at least)
	//       32 bits long.

	if ((LPointer)(s) == ANY)
		zz_current_observer->smask = FxMASK;
	else
		zz_current_observer->smask |= SH_1 << ((s->Id) & SHF);

	if (p == (void*) (&zz_Process_prcs)) {
		p = (void*) ANY;
		zz_current_observer->pmask = FxMASK;
	} else
		zz_current_observer->pmask |= SH_1 << (((ptrToInt (p)) >> 2) &
			SHF);

	if ((LPointer)(n) == ANY)
		zz_current_observer->nmask = FxMASK;
	else
		zz_current_observer->nmask |= SH_1 << ((*n) & SHF);

	if (pstate == ANY)
		zz_current_observer->amask = FxMASK;
	else
		zz_current_observer->amask |= SH_1 << (pstate & SHF);

	// Create and queue the inspect request
	ins = new ZZ_INSPECT (s, p, n, pstate, os);
#endif
}

void    zz_timeout_rq (TIME t, int a) {

/* ----------------------------------------- */
/* Issues a timeout request from an observer */
/* ----------------------------------------- */

#if     ZZ_OBS

	if_not_from_observer ("timeout: called not from an observer");

	assert (zz_current_observer->tevent == NULL,
		"timeout: duplicate timeout request from an observer (%s)",
			zz_current_observer->getSName ());

	zz_current_observer->tevent = new ZZ_EVENT (zz_sentinel_event,
		Time + t, System, (void*) zz_current_observer, (void*)a,
			phandle, zz_current_observer, OBS_TSERV, ObsTimeout,
				NULL);
#endif
}

const char *ZZ_INSPECT::getptype () {

#if     ZZ_OBS

/* ---------------------------------------------------------- */
/* Returns  the  type  name of the process represented by the */
/* inspect entry                                              */
/* ---------------------------------------------------------- */

	ZZ_Object  *o;
	const char *res;
	int        i;

	if (typeidn == (void*)(ANY)) return ("ANY");

	for (i = 0; i < NStations; i++) {
		for (o = idToStation (i)->ChList; o != NULL; o = o->next) {
			if (o->Class == AIC_process) {
				if ((res = pttrav (o)) != NULL) {
					return (res);
				}
			}
		}
	}

	for (o = Kernel->ChList; o != NULL; o = o->next) {
		if (o->Class == AIC_process) {
			if ((res = pttrav (o)) != NULL) {
				return (res);
			}
		}
	}

	return ("unknown");
#else
	return (NULL);
#endif

}

const char *ZZ_INSPECT::pttrav (ZZ_Object *o) {

#if     ZZ_OBS

/* --------------------------------- */
/* A recursive searcher for getptype */
/* --------------------------------- */

	if (((Process*)o)->zz_typeid == typeidn) {
		PFound = (Process*) o;
		return (o->getTName ());
	}

	for (ZZ_Object *b = ((Process*)o)->ChList; b != NULL; b = b->next)
		if (b->Class == AIC_process) return (pttrav (b));

	return (NULL);
#else
	return ((char*)o);
#endif
}

sexposure (Observer)

	sonpaper {

		sfxmode (0)

			exPrint0 (Hdr);         // Global information

		sexmode (1)

			exPrint1 (Hdr);         // Inspect list
	}

	sonscreen {

		sfxmode (0)

			exDisplay0 ();          // Global information

		sexmode (1)

			exDisplay1 ();          // Inspect list
	}
	USESID;
}

void    Observer::exPrint0 (const char *hdr) {

#if     ZZ_OBS

/* ------------------------------------- */
/* Print information about all observers */
/* ------------------------------------- */

	Observer                        *o;
	ZZ_INSPECT                      *in;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << "(Observer) Inspect lists of all observers:\n\n";
	}

	Ouf << "            Observer    St      PType  PNickname      State" <<
		"     OState\n";

	for (o = zz_obslist; o != NULL; o = o -> next) {
		for (in = o->inlistHead; in != NULL; in = in -> next) {

			if (in == o->inlistHead)
				// First time for this observer
				print (o->getOName (), 20);
			else
				Ouf << "                    ";
			Ouf << ' ';
			if ((LPointer)(in->station) == ANY)
				print ("ANY", 5);
			else
				print (zz_trunc (ident (in->station), 5), 5);
			Ouf << ' ';
	
			PFound = NULL;  // Will be set by getptype
			print (in->getptype (), 10);

			Ouf << ' ';
			if ((LPointer)(in->nickname) == ANY)
				print ("ANY", 10);
			else
				print (in->nickname, 10);
			Ouf << ' ';
			if (in->pstate == ANY)
				print ("ANY", 10);
			else if (PFound != NULL)
				print (PFound->zz_eid (in->pstate), 10);
			else
				print (zz_trunc (in->pstate, 10), 10);
			Ouf << ' ';
			print (o->zz_sn (in->ostate), 10);
			Ouf << '\n';
		}
		if (o->tevent != NULL) {
			// Display timeout info
			if (o->inlistHead == NULL)
				print (o->getOName (), 20);
			else
				Ouf << "                    ";
			Ouf << "    >> Timeout at ";
			ptime (tevent->waketime, 15);
			Ouf << " <<\n";
		}
	}

	Ouf << "\n(Observer) End of list\n\n";
#endif
}

void    Observer::exPrint1 (const char *hdr) {

#if     ZZ_OBS

/* --------------------------------------- */
/* Print the inspect list of this observer */
/* --------------------------------------- */

	ZZ_INSPECT                      *in;

	if (hdr != NULL) {
		Ouf << hdr << "\n\n";
	} else {
		zz_outth (); Ouf << ' ';
		Ouf << '(' << getOName () << ") Inspect list:\n\n";
	}

	Ouf << "    St      PType         Id      State     OState\n";

	for (in = inlistHead; in != NULL; in = in -> next) {

		if ((LPointer)(in->station) == ANY)
			print ("ANY", 5);
		else
			print (zz_trunc (ident (in->station), 5), 5);
		Ouf << ' ';
	
		PFound = NULL;
		print (in->getptype (), 10);

		Ouf << ' ';
		if ((LPointer)(in->nickname) == ANY)
			print ("ANY", 10);
		else
			print (in->nickname, 10);
		Ouf << ' ';
		if (in->pstate == ANY)
			print ("ANY", 10);
		else if (PFound != NULL)
			print (PFound->zz_eid (in->pstate), 10);
		else
			print (zz_trunc (in->pstate, 10), 10);

		Ouf << ' ';
		print (zz_sn (in->ostate), 10);
		Ouf << '\n';
	}

	if (tevent != NULL) {
		// Display timeout info
		Ouf << "    >> Timeout at ";
		ptime (tevent->waketime, 15);
		Ouf << " <<\n";
	}

	Ouf << "\n(" << getOName () << ") End of list\n\n";
#endif
}

void    Observer::exDisplay0 () {

#if     ZZ_OBS

/* ------------------------------------- */
/* Print information about all observers */
/* ------------------------------------- */

	Observer                        *o;
	ZZ_INSPECT                      *in;

	for (o = zz_obslist; o != NULL; o = o -> next) {
		for (in = o->inlistHead; in != NULL; in = in -> next) {

			if (in == o->inlistHead)
				// First time for this observer
				display (o->getOName ());
			else
				display (' ');

			if ((LPointer)(in->station) == ANY)
				display ("ANY");
			else
				display (ident (in->station));
	
			PFound = NULL;
			display (in->getptype ());

			if ((LPointer)(in->nickname) == ANY)
				display ("ANY");
			else
				display (in->nickname);

			if (in->pstate == ANY)
				display ("ANY");
			else if (PFound != NULL)
				display (PFound->zz_eid (in->pstate));
			else
				display (in->pstate);

			display (o->zz_sn (in->ostate));
		}
		if (o->tevent != NULL) {
			// Display timeout info
			if (o->inlistHead == NULL)
				display (o->getOName ());
			else
				display (' ');

			display ("Timeout at");
			dtime (o->tevent->waketime);
			display (' ');
			display (' ');
			display (' ');
		}
	}
#endif
}

void    Observer::exDisplay1 () {

#if     ZZ_OBS

/* ----------------------------------------- */
/* Display the inspect list of this observer */
/* ----------------------------------------- */

	ZZ_INSPECT                      *in;

	for (in = inlistHead; in != NULL; in = in -> next) {

		if ((LPointer)(in->station) == ANY)
			display ("ANY");
		else
			display (ident (in->station));
	
		PFound = NULL;
		display (in->getptype ());

		if ((LPointer)(in->nickname) == ANY)
			display ("ANY");
		else
			display (in->nickname);

		if (in->pstate == ANY)
			display ("ANY");
		else if (PFound != NULL)
			display (PFound->zz_eid (in->pstate));
		else
			display (in->pstate);

		display (zz_sn (in->ostate));
	}

	if (tevent != NULL) {
		// Display timeout info
		display ("Timeout at");
		dtime (tevent->waketime);
		display (' ');
		display (' ');
		display (' ');
	}
#endif
}
