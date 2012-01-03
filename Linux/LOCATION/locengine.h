#ifndef	__locengine_h
#define	__locengine_h

//
// Copyright (C) 2008-2012 Olsonet Communications Corporation
//
// PG March 2008, revised December 2011, January 2012
//

#include <math.h>
#include "ntypes.h"

#define	DBVER		1		// Current version number

// Default file names
#define	DBNAME	"points.db"
#define	PMNAME	"params.xml"

#define	EPS	0.000001

typedef	struct {
//
// Association list entry
//
	u32	Peg;
	u16	RSSI;
	float	SLR;
} alitem_t;

typedef	struct {
//
// This represents a single point in the database
//
	float		x, y;		// Add 'z' for 3-d

	u32		Tag;

	// Bit 31   = mobile, i.e., set for a true Tag, 0 means Peg (as in
	//	      autoprofiling)
	//
	// Bits 0-2 = power level
	//
	u32		properties;

	// Association list length
	u16		NPegs;

	// The association list itself
	alitem_t	Pegs [0];

} tpoint_t;

// Gives the actual size (for malloc) of the tpoint structure:
//	- when we have the structure
#define	tpoint_size(tp)	(sizeof (tpoint_t) + (tp)->NPegs * sizeof (alitem_t))
//	- when we want to allocate memory for it
#define tpoint_tsize(n)	(sizeof (tpoint_t) + (n) * sizeof (alitem_t))

// Checks if the point falls within the specified rectangle
#define tpoint_inrec(x0,y0,x1,y1,p) \
	(x0 <= (p)->x && x1 >= (p)->x && y0 <= (p)->y && y1 >= (p)->y)

// Distance between a pair of points (coordinate pairs)
#define	dist(x0,y0,x1,y1) (sqrt (((x0)-(x1))*((x0)-(x1)) + \
						((y0)-(y1))*((y0)-(y1))))

#define	PROP_AUTOPROF	0x80000000
#define	PROP_ENVMASK	0x7fffffff

// Called when out of memory
void oom (const char*);

// ========================= //
// Database access functions //
// ========================= //

// Opens the database, returns 1-old, 0-new:
//	arg0 == database file name
//	arg1 == parameters (XML) file name
int db_open (const char *, const char *);

// Closes the database (writing it back, if modified)
void db_close (void);

// Start reading all points perceptible by the given Peg (if Peg != 0), or
// simply all points in the database (if Peg == 0)
void db_start_points (u32 peg);

// Advance to the next point. This is to be called after db_get_point to make it
// read the next point the next time around. If all point from the selection
// have been exhausted, db_next_point will set the cursor to NULL.
void db_next_point (void);

// Read the current point (the one pointed to by the cursor). Returns the point
// pointer or NULL, if there are no more points.
tpoint_t *db_get_point (void);

// Delete the current point
void db_delete_point (tpoint_t*);

// Add a point to the database
void db_add_point (tpoint_t*);

// Sort association lists by Pegs
void db_sort_al (alitem_t*, int);

// Find all points that fall into the specified rectangle
int loc_findrect (float x0, float y0, float x1, float y1, tpoint_t **, int);

//
// Do the location estimation:
//
//	- the Tag being located (sending the query)
//	- the query association list
//	- length of the association list
//	- properties
//	- returned X
//	- returned Y
//
int loc_locate (u32 tag, alitem_t *al, int N, u32 pro, float *X, float *Y);

float rssi_to_slr (u16);

#include "params.h"

#endif
