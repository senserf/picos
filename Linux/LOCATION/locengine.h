#ifndef	__locengine_h
#define	__locengine_h

//
// Copyright (C) 2008 Olsonet Communications Corporation
//
// PG March 2008
//

#include <math.h>
#include "ntypes.h"

// #define	DEBUGGING

typedef	struct {
//
// Association list entry
//
	u32	Peg;
	float	SLR;
} pegitem_t;

typedef	struct {
//
// This represents a single point in the database
//

	float	x, y;		// Add 'z' for 3-d

	// This one is for extra properties, zero may mean 'ignore in matching';
	// sample properties: xmit power, frequency
	u32	properties;

	// Association list length
	u16	NPegs;

	// The association list itself
	pegitem_t	Pegs [0];

} tpoint_t;

// Gives the actual size (for malloc) of the tpoint structure:
//	- when we have the structure
#define	tpoint_size(tp)	(sizeof (tpoint_t) + (tp)->NPegs * sizeof (pegitem_t))
//	- when we want to allocate memory for it
#define tpoint_tsize(n)	(sizeof (tpoint_t) + (n) * sizeof (pegitem_t))

// Checks if the point falls within the specified rectangle
#define tpoint_inrec(x0,y0,x1,y1,p) \
	(x0 <= (p)->x && x1 >= (p)->x && y0 <= (p)->y && y1 >= (p)->y)

// Distance between a pairs of points (coordinate pairs)
#define	dist(x0,y0,x1,y1) (sqrt (((x0)-(x1))*((x0)-(x1)) + \
						((y0)-(y1))*((y0)-(y1))))
// ========================= //
// Database access functions //
// ========================= //

// Opens the database, returns 1-old, 0-new
int db_open (const char *);

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

// Find all points that fall into the specified rectangle
int loc_findrect (float x0, float y0, float x1, float y1, tpoint_t **, int);

// Do the location tracking
int loc_locate (int K, u32 *pg, float *v, int N, u32 prop, float *X, float *Y);

#endif
