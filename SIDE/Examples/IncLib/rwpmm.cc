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

static inline double rwpmm_dist (  double  x, double  y 
#if ZZ_R3D
				 , double  z
#endif
				 , double xt, double yt
#if ZZ_R3D
				 , double zt
#endif
							) {

	return sqrt ((xt - x) * (xt - x) + (yt - y) * (yt - y)
#if ZZ_R3D
					 + (zt - z) * (zt - z)
#endif
								);
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
#if ZZ_R3D
			double z0,
#endif
			double x1,
			double y1,
#if ZZ_R3D
			double z1,
#endif
			double mnsp,
			double mxsp,
			double mnpa,
			double mxpa,
			double dur,
			int mode)		{

	if (!isStationId (nid))
		excptn ("rwpmmStart: illegal node Id %1d", nid);

	if (x0 < 0.0 || x1 < x0 || y0 < 0.0 || y1 < y0
#if ZZ_R3D
				|| z0 < 0.0 || z1 < z0
#endif
							)
		excptn ("rwpmmStart: node %1d, illegal bounding box", nid);

	if (mnsp <= 0.0 || mxsp < mnsp)
		excptn ("rwpmmStart: node %1d, illegal speed", nid);

	if (mnpa < 0.0 || mxpa < mnpa)
		excptn ("rwpmmStart: node %1d, illegal pause time", nid);

	if (mode < 0 || mode > RWPMM_MODE_MAX)
		excptn ("rwpmmStart: node %1d, illegal mode", nid);

	rwpmmStop (rfm);

	create RWPMover (nid, rfm, x0, y0, 
#if ZZ_R3D
				       z0,
#endif
				       x1, y1,
#if ZZ_R3D
					   z1,
#endif
					       mnsp, mxsp, mnpa, mxpa, dur,
							mode);
}

void RWPMover::setup (Long nn, Transceiver *rfm,
						double x0,
						double y0,
#if ZZ_R3D
						double z0,
#endif
						double x1,
						double y1,
#if ZZ_R3D
						double z1,
#endif
						double minsp,
						double maxsp,
						double minpa,
						double maxpa,
						double howlong,
						int mode) {

	ME = new rwpmm_pool_t;
	ME->RFM = TR = rfm;
	ME->Thread = this;
	NID = nn;
	pool_in (ME, rwpmm_pool);

	X0 = x0;
	Y0 = y0;
	X1 = x1;
	Y1 = y1;
#if ZZ_R3D
	Z0 = z0;
	Z1 = z1;
#endif
	MINSP = minsp;
	MAXSP = maxsp;

	MINPA = minpa;
	MAXPA = maxpa;

	Mode = mode;

	if (howlong <= 0.0)
		Until = TIME_inf;
	else
		Until = Time + etuToItu (howlong);

	TR->getLocation (TX, TY
#if ZZ_R3D
			   , TZ
#endif
				);
}

RWPMover::perform {

    TIME delta;
    double cn, sp;

    state NextLeg:

	// Current coordinates
	CX = TX;
	CY = TY;
#if ZZ_R3D
	CZ = TZ;
#endif

	switch (Mode) {

		case RWPMM_MODE_RANDOM:

			// Generate a random destination within the rectangle
			TX = dRndUniform (X0, X1);
			TY = dRndUniform (Y0, Y1);
#if ZZ_R3D
			TZ = dRndUniform (Z0, Z1);
#endif
			break;

		case RWPMM_MODE_CIRCUMFERENCE:

			if (X0 == X1) {
VLine:
				TX = X0;
				TY = (TY == Y0) ? Y1 : Y0;
			} else if (Y0 == Y1) {
HLine:
				TY = Y0;
				TX = (TX == X0) ? X1 : X0;
			} else if (TX == X0) {
				if (TY == Y1)
					TX = X1;
				else
					TY = Y1;
			} else if (TY == Y0) {
				TX = X0;
			} else {
				TX = X1;
				TY = Y0;
			}
#if ZZ_R3D
			TZ = Z0;
#endif
			break;

		default:

			// Diagonal
			if (X0 == X1)
				goto VLine;
			if (Y0 == Y1)
				goto HLine;

			if (TX == X0) {
				if (TY == Y0) {
					TX = X1;
					TY = Y1;
				} else {
					TY = Y0;
				}
			} else {
				if (TY == Y0) {
					TX = X0;
					TY = Y1;
				} else {
					TX = X1;
					TY = Y0;
				}
			}
#if ZZ_R3D
			TZ = Z0;
#endif
	}
				
	// Distance in meters
	sp = rwpmm_dist (TX, TY 
#if ZZ_R3D
			   , TZ
#endif
		       , CX, CY
#if ZZ_R3D
			   , CZ
#endif
				);

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

	TR->setLocation (CX, CY
#if ZZ_R3D
			   , CZ
#endif
				);

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
#if ZZ_R3D
		CZ = TZ;
#endif
		Timer->wait (TLeft, Advance);
		sleep;
	}

	// Remaining distance in meters
	sp = rwpmm_dist (CX, CY
#if ZZ_R3D
			   , CZ
#endif
		       , TX, TY
#if ZZ_R3D
			   , TZ
#endif
				);
	if (sp < 0.0001) {
		// Just in case
		CX = TX;
		CY = TY;
#if ZZ_R3D
		CZ = TZ;
#endif
	} else {
		// Distance fraction for next step
		cn =  sp / Count;
		CX += cn * (TX - CX) / sp;
		CY += cn * (TY - CY) / sp;
#if ZZ_R3D
		CZ += cn * (TZ - CZ) / sp;
#endif
	}

	// Current interval
	delta = (TIME) ((double) TLeft / Count);
	Count--;
	TLeft -= delta;
	Timer->wait (delta, Advance);
	sleep;
}

#endif
