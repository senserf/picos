/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */


/*============================================================================= */
/* FILE SimNet.h                                                                */
/*    Common header.                                                            */
/* =============================================================================*/

#define SIM_NET 1

#include "..\PicOS\eCOG\ethernet.h"
#include "..\PicOS\eCOG\sysio.h"
#include "ecog1.h"

extern volatile __rg_t rg;

#define SHMEMSIZE (sizeof(shmemType))

#define MAX_MSG_LEN 256

typedef	struct bufStruct {
  int currIndex; //for writing out the buffer, i.e. head
  int msgLen;//tail, for inputting into the buffer
  char msgBuf[MAX_MSG_LEN];
} uartt;

#define NUARTS 4

typedef struct framestruct {
  word frame [ETH_MXLEN] ;
  int i1; //current index/word
  int i2; //length of frame in words
  word destMAC [3]; //MAC address
} framet;

#define MAXFIFO 128 //later can do in sync with the chip FIFO
#define MAXDESTS 64
#define MAXRADIO 4096 //bytes

typedef struct deststruct {
  word destMAC [3]; //MAC address
  framet RXframesFIFO [MAXFIFO] ; //FIFO of frames for a particular destination (i.e.rx FIFO for dest)
  //indices for the circular buffer above
  int i1; //tail
  int i2; //head
  word RCR;//Rx Ctrl Reg

  char radio [MAXRADIO];
  int r1; //tail
  int r2; //head
  int rlag;
  int x, y; //coordinates for RADIO
  int range; //sender's radio range

} destt;


#define incrFIFO(p,size)       do { \
        if (++(p) ==size) (p) = 0; \
        } while (0)

#define isFIFOempty(d,i1,i2)       (d.i1==d.i2)


typedef	struct shmemStruct {

  destt dests [MAXDESTS] ;
  int ndests; //current number of destinations

} shmemType;

#define destination(i) (shmemPtr->dests[i])
#define ndestinations (shmemPtr->ndests)


extern shmemType		* shmemPtr;
extern uartt			uart[];

#define NOTFOUND -1

#define DEBUG 0

#if DEBUG
#define  _ui_output_ws  ui_output_ws
#else
#define  _ui_output_ws  noop
#endif
#undef DEBUG

noop (const char *format, ...) ; //{ }

