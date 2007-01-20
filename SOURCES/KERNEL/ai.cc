/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

// #define  CDEBUG   1

/* ------------------------------------------------------- */
/* Global functions related to AIs and displayable objects */
/* ------------------------------------------------------- */

#include        "system.h"

#include	"cdebug.h"

#define		o_sent(o,tp)	zz_hptr (((tp*)(o))->ChList, ZZ_Object)

#ifdef  CDEBUG
void    zz_dumpTree (ZZ_Object *o) {

/* -------------------- */
/* Dumps ownership tree */
/* -------------------- */

	ZZ_Object  *clist, *osent;

	switch (o->Class) {

		case    OBJ_station:

			clist = ((Station*)o)->ChList;
			osent = o_sent (o, Station);
			break;

#if	ZZ_NOC
		case    AIC_traffic:

			clist = ((Traffic*)o)->ChList;
			osent = o_sent (o, Traffic);
			break;
#endif

		case    AIC_process:

			clist = ((Process*)o)->ChList;
			osent = o_sent (o, Process);
			break;

		case    AIC_observer:

			clist = ((Observer*)o)->ChList;
			osent = o_sent (o, Observer);
			break;


		default:

			return;         // Nothing to do
	}

	if (clist == NULL) return;      // Same thing

	// Go to the end of the object list

	while (clist->next != NULL) clist = clist->next;

	// Now go backwards

	do {
		cdebugl ("---------------------------");
		cdebugl (form ("Parent: %s (%x) /%x/", o->getSName (), (long)o,
								(long)clist));
		cdebugl (form ("Object: %s (%x)", clist->getSName (),
								(long)clist));
		cdebugl (form ("BasNam: %s", clist->getBName ()));
		cdebugl (form ("NikNam: %s", (clist->zz_nickname == NULL) ?
			"NONE" : clist->zz_nickname));
		zz_dumpTree (clist);    // Recursively for the child

	} while ((clist = clist->prev) != osent);
}
#endif

void    zz_adjust_ownership () {

/* ---------------------------------------------------------- */
/* Reorganizes  the  ownership  tree structure after Root has */
/* completed initialization                                   */
/* ---------------------------------------------------------- */

	ZZ_Object  *o, *so, *ox;
	int        i;

	// Move all stations to System

	Assert (System->ChList == NULL,
	"Adjusting ownership tree: internal error -- System ChList not empty");

	pool_in ((ZZ_Object*)Kernel, System->ChList, ZZ_Object);

	for (i = 0; i < NStations; i++) {
		o = idToStation (i);
		pool_out (o);
		pool_in (o, System->ChList, ZZ_Object);
		// Note that all objects are linked at their owners in the
		// reverse order of creation
	}

#if	ZZ_NOL
	// Now, move all links to System
	for (i = 0; i < NLinks; i++) {

		o = idToLink (i);
		pool_out (o);
		pool_in (o, System->ChList, ZZ_Object);
	}
#endif

#if	ZZ_NOR
	// Move all RFChannels to System (they will precede Links)
	for (i = 0; i < NRFChannels; i++) {

		o = idToRFChannel (i);
		pool_out (o);
		pool_in (o, System->ChList, ZZ_Object);
	}
#endif

#if	ZZ_NOC
	// Same with traffic patterns
	for (i = 0; i < NTraffics; i++) {

		o = idToTraffic (i);
		pool_out (o);
		pool_in (o, System->ChList, ZZ_Object);
	}

	// Fixed standard AI's

	pool_in ((ZZ_Object*)Client, System->ChList, ZZ_Object);
#endif
	pool_in ((ZZ_Object*)Timer, System->ChList, ZZ_Object);
	pool_in ((ZZ_Object*)Monitor, System->ChList, ZZ_Object);

	// Assign static mailboxes to their stations
	for (i = 0; i < NStations; i++) {

		Station *st;

		st = idToStation (i);
		for (o = st->Mailboxes; o != NULL; o = ((Mailbox*)o)->nextm) {
			pool_out (o);
			pool_in (o, st->ChList, ZZ_Object);
		}
	}

#if	ZZ_NOL
	// Assign ports to their stations
	for (i = 0; i < NStations; i++) {
		Station *st;
		st = idToStation (i);
		for (o = st->Ports; o != NULL; o = ((Port*)o)->nextp) {
			pool_out (o);
			pool_in (o, st->ChList, ZZ_Object);
		}
	}
#endif

#if	ZZ_NOR
	// Assign Transceivers to their stations
	for (i = 0; i < NStations; i++) {
		Station *st;
		st = idToStation (i);
		for (o = st->Transceivers; o != NULL; o =
		    ((Transceiver*)o)->nextp) {
			pool_out (o);
			pool_in (o, st->ChList, ZZ_Object);
		}
	}
#endif
	// Assign level 0 processes to their stations as well
	if ((so = ZZ_Main->ChList) != NULL) {
		// The process list nonempty (normally it should be)
		for (o = so; o->next != NULL; o = o->next);
		// Traverse the Kernel list backwards -- to retain the
		// creation order
		while (1) {

			if (o->Class != AIC_process ||
				((Process*)o)->Owner == System) {

				// System processes remain at Kernel
				if (o == so) break;
				o = o->prev;
				continue;
			}

			assert (((Process*)o)->Owner != NULL,
			   "System: a process without owner -- internal error");

			ox = o->prev;

			pool_out (o);
			pool_in (o, ((Process*)o)->Owner->ChList, ZZ_Object);
			if (o == so) break;
			o = ox;
		}
	}

#if	ZZ_NOS
	// Move RVariables at stations to the end of the children list (actually
	// they have to be moved to the front; the list is reversed!)
	for (i = 0; i < NStations; i++) {
		Station *st;
		st = idToStation (i);
		for (o = st->ChList, ox = o_sent (st, Station); o != NULL;) {
			if (o->Class != OBJ_rvariable) {
				o = o->next;
				continue;
			}
			so = o->next;
			pool_out (o);
			o->prev = ox;
			o->next = ox->next;
			if (o->next != NULL) o->next->prev = o;
			ox->next = o;
			ox = o;
			o = so;
		}
	}
#endif

	// Move User-defined displayable objects created at level-0 (belonging
	// to stations) to the end of the children list (actually
	// they have to be moved to the front; the list is reversed!)
	for (i = 0; i < NStations; i++) {

		Station *st;

		st = idToStation (i);

		for (o = st->ChList, ox = o_sent (st, Station); o != NULL;) {

			if (o->Class != OBJ_eobject) {
				o = o->next;
				continue;
			}

			so = o->next;

			pool_out (o);
			o->prev = ox;
			o->next = ox->next;
			if (o->next != NULL) o->next->prev = o;
			ox->next = o;
			ox = o;
			o = so;
		}
	}

	// Sort the Kernel's queue

	ox = Kernel->ChList;
	// At least Root should be there
	Assert (ox != NULL,
		"Adjusting ownership tree: Kernel's ChList is empty");
	ox->prev = zz_hptr (ox, ZZ_Object); // Make it consistent
	Kernel->ChList = NULL;

	// Find Root: it goes first

	for (o = ox; o != NULL && o != ZZ_Main; o = o->next);

	if (o == NULL)
	   excptn ("Adjusting ownership tree: Root missing from Kernel's list");

	pool_out (o);
	pool_in (o, Kernel->ChList, ZZ_Object);

	// Now for system processes

	for (o = ox; o != NULL; ) {
		if (o->Class != AIC_process || ((Process*)o)->Owner != System) {
			o = o->next;
			continue;
		}

		// A system process
		so = o->next;
		pool_out (o);
		pool_in (o, Kernel->ChList, ZZ_Object);
		o = so;
	}

	// Now for the Root's list

	for (o = ZZ_Main->ChList, ox = o_sent (ZZ_Main, Process); o != NULL;) {

		// Observers first

		if (o->Class != AIC_observer) {
			o = o->next;
			continue;
		}

		so = o->next;

		pool_out (o);
		o->prev = ox;
		o->next = ox->next;
		if (o->next != NULL) o->next->prev = o;
		ox->next = o;
		ox = o;
		o = so;
	}

#if	ZZ_NOS
	for (o = ZZ_Main->ChList, ox = o_sent (ZZ_Main, Process); o != NULL;) {

		// Then RVariables

		if (o->Class != OBJ_rvariable) {
			o = o->next;
			continue;
		}

		so = o->next;

		pool_out (o);
		o->prev = ox;
		o->next = ox->next;
		if (o->next != NULL) o->next->prev = o;
		ox->next = o;
		ox = o;
		o = so;
	}
#endif

	// Whatever remains, it must be level-0 user-defined displayable
	// objects unasigned to any station; so they should stay right where
	// they are.
#ifdef  CDEBUG
	// zz_dumpTree (System);
#endif
}

void    ZZ_Object::zz_start () {

/* --------------- */
/* This is a dummy */
/* --------------- */

	excptn ("Object: Id = %1d, Class = %1d, cannot create directly objects "
		"of this class", Id, Class);
}

char *ZZ_Object::getNName () {

/* ----------------------------- */
/* Returns the object's nickname */
/* ----------------------------- */

	return (zz_nickname);
}

void ZZ_Object::setNName (const char *nn) {

/* ------------------ */
/* Sets up a nickname */
/* ------------------ */

	if (zz_nickname != NULL) delete zz_nickname;
	zz_nickname = new char [strlen (nn) + 1];
	strcpy (zz_nickname, nn);
};

char *ZZ_Object::getOName () {

/* ----------------------------------- */
/* Returns the object's printable name */
/* ----------------------------------- */

	char    *s;

	if ((s = getNName ()) != NULL)
		return (s);
	else
		return (getSName());
}

char *ZZ_Object::getSName () {

/* ---------------------------------- */
/* Returns the object's standard name */
/* ---------------------------------- */

	char    *sn;
	Long    sid;
	
	if (Id == NONE) return (getTName ());

	if (
#if	ZZ_NOL
            Class == AIC_port ||
#endif
#if	ZZ_NOR
	    Class == AIC_transceiver ||
#endif
           (Class == AIC_mailbox && ((Mailbox*)this)->nextm != (void*)NONE) ) {

		// To be treated in a special way
		if ((sid = GSID (Id)) == GSID_NONE)
			sn = System->getSName ();
			// Ports don't belong to the System station, but
			// mailboxes may
		else
			sn = idToStation (sid)->getSName ();

		return (form ("%s %1d at %s", getTName (), GYID (Id), sn));
	} else
		// Compose name and Id
		return (form ("%s %1d", getTName (), Id));
}

char *ZZ_Object::getBName () {

/* --------------------------------------- */
/* Returns the object's base standard name */
/* --------------------------------------- */

	char    *sn;
	Long    sid;
	
	switch (Class) {

		case AIC_timer:         // All these objects are not
		case AIC_monitor:
#if	ZZ_NOC
		case AIC_client:        // extensible by the user
#endif
#if	ZZ_NOL
		case AIC_port:
#endif
#if	ZZ_NOR
		case AIC_transceiver:
#endif
#if	ZZ_NOS
		case OBJ_rvariable:
#endif
			return (getSName ());

		case AIC_mailbox:

                  if (((Mailbox*)this)->nextm != (void*)NONE) {
		    if ((sid = GSID (Id)) == GSID_NONE)
			sn = System->getSName ();
			// Mailboxes may belong to the System station
		    else
			sn = idToStation (sid)->getSName ();
		    return (form ("Mailbox %1d at %s", GYID (Id), sn));
                  }

                  return (form ("Mailbox %1d", Id));

#if	ZZ_NOL
		case AIC_link:

			return (form ("Link %1d", Id));
#endif

#if	ZZ_NOR
		case AIC_rfchannel:

			return (form ("RFChannel %1d", Id));
#endif

#if	ZZ_NOC
		case AIC_traffic:

			return (form ("Traffic %1d", Id));
#endif
		case AIC_process:

			return ((this == Kernel) ? getSName () :
				form ("Process %1d", Id));

		case AIC_observer:

			return (form ("Observer %1d", Id));

		case OBJ_station:

			return ((this == System) ? getSName () :
				form ("Station %1d", Id));

		case OBJ_eobject:

			return (form ("EObject %1d", Id));

		default:

			excptn ("getBName: illegal object class %1d", Class);
			return (NULL);
	}
}

void    EObject::zz_start () {

/* ---------------------------------------------------------- */
/* Nonstandard  constructor  for  user-definable  displayable */
/* objects                                                    */
/* ---------------------------------------------------------- */

	Id = sernum++;                  // Update numerical Id

	Class = OBJ_eobject;

	if (TheProcess == ZZ_Main && TheStation != NULL &&
		TheStation != System) {

		// Created at level 0 -- assign to the user station

		assert (!zz_flg_started,
		 "EObject: %s, cannot create from Root after protocol has "
		   "started", getSName ());

		pool_in ((ZZ_Object*)this, TheStation->ChList, ZZ_Object);

	} else {

		// Not level 0 -- assign to the rightful owner, i.e. the
		// process that creates the object

#if     ZZ_OBS
		if (zz_observer_running) {
			// Observers are also allowed to create dynamic
			// displayable objects
			pool_in (this, zz_current_observer->ChList,
				ZZ_Object);
		} else
#endif
		{
			pool_in (this, TheProcess->ChList, ZZ_Object);
		}
	}

	// Note: We assume that DisplayActive cannot be set before Root
	// initializes things
};

EObject::~EObject () {

/* ----------------------- */
/* Destructor for EObjects */
/* ----------------------- */

	pool_out (this);
        zz_DREM (this);
	if (zz_nickname != NULL) delete (zz_nickname);
};

Long    zz_trunc (LONG a, int b) {

/* ---------------------------------------------------------- */
/* Truncates  a  to  fit  into  a specified number of decimal */
/* positions                                                  */
/* ---------------------------------------------------------- */

	LONG    c;

	switch (b) {

		case  1: c = 10; break;
		case  2: c = 100; break;
		case  3: c = 1000; break;
		case  4: c = 10000; break;
		case  5: c = 100000; break;
		case  6: c = 1000000; break;
		case  7: c = 10000000; break;
		case  8: c = 100000000; break;
		case  9: c = 1000000000; break;
		default:
#if	__WORDSIZE <= 32
         	     c = LCS(10000000000); break;
#else
			return a;
#endif
	}

	if (a >= 0) return ((Long) (a % c));
	return ((Long) (- ((-a) % (c / 10))));
}

const char    *AI::zz_eid (LPointer eid) {

/* ---------------------------------------------------------- */
/* Returns  textual  event  identifier  for  a  given  AI and */
/* numeric event id                                           */
/* ---------------------------------------------------------- */
#if __WORDSIZE <= 32
        char tmp [12];
#endif
        switch (Class) {

                case AIC_timer:

                        if (eid == NONE)
                                return ("wakeup");
                        else {
#if __WORDSIZE <= 32
                                encodeLong (eid, tmp, 10);
                                return (form ("%s", tmp));
#else
                                return (form ("%1d", zz_trunc (eid, 10)));
#endif
                        }

		case AIC_monitor:

			return (form ("%08x", (unsigned int) eid));

		case AIC_mailbox:

			if (((Mailbox*)this)->getLimit () >= 0) {
                        	switch (eid) {
                        		case SENTINEL : return ("SENTINEL");
                          		case RECEIVE  : return ("RECEIVE");
                          		case NONEMPTY : return ("NONEMPTY");
                          		case NEWITEM  : return ("NEWITEM");
                          		case GET      : return ("GET");
                          		case EMPTY    : return ("EMPTY");
                          		case OUTPUT   : return ("OUTPUT");
				}
			}
                        return (form ("%1d", zz_trunc (eid, 10)));

#if	ZZ_NOL
		case AIC_link:

			switch (eid) {

				case ARC_PURGE:
					return ("ARC_PURGE");
				case LNK_PURGE:
					return ("LNK_PURGE");
				case BOT_HEARD:
					return ("BOT_HEARD");
				case EOT_HEARD:
					return ("EOT_HEARD");
				default:
					return ("unknown");
			}

		case AIC_port:

			switch (eid) {

				case SILENCE:
					return ("SILENCE");
				case ACTIVITY:
					return ("ACTIVITY");
				case COLLISION:
					return ("COLLISION");
				case BOJ:
					return ("BOJ");
				case EOJ:
					return ("EOJ");
				case BOT:
					return ("BOT");
				case EOT:
					return ("EOT");
				case BMP:
					return ("BMP");
				case EMP:
					return ("EMP");
				case ANYEVENT:
					return ("ANYEVENT");
				default:
					return ("unknown");
			}
#endif	/* NOL */

#if	ZZ_NOR
		case AIC_transceiver:

			/*
			 * Note that administrative events are handled by
			 * transceivers rather than RFChannels
			 */
			switch (eid) {
				case SILENCE:
					return ("SILENCE");
				case ACTIVITY:
					return ("ACTIVITY");
				case BOP:
					return ("BOP");
				case EOP:
					return ("EOP");
				case BOT:
					return ("BOT");
				case EOT:
					return ("EOT");
				case BMP:
					return ("BMP");
				case EMP:
					return ("EMP");
				case ANYEVENT:
					return ("ANYEVENT");
				case BERROR:
					return ("BERROR");
				case INTLOW:
					return ("INTLOW");
				case INTHIGH:
					return ("INTHIGH");
				case SIGLOW:
					return ("SIGLOW");
				case SIGHIGH:
					return ("SIGHIGH");
				case DO_ROSTER:
					return ("DO_ROSTER");
				case RACT_PURGE:
					return ("RACT_PURGE");
				case BOT_TRIGGER:
					return ("BOT_TRIGGER");
				default:
					return ("unknown");
			}
#endif	/* NOR */

#if	ZZ_NOC
		case AIC_client:
		case AIC_traffic:

			switch (eid) {

				case ARR_MSG:
					return ("ARR_MSG");  // Internal
				case ARR_BST:
					return ("ARR_BST");  // Internal
				case SUSPEND:
					return ("SUSPEND");
				case RESUME:
					return ("RESUME");
				case RESET:
					return ("RESET");
				case ARRIVAL:
					return ("ARRIVAL");
				case INTERCEPT:
					return ("INTERCEPT");
				default:
					return (form ("arr_Trf%03d",
						zz_trunc (eid, 3)));
			};
#endif	/* NOC */

		case AIC_process:

			switch (eid) {
#ifdef STALL
				case STALL:
				    return ("STALL");
#endif
				case DEATH:
				    return ("DEATH");
				case CHILD:
				    return ("CHILD");
				case START:
				    return ("START");
				case SIGNAL:
				    return ("SIGNAL");
                                case CLEAR:
                                    return ("CLEAR");
				default:
				    return (((Process*)this)->zz_sn ((int)eid));
			}

#if     OBSERVERS
		case AIC_observer:

			if (eid == OBS_TSERV)
				return ("TIMEOUT");
			else
				return ("unknown");
#endif
		default:
			return ("unknown");
	}
}

Long  AI::zz_aid () {

/* ------------------------------------------------- */
/* Returns absolute Id of the AI (used in exposures) */
/* ------------------------------------------------- */

	if (
#if	ZZ_NOL
             Class == AIC_port ||
#endif
#if	ZZ_NOR
             Class == AIC_transceiver ||
#endif
                                   Class == AIC_mailbox)
		return (GYID (Id));
	else
		return (Id);
};

#if 	ZZ_NOL
int Port::getSID () { return GSID (Id); }
int Port::getYID () { return GYID (Id); }
#endif

#if 	ZZ_NOR
int Transceiver::getSID () { return GSID (Id); }
int Transceiver::getYID () { return GYID (Id); }
#endif

/* --------------- */
/* Dummy exposures */
/* --------------- */

sexposure (ZZ_Object)

	excptn ("Object->exposure: Id = %1d, Class = %1d, Object class cannot "
		"be exposed directly", Id, Class);
	USEEMD;
	USESID;
}

sexposure (AI)

	excptn ("AI->exposure: Id = %1d, Class = %1d, AI class cannot be "
		"exposed directly", Id, Class);
	USEEMD;
	USESID;
}
