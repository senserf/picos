/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2003                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

heapmem {100};

#include "lcd.h"
#include "ser.h"
#include "form.h"

#define RS_INIT  00
#define RS_RCMD  10
#define RS_LCD  20
#define RS_PRIO  30

#define IBUFSIZE 128


process (A, void)

  nodata;

  entry (RS_INIT)
     upd_lcd ("1");

  entry (RS_INIT+1)
     finish;

endprocess (3)


process (B, void)

  nodata;

  entry (RS_INIT)
      upd_lcd ("2");

  entry (RS_INIT+1)
     finish;

endprocess (3)


process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

 static char *ibuf = NULL;
 static word p1, p2, pid;

 nodata;

 entry (RS_INIT)

         if (ibuf == NULL)
              ibuf = (char*) umalloc (IBUFSIZE);

 entry (RS_RCMD-1)

     ser_out (RS_RCMD-1, "\r\n"
  "          'p p1 p2 (priority test)\r\n" );

 entry (RS_RCMD)

     ser_in (RS_RCMD, ibuf, IBUFSIZE);

     if (ibuf [0] == 'p')
          proceed (RS_PRIO);

 entry (RS_LCD-1)

     ser_out (RS_LCD-1, "Illegal command or parameter\r\n");
     proceed (RS_RCMD-1);

 entry (RS_PRIO)

     if (scan (ibuf + 1, "%u %u", &p1, &p2) != 2)
          proceed (RS_LCD-1);

     diag ("Changing priority %d ", prioritize(0,-1));
     diag ("for %d root process(es) to ", prioritizeall(root,p1));
     diag ("%d", prioritize(0,-1));
     diag ("Changing priority to %d", prioritize(0,p2) );
     diag ("The new priority is %d", prioritize(0,-1));

 entry (RS_PRIO+1)

    pid =fork (A, NULL);

    diag ("Changing priority %d ", prioritize(pid,-1));
    diag ("for %d A process(es) to ", prioritizeall(A,p1));
    diag ("%d", prioritize(pid,-1));
    diag ("Changing priority to %d", prioritize(pid,p1+1) );
    diag ("The new priority is %d", prioritize(pid,-1));

entry (RS_PRIO+2)

    pid =fork (B, NULL);

    diag ("Changing priority %d ", prioritize(pid,-1));
    diag ("for %d B process(es) to ", prioritizeall(B,p2));
    diag ("%d", prioritize(pid,-1));
    diag ("Changing priority to %d", prioritize(pid,p2+1) );
    diag ("The new priority is %d", prioritize(pid,-1));

    proceed (RS_RCMD);

endprocess (1)
