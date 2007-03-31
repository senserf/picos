#ifndef __rwpmm_c__
#define __rwpmm_c__

// Random waypoint mobility model

#include "rwpmm.h"

static rwpmm_notifier_t	rwpmm_notifier = NULL;	// Notifier function

static rwpmm_pool_t *rwpmm_pool = NULL;		// List of moves in progress

static rwpmm_pool_t *rwpmm_find_mip (Transceiver *t) {

	rwpmm_pool_t *p;

	for_pool (p, rwpmm_pool)
		if (p->RFM == t)
			return p;
	return NULL;
}

static inline double rwpmm_dist (double x, double y, double xt, double yt) {

	return sqrt ((xt - x) * (xt - x) + (yt - y) * (yt - y));
}

void rwpmmSetNotifier (rwpmm_notifier_t n) {

	rwpmm_notifier = n;
}

Boolean rwpmmMoving (Transceiver *t) {

	return rwpmm_find_mip (t) != NULL;
}

Boolean rwpmmStop (Transceiver *t) {

	rwpmm_pool_t *m;

	if ((m = rwpmm_find_mip (t)) != NULL) {
		m->Thread->terminate ();
		pool_out (m);
		delete m;
		return YES;
	}

	return NO;
}

void rwpmmStart (Long nid, Transceiver *rfm,

			double x0,
			double y0,
			double x1,
			double y1,
			double mnsp,
			double mxsp,
			double mnpa,
			double mxpa,
			double dur)		{

	if (!isStationId (nid))
		excptn ("rwpmmStart: illegal node Id %1d", nid);

	if (x0 < 0.0 || x1 < x0 || y0 < 0.0 || y1 < y0)
		excptn ("rwpmmStart: node %1d, illegal bounding rectangle",
			nid);

	if (mnsp <= 0.0 || mxsp < mnsp)
		excptn ("rwpmmStart: node %1d, illegal speed", nid);

	if (mnpa < 0.0 || mxpa < mnpa)
		excptn ("rwpmmStart: node %1d, illegal pause time", nid);

	rwpmmStop (rfm);

	create RWPMover (nid, rfm, x0, y0, x1, y1, mnsp, mxsp, mnpa, mxpa, dur);
}

void RWPMover::setup (Long nn, Transceiver *rfm,
						double x0,
						double y0,
						double x1,
						double y1,
						double minsp,
						double maxsp,
						double minpa,
						double maxpa,
						double howlong) {

	ME = new rwpmm_pool_t;
	ME->RFM = TR = rfm;
	ME->Thread = this;
	NID = nn;
	pool_in (ME, rwpmm_pool);

	X0 = x0;
	Y0 = y0;
	X1 = x1;
	Y1 = y1;

	MINSP = minsp;
	MAXSP = maxsp;

	MINPA = minpa;
	MAXPA = maxpa;

	if (howlong <= 0.0)
		Until = TIME_inf;
	else
		Until = Time + etuToItu (howlong);

	TR->getLocation (TX, TY);
}

RWPMover::perform {

    TIME delta;
    double cn, sp;

    state NextLeg:

	// Current coordinates
	CX = TX;
	CY = TY;

	// Generate a random destination within the rectangle
	TX = dRndUniform (X0, X1);
	TY = dRndUniform (Y0, Y1);

	// Distance in meters
	sp = rwpmm_dist (TX, TY, CX, CY);

	// Target number of steps (tiny teleportations)
	cn = sp / RWPMM_TARGET_STEP;

	// Total time per move in ITUs
	TLeft = etuToItu (sp / dRndUniform (MINSP, MAXSP));

	// We try to make it as smooth as feasible, but no more than that;
	// Count is the total number of steps
	if (cn > RWPMM_MAX_STEPS)
		Count = RWPMM_MAX_STEPS;
	else if (cn < RWPMM_MIN_STEPS)
		Count = RWPMM_MIN_STEPS;
	else
		Count = (Long) cn;

    transient Advance:

	TR->setLocation (CX, CY);

	if (rwpmm_notifier != NULL)
		(*rwpmm_notifier) (NID);

	if (Time >= Until) {
		// We are done
Finish:
		pool_out (ME);
		delete ME;
		terminate;
	}

	if (Count == 0) {
		// End of leg
		delta = etuToItu (dRndUniform (MINPA, MAXPA));
		if (delta == TIME_0)
			proceed NextLeg;
		if (def (Until) && Time + delta >= Until)
			// This will get is beyond termination
			goto Finish;
		Timer->wait (delta, NextLeg);
		sleep;
	}
		
	// Calculate next step
	if (Count == 1) {
		// Make sure we always end up where we wanted to get
		Count = 0;
		CX = TX;
		CY = TY;
		Timer->wait (TLeft, Advance);
		sleep;
	}

	// Remaining distance in meters
	sp = rwpmm_dist (CX, CY, TX, TY);
	if (sp < 0.0001) {
		// Just in case
		CX = TX;
		CY = TY;
	} else {
		// Distance fraction for next step
		cn =  sp / Count;
		CX += cn * (TX - CX) / sp;
		CY += cn * (TY - CY) / sp;
	}

	// Current interval
	delta = (TIME) ((double) TLeft / Count);
	Count--;
	TLeft -= delta;
	Timer->wait (delta, Advance);
	sleep;
}

#endif
