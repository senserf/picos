/* ooooooooooooooooooooooooooooooooooooo */
/* Copyright (C) 1991-06   P. Gburzynski */
/* ooooooooooooooooooooooooooooooooooooo */

/* --- */

/* ------------------------------- */
/* Internal offsets for RVariables */
/* ------------------------------- */

#define MAX_MOMENTS     32   // Should content everybody
#define MAX_REGION      24   // The number of last RV mean samples for display

#if     BIG_precision > 0
#define BIG_SIZE        BIG_precision
#else
#define BIG_SIZE        ((sizeof(double) + sizeof(LONG) - 1) / sizeof (LONG))
#endif

#define STYPE                           0               // The type
#define COUNTER                         1               // The counter
#define MIN             ((sizeof(LONG)*2+sizeof(double)-1)/sizeof(double))
#define MAX             (MIN+1)                         // Min/ Max
#define MOMENT          (MAX+1)                         // Moments

/* ----------------------- */
/* For BIG type RVariables */
/* ----------------------- */
#define BCOUNTER        1

#define BMIN ((sizeof(LONG)*(BCOUNTER+BIG_SIZE)+sizeof(double)-1)/sizeof(double))
#define BMAX            (BMIN+1)
#define BMOMENT         (BMAX+1)

/* ---------------------------------------- */
/* Macros to reference RVariable attributes */
/* ---------------------------------------- */
#define stype(s)        ((s)[STYPE])
#define counter(s)      ((s)[COUNTER])
#define minimum(s)      (((double*) (s))[MIN])
#define maximum(s)      (((double*) (s))[MAX])
#define moment(s,i)     (((double*) (s))[MOMENT+(i)])

/* --------------------------------- */
/* The same for a BIG-type RVariable */
/* --------------------------------- */
#define bcounter(s)     (*(BIG*)(&((s)[BCOUNTER])))
#define bminimum(s)     (((double*) (s))[BMIN])
#define bmaximum(s)     (((double*) (s))[BMAX])
#define bmoment(s,i)    (((double*) (s))[BMOMENT+(i)])

/* ------------------------------------------------------ */
/* Values of Z_alpha for calculating confidence intervals */
/* ------------------------------------------------------ */
#define ZALPHA95        1.960
#define ZALPHA99        2.575
