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

/*=============================================================================

FILE SimNetIO.c
   File with the simulator's IO.

DESCRIPTION
   This software simulates ad hoc networks of Olsonet nodes
   implemented on Cyan board. It is meant to morph into a tool for prototyping
   and verifying applications built with Olsonet's framework for
   ad hoc networking applications.

=============================================================================*/

#include <windows.h>
#include <stdio.h>

#include "global.h"
#include "cli.h"
#include "core.h"
#include "ui.h"

#include "SimNet.h"

volatile __rg_t rg;

#define rgtype unsigned int

#define rgaddr  0xfea0
#define rg2addr(r)  ( ( (int) & (r) - (int) &rg) / sizeof (rgtype) + rgaddr )

noop (const char *format, ...) { }

//--------------

// Simulator IO System API
EXPORT int		sim_init	(void);
EXPORT int		sim_reset	(void);
EXPORT coremem 	sim_d_read  (coreadr adr,
				    	bool byte_access,
			    		bool proc_access,
			    		lword * cycles);
EXPORT int		sim_d_write (coreadr adr,
			 			coremem val,
			    		bool byte_access,
			    		bool proc_access,
			    		lword * cycles);
EXPORT coremem 	sim_p_read  (coreadr adr,
						bool proc_access,
			    		lword * cycles);
EXPORT int		sim_p_write (coreadr adr,
						coremem val,
			    		bool proc_access,
			    		lword * cycles);
EXPORT int		sim_sif	(void);
EXPORT int		sim_clock	(lword cycles);
EXPORT int		sim_check_irq (coreadr * vector);

static lword 			proc_cycles;

//----------------------------------------------------------------------

static word ourMACaddr [3] = {NOTFOUND, NOTFOUND, NOTFOUND} ; //our MAC address
extern int  ourMACindex = NOTFOUND;

//add a new one to the list of destinations
//return the new index
int newdest () {

  int i=ndestinations;

  destination(i).destMAC[0]=i;
  destination(i).destMAC[1]=i;
  destination(i).destMAC[2]=i;

  //this destination's RXFIFO is empty yet
  destination(i).i1= destination(i).i2= 0;
  destination(i).RCR =0;

  destination(i).r1= destination(i).r2= 0;
  destination(i).x= destination(i).y= i;
  destination(i).range=2;

  ndestinations++;

  return i;
}

#define range  destination(ourMACindex).range

// This function is optional, but used here to create and map shared structs.
// (Windows' stuff, somehow working all right.)

EXPORT int sim_init(void) {
	HANDLE hMapObject = NULL;  //	handle to file mapping
	bool isFirst;

	hMapObject = CreateFileMapping(
		INVALID_HANDLE_VALUE,	// use paging file
		NULL,			// default security attributes
		PAGE_READWRITE,	// read/write access
		0,			// size: high 32-bits
		SHMEMSIZE,		// size: low 32-bits
		"dllmemfilemap");	// name of map object

	if (hMapObject == NULL) {
		ui_output_ws("CreateFileMapping failed\n>");
		return FAILURE;
	}

	isFirst = (GetLastError() != ERROR_ALREADY_EXISTS);

	// Get a pointer to the file-mapped shared memory.
	shmemPtr = (shmemType *) MapViewOfFile(
		hMapObject,	  // object to map view	of
		FILE_MAP_WRITE, // read/write	access
		0,		  // high offset:  map from
		0,		  // low offset:   beginning
		0);		  // default: map entire file

	if (shmemPtr == NULL) {
		ui_output_ws("MapViewOfFile failed\n>");
		return FAILURE;
	}
	if (isFirst) {
		memset(shmemPtr, '\0', SHMEMSIZE);
	}

	ourMACindex= newdest();
	ourMACaddr[0]= ourMACindex;
	ourMACaddr[1]= ourMACindex;
	ourMACaddr[2]= ourMACindex;
	ui_output_ws("---Node Index: %d\n",ourMACindex);
	ui_output_ws("---Coordinate X: %d\n",ourMACindex);
	ui_output_ws("---Coordinate Y: %d\n",ourMACindex);
	ui_output_ws("---Sender Range: %d\n",range);
	ui_output_ws("---Radio Type: %d\n", RADIO_TYPE);

	// init local duart(s) to 0
	memset(uart, 0, sizeof(uartt)<<2);
	return SUCCESS ;
}

// Corresponding shutdown function
// EXPORT void sim_shutdown(void) {}
// should have:
// Unmap shared memory from the process' address space.
// (void)UnmapViewOfFile(lpvMem);
// Close the process' handle to the file-mapping object.
// (void)CloseHandle(hMapObject);
// ***but only when the last node goes away.***
// I don't think we care.




/******************************************************************************
NAME
	sim_reset

SYNOPSIS
	EXPORT int sim_reset( void )

FUNCTION
Called when the reset IO or reset ALL command is typed. Can be used to
initialise the custom IO, and start other processes running. On the
Simulator the processor and custom IO can be reset separately for
flexibility.
This function is required.

RETURNS
	SUCCESS or FAILURE.
******************************************************************************/

EXPORT int sim_reset (void) {
	return SUCCESS ;
}

//---------------------------------BEEPER-------------------------------------

coremem BEEPERr (coreadr adr) {

	if ( adr==rg2addr(rg.ssm.tap_sel2) ) {
	  return rg.ssm.tap_sel2;
	}
	else if ( adr==rg2addr(rg.ssm.rst_set) ) {
	  return rg.ssm.rst_set;
	}

	ui_output_ws("ERROR: BEEPERr called with wrong argument values.\n");
	return FAILURE; //should never happen
}

int BEEPERw (coreadr adr, coremem val) {

  	if ( adr==rg2addr(rg.ssm.tap_sel2) ) {
	  rg.ssm.tap_sel2= val;
	  if (val & SSM_TAP_SEL2_PWM1_MASK) {
	    ui_output_ws ("BEEPER: tone (H'%04X)\n", (val & 0x0f00) >> 8);
	  }
	  return SUCCESS;
	}
	else if ( adr==rg2addr(rg.ssm.rst_set) ) {
	  rg.ssm.rst_set= val;
	  if (val & SSM_RST_SET_PWM1_MASK) {
	    //ui_output_ws ("Stop BEEPER\n");
	  }
	  return SUCCESS;
	}

	ui_output_ws("ERROR: BEEPERw called with wrong argument values.\n");
	return FAILURE; //should never happen
}



//---------------------------------LEDs-------------------------------------

int LEDw (coreadr adr, coremem val) {

 if (adr==rg2addr (rg.io.gp0_3_out)) {

   if (val != rg.io.gp0_3_out) { //LED value's changed
     if (val & 0x3333) {//only for set and clr bits as per leds() in main.c
       ui_output_ws ("\n LED: ");
       ui_output_ws("H'%04X\n", val);
     }
   }
   rg.io.gp0_3_out = val; //need keep track to output LED when changed only
   return SUCCESS;
 }

 ui_output_ws("ERROR: LEDw called with wrong argument values.\n");
 return FAILURE; //should never happen
}//LEDw



//---------------------------------LCD-------------------------------------

coremem LCDr (coreadr adr) {

	if (adr==rg2addr (rg.io.gp8_11_out)) {
	  return rg.io.gp8_11_out ;
	}

	else if (adr==rg2addr (rg.io.gp16_19_out)) {
	  return rg.io.gp16_19_out;
	}

	else if (adr==rg2addr (rg.io.gp20_23_out)) {
	  return rg.io.gp20_23_out;
	}

	 ui_output_ws("ERROR: LCDr called with wrong argument values.\n");
	 return FAILURE; //should never happen
}



void clearstr (char LCDstr[], int l) {

  int i;
  for (i=0; i<l; i++) {
    LCDstr[i]='\0';
  }
}


int LCDw (coreadr adr, coremem val) {

	static char  LCDchar = 0;
	static int gpio8=-1; //data vs command flag
	//coremem readval;
	rgtype readval;
	static char LCDstr [40];
	static int i;


 if (adr==rg2addr (rg.io.gp8_11_out)) {
     rg.io.gp8_11_out = val; //need keep track of a custom mapped location

     if (rg.io.gp8_11_out == IO_GP8_11_OUT_SET8_MASK) //data
       gpio8=1;
     if (rg.io.gp8_11_out == IO_GP8_11_OUT_CLR8_MASK) //command
       gpio8=0;

     if ((rg.io.gp8_11_out & IO_GP8_11_OUT_CLR10_MASK) && (rg.io.gp8_11_out & IO_GP8_11_OUT_SET10_MASK)) { //1st (most significant) nibble

       LCDchar=0;

       //core_read_mem (rg2addr (rg.io.gp20_23_out), &readval); //the MSBit
       readval= rg.io.gp20_23_out;

       if (readval & IO_GP20_23_OUT_SET20_MASK)
	 LCDchar = LCDchar | 0x80;

       //core_read_mem (rg2addr (rg.io.gp16_19_out), &readval);
       readval= rg.io.gp16_19_out;

       if (readval & IO_GP16_19_OUT_SET19_MASK)
	 LCDchar = LCDchar | 0x40;
       if (readval & IO_GP16_19_OUT_SET18_MASK)
	 LCDchar = LCDchar | 0x20;
       if (readval & IO_GP16_19_OUT_SET17_MASK)
	 LCDchar = LCDchar | 0x10;

     } else if (rg.io.gp8_11_out == IO_GP8_11_OUT_CLR10_MASK) {//2nd (least signif) nibble

       //core_read_mem (rg2addr (rg.io.gp20_23_out), &readval); //the MSBit
       readval= rg.io.gp20_23_out;

       if (readval & IO_GP20_23_OUT_SET20_MASK)
	 LCDchar = LCDchar | 0x08;

       //core_read_mem (rg2addr (rg.io.gp16_19_out), &readval);
       readval= rg.io.gp16_19_out;

       if (readval & IO_GP16_19_OUT_SET19_MASK)
	 LCDchar = LCDchar | 0x04;
       if (readval & IO_GP16_19_OUT_SET18_MASK)
	 LCDchar = LCDchar | 0x02;
       if (readval & IO_GP16_19_OUT_SET17_MASK)
	 LCDchar = LCDchar | 0x01;

       if (gpio8==1) { //write data
	  ui_output_program_char (LCDchar);
	  LCDstr[i++]= LCDchar;

       } else if (gpio8==0) { //write command

	  switch (LCDchar) {
	    //---------seek to (1,1)
	    case 0x02:
	      //ui_output_program_char ('|'); //seek to (1,1)
	      ui_output_ws ("Updating LCD, LCD was: %s \n", LCDstr);
	      i=0;
	      break;

	    //---------seek 1 to the right?
	    case 0x14:
	      i++;
	      if (i==40) {//go to 2nd row
		i=16;
		ui_output_program_char ('\n');
		LCDstr[i++]= '|';
	      }
	      break;

	    //---------clear
	    case 0x01:
	      _ui_output_ws ("Clearing LCD, LCD was: ");
	      LCDstr[i]= '\0';
	      _ui_output_ws (LCDstr);
	      i=0;
	      if (LCDstr[0]=='\0')
		 _ui_output_ws ("BLANK");
	      _ui_output_ws("\n");
	      clearstr (LCDstr, 40);
	      break;
	  }//switch

	}

	LCDchar=0;
	//gpio8=-1;no need

     }//2nd nibble

   return SUCCESS;
 } //gp8_11_out

 else if (adr==rg2addr (rg.io.gp16_19_out)) {
   rg.io.gp16_19_out = val;
   return SUCCESS;
 }

 else if (adr==rg2addr (rg.io.gp20_23_out)) {
   rg.io.gp20_23_out=val;
   return SUCCESS;
 }

 ui_output_ws("ERROR: LCDw called with wrong argument values.\n");
 return FAILURE; //should never happen
}//LCDw




//---------------------------------DUART-------------------------------------


coremem ser_inb (coreadr adr, int selector) {
	coremem	result;

	result = uart[selector].msgBuf[uart[selector].currIndex];
	incrFIFO (uart[selector].currIndex, MAX_MSG_LEN);

	_ui_output_ws("in ser_in msgBuf is %s\n",uart[selector].msgBuf );

	return result;
}


coremem DUARTr (coreadr adr) {

	//------DUART status register
	if (adr==rg2addr(rg.duart.a_sts)) {
	   if ( uart[0].currIndex < uart[0].msgLen ) {
	     //rg.duart.a_sts |= 0x0008;
	     return 0x0009; //read and write ready
	   }
	   return 0x0001;
	   //return (rg.duart.a_sts | 0x0001); //always tx_rdy
	}

	else if (adr==rg2addr(rg.duart.b_sts)) {
	  if ( (uart[1].currIndex !=uart[1].msgLen) )  {
	     return 0x0009; //read and write ready
	  }
	  return 0x0001; //write ready
	}

	//-------DUART data read
	else if (adr==rg2addr(rg.duart.a_rx) ) {
	  return ser_inb(adr, 0);
	}
	else if (adr==rg2addr(rg.duart.b_rx) ) {
	  return ser_inb(adr, 1);
	}
	else if (adr==rg2addr(rg.duart.b_tx16) ) { //uart b broken, read from tx_16
	   _ui_output_ws ("DUARTr b data\n");
	  return ser_inb(adr, 1);
	}

	//-------DUART interrupts
	 else if (adr== rg2addr(rg.duart.a_int_en)) {
	   return rg.duart.a_int_en;
	 }
	 else if (adr== rg2addr(rg.duart.a_int_dis)) {
	   return rg.duart.a_int_dis;
	 }
	 else if (adr== rg2addr(rg.duart.b_int_en)) {
	    _ui_output_ws ("DUARTr b_int_en\n");
	   return rg.duart.b_int_en;
	 }
	 else if (adr== rg2addr(rg.duart.b_int_dis)) {
	   _ui_output_ws ("DUARTr b_int_dis\n");
	  return rg.duart.b_int_dis;
	 }

	//------
	 ui_output_ws("ERROR: DUARTr called with wrong argument values.\n");
	 return FAILURE; //should never happen
} //DUARTr



int DUARTw (coreadr adr, coremem val) {

 //-------DUART data write

 if (adr== rg2addr(rg.duart.a_tx8))  {
   ui_output_program_char ((char)val);
   return SUCCESS;
 }
 else if (adr== rg2addr(rg.duart.b_tx8)) {

   //Rx uart of duart b
   uart[1].msgBuf[uart[1].msgLen] = (char)val ;
   incrFIFO (uart[1].msgLen, MAX_MSG_LEN);
   if (rg.duart.b_int_clr == 0) {//duart B rx interrupts enabled
     _ui_output_ws ("setting irq 0x35\n");
     core_irq_set (0x35, 0); //duart b rx rdy
   }
   else
      _ui_output_ws ("duart B rx interrupts must have been disabled...\n");
   return SUCCESS;
 }

 else if (adr== rg2addr(rg.duart.a_tx16)) {
   ui_output_program_char ((char) (val >> 8));
   ui_output_program_char ((char) (val & 0x00ff));
   return SUCCESS;
 }
 else if (adr== rg2addr(rg.duart.b_tx16)) {

   //Rx uart of duart b
   uart[1].msgBuf[uart[1].msgLen] = (char)(val >> 8);
   incrFIFO (uart[1].msgLen, MAX_MSG_LEN);
   uart[1].msgBuf[uart[1].msgLen] = (char)(val & 0x00ff);
   incrFIFO (uart[1].msgLen, MAX_MSG_LEN);
   if (rg.duart.b_int_clr == 0) {//duart B rx interrupts enabled
     _ui_output_ws ("setting irq 0x35\n");
     core_irq_set(0x35, 0); //duart b rx rdy
   }
   else
      _ui_output_ws ("duart B rx interrupts must have been disabled...\n");

   return SUCCESS;
 }

 //-------DUART interrupts

#define duartinter_en(A,a)  do { \
   if ( (val & DUART_ ## A ## _INT_EN_RX_1B_RDY_MASK) || (val & DUART_ ## A ## _INT_EN_RX_2B_RDY_MASK) ) { \
     rg.duart. ## a ## _int_clr = 0x0000 ; \
     /*_ui_output_ws("enabled.\n");*/ \
   } \
   rg.duart. ## a ## _int_en = val; \
} while (0)

#define duartinter_dis(A,a)  do { \
   if ( (val & DUART_ ## A ## _INT_DIS_RX_1B_RDY_MASK) || (val & DUART_ ## A ## _INT_DIS_RX_2B_RDY_MASK) ) { \
      rg.duart. ## a ## _int_clr = val; \
      /*_ui_output_ws("disabled.\n");*/ \
   } \
   rg.duart. ## a ## _int_dis = val; \
} while (0)

 else if (adr== rg2addr(rg.duart.a_int_en)) {
   //_ui_output_ws("duart A\t");
   duartinter_en (A,a) ;    //manipulate thru local int_clr register (not custom mapped)
   return SUCCESS;
 }
 else if (adr== rg2addr(rg.duart.a_int_dis)) {
   //_ui_output_ws("duart A\t");
   duartinter_dis (A,a);
   return SUCCESS;
 }

 else if (adr== rg2addr(rg.duart.b_int_en)) {
   //_ui_output_ws("duart B\t");
   duartinter_en (B,b) ;
   return SUCCESS;
 }
 else if (adr== rg2addr(rg.duart.b_int_dis)) {
   //_ui_output_ws("duart B\t");
   duartinter_dis (B,b);
   return SUCCESS;
 }

#undef duartinter_en
#undef duartinter_dis

 ui_output_ws("ERROR: DUARTw called with wrong argument values.\n");
 return FAILURE; //should never happen
} //DUARTw




//---------------------------------CLOCK-------------------------------------

static enum timerintert {OFF, ON} timerinter =OFF ;

#define  switchcase(a)    case SSM_CFG_CLK_SEL_ ## a ## _CLK: \
	                         rg.ssm.cpu = (rg.ssm.cpu & 0xf0ff) | (SSM_CPU_STS_ ## a ## _CLK << 8); \
	                         return rg.ssm.cpu;

coremem CLOCKr (coreadr adr) {

	coremem	 result;

	//fd.ssm.cpu.sts
	if (adr==rg2addr(rg.ssm.cpu)) {
	  core_read_mem (rg2addr (rg.ssm.cfg), &result);
	  switch ((result & SSM_CFG_CLK_SEL_MASK) >> 4) {
	    switchcase (LOW_REF);
	    switchcase (HIGH_REF);
	    switchcase (LOW_PLL);
	    switchcase (HIGH_PLL);
   	    default:
	      ui_output_ws("Illegal rg.ssm.cfg & SSM_CFG_CLK_SEL_MASK\n>");
	      return NMEM;
	 }//switch
	}

	//tim.int_en1, tim.int_dis1
	else if (adr==rg2addr(rg.tim.int_en1)) {
	  return rg.tim.int_en1;
	}
	else if (adr==rg2addr(rg.tim.int_dis1)) {
	  return rg.tim.int_dis1;
	}

         ui_output_ws("ERROR: CLOCKr called with wrong argument values.\n");
	 return FAILURE; //should never happen
}//CLOCKr

#undef  switchcase


int CLOCKw (coreadr adr, coremem val) {

 //fd.ssm.cpu.sts
 if (adr==rg2addr(rg.ssm.cpu)) {
   rg.ssm.cpu=val; //cause mapped as custom, now must keep track of locally
   return SUCCESS;
 }
 //tim.int_en1, tim.int_dis1
 else if (adr==rg2addr(rg.tim.int_en1)) {
   rg.tim.int_en1=val; //cause mapped as custom, now must keep track of locally
   if (val & TIM_INT_EN1_TMR_EXP_MASK) {
     timerinter = ON;
   }
   return SUCCESS;
 }
 else if (adr==rg2addr(rg.tim.int_dis1)) {
   rg.tim.int_dis1=val; //cause mapped as custom, now must keep track of locally
   if (val & TIM_INT_DIS1_TMR_EXP_MASK) {
     timerinter = OFF;
   }
   return SUCCESS;
 }

 ui_output_ws("ERROR: CLOCKw called with wrong argument values.\n");
 return FAILURE; //should never happen
}//CLOCKw





#define K *1000
#define M *1000*1000

const lword HR=5 M, LR= 32768, HP=100 M, LP=4915200, PS=98 M;

lword cpuclockfreq_inHz () {

  lword cpuclockfreq;
  coremem val;

 //assume clocks are enabled.....
 //assume that clock refs and PLLs enabled
  switch ((rg.ssm.cpu & SSM_CPU_STS_MASK) >> 8) {
    case SSM_CPU_STS_HIGH_REF_CLK:
      cpuclockfreq = HR;
      break;
    case SSM_CPU_STS_LOW_REF_CLK:
      cpuclockfreq = LR;
      break;
    case SSM_CPU_STS_HIGH_PLL_CLK:
      cpuclockfreq = HP;
      break;
    case SSM_CPU_STS_LOW_PLL_CLK:
      core_read_mem (rg2addr (rg.ssm.cfg), &val);
      if (val & SSM_CFG_PLL_STEPUP_MASK)  {
	cpuclockfreq = PS;
      } else {
	cpuclockfreq = LP;
      }
      break;
  }//switch

  _ui_output_ws("in cycles: cpuclockfreq is %d \n ",  cpuclockfreq  );
  cpuclockfreq /= ( 16- 2* (rg.ssm.cpu & SSM_CPU_PRESCALAR_MASK) );
  cpuclockfreq /= ( 8- ((rg.ssm.cpu & SSM_CPU_CPU_CLK_DIV_MASK) >>3) );
  _ui_output_ws("in cycles: cpuclockfreq is %d  \n ",  cpuclockfreq );

  return cpuclockfreq;
}



lword timerfreq_inHz (word ssm_div_sel_pll_mask, coreadr ssm_tap_sel_rg_addr, word ssm_tap_sel_mask, coreadr tim_ld_rg_addr) {

  lword timerfreq;
  coremem val;

  //assume tim_clk enabled in ssm.clk_en

  core_read_mem (rg2addr (rg.ssm.div_sel), &val);
  if (val & SSM_DIV_SEL_LOW_CLK_TIMER_MASK) {//low_clk
    if (val & ssm_div_sel_pll_mask) {//pll
      timerfreq = LP;
    } else { //ref
      timerfreq = LR;
    }
  } else { //high_clk
    if (val & ssm_div_sel_pll_mask) {//pll
      timerfreq = HP;
    } else { //ref
      timerfreq = HR;
    }
  }

  _ui_output_ws("in cycles: timerfreq is %d \n ",timerfreq  );
  core_read_mem (ssm_tap_sel_rg_addr, &val);
  //timerfreq >>= ( ((val & SSM_TAP_SEL3_TMR_MASK) >> 8) +3 ) ;
  timerfreq >>= ( ((val & ssm_tap_sel_mask) >> 8) +2 ) ; //ripple counters/divide chains

  core_read_mem (tim_ld_rg_addr, &val);
  timerfreq /= (val +1); //in how many tmr clock periods timer interrupt is fired
  _ui_output_ws("in cycles: timerfreq is %d \n ",timerfreq  );

  return timerfreq;
}


//determines in how many cpu cycles the timer interrupt should be fired
lword timerfreq_incpucycles () {

  lword cpuclockfreq, timerfreq;

  //-------calculate cpuclockfreq
  cpuclockfreq= cpuclockfreq_inHz ();

  //--------now calculate timer (rtc) freq
  timerfreq= timerfreq_inHz (SSM_DIV_SEL_TMR_MASK, rg2addr(rg.ssm.tap_sel3), SSM_TAP_SEL3_TMR_MASK, rg2addr(rg.tim.tmr_ld));

  _ui_output_ws("\n cpuclockfreq is %d and timerfreq is %d and ratio is %d \n ",  cpuclockfreq, timerfreq , cpuclockfreq/timerfreq );

  return cpuclockfreq/timerfreq;

}

#undef K
#undef M


//---------------------------------ETHER-------------------------------------

static int RXlag=0;
static coremem custombanks [4][8] ; //custom mapped overlapping bank locations
static word mcastaddr [3] = ETH_DEST; //same as in ethernet.c //here swabed when reading ourMACaddress


#define MACequal(a,b)  ( (a[0]==b[0]) && (a[1]==b[1]) && (a[2]==b[2]) )
#define Tx(i,f) do { destination(i).RXframesFIFO[destination(i).i1] =f;\
                     incrFIFO (destination(i).i1, MAXFIFO) ;\
                   } while(0)

//copy local frame to appropriate location in shared memory
void tx2shmem (framet f) {

  int i;

  if ( MACequal (f.destMAC, mcastaddr) ) {//multicast
    for (i=0; i< ndestinations; i++) {
      if ( (destination(i).RCR & RCR_ALMUL) && (i!= ourMACindex) ) //do not Tx to ourselves ?
	Tx (i,f);
    }

  } else { //not multicast, Tx to the destination
    i = f.destMAC[0];

    _ui_output_ws("tx2shmem destindex %d\n", i);
    _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[0] );
    _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[1] );
    _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[2] );

    if (i < ndestinations) {
      //now copy over
      Tx (i,f);
    }

    //now "multicast" to permiscious nodes
    for (i=0; i< ndestinations; i++) {
      _ui_output_ws("tx2shmem: checking if node %d is permiscious. its RCR is (H'%04X) \n", i,destination(i).RCR );
      if ((destination(i).RCR & RCR_PRMS) && (i!= ourMACindex) && (i!=f.destMAC[0]) ) {
	//do not Rx own frames unless in Full Duplex (p.43)
	_ui_output_ws("tx2shmem: TXing to permiscious node %d\n", i);
	Tx (i,f);
      }
    }
  }//not multicast

}//tx2shmem

#undef Tx
#undef MACequal


//called from check_isr
void rx1fromshmem () {

  incrFIFO (destination(ourMACindex).i2, MAXFIFO) ;
  RXlag++;
  _ui_output_ws("increased RXlag in rx1fromshmem to %d\n", RXlag);
}


//called from ETHERr
//(reminder: returning destt would overflow the stack)
void rx2fromshmem (framet * f) {

  int i= ourMACindex;

  //now copy over
  *f= destination(i).RXframesFIFO[destination(i).i2-RXlag] ;
  f->i1=0; //frame index
  RXlag--;
  _ui_output_ws("decreased RXlag in rx2fromshmem to %d\n", RXlag);
}



coremem ETHERr (coreadr adr) {

  static framet f;
  coremem val;
  coremem bank;
  word w;

 	core_read_mem (ioaddr+BANK_SELECT, &bank);

        //-------MAC address, 3 words
	if ( (bank==1) && (adr== ioaddr+ADDR0_REG || adr== ioaddr+ADDR1_REG || adr== ioaddr+ADDR2_REG) )  {

	  //the driver will swab it, let's swab it twice to keep consistent with ourMACaddr
	  w= ourMACaddr[adr-ioaddr-ADDR0_REG];
	  swab (w);

	  //also let's swab mcastaddr for a company
	  swab (mcastaddr[adr-ioaddr-ADDR0_REG]);

	  return w;
	}

	//---------Rx status, for RXing
	else if ( (bank==2) && (adr== ioaddr+ RXFIFO_REG) )  {

	  if (RXlag >0) {
	    rx2fromshmem (&f) ; //may need to go down to read from DATA_REG for i1=0;
	    _ui_output_ws("Rx status not empty!! \n");
	    return 0x0000; // ! RXFIFO_EMPTY
	  }
	  else {
	    _ui_output_ws("Rx status empty \n");
	    return RXFIFO_EMPTY; //0x8000
	  }
	}

	//--------DATA_REG
	else if ( (bank==2) && (adr== ioaddr+ DATA_REG) )  {
	   val= (coremem) f.frame[f.i1] ; //from local frame f

	   _ui_output_ws("in read f.i1 %d", f.i1 );
	   _ui_output_ws(" and val (H'%04X)\n", val );

	   f.i1++;
	   return val;
	}

	//---------MMU status, needed in waitmmu(w), for TXing
	//the only read bit in MMU_CMD_REG, the rest are write only
	else if ( (bank==2) && (adr== ioaddr+ MMU_CMD_REG) ) {
	  _ui_output_ws("reading MMU status from MMU_CMD_REG\n");
	  return 0x0000; //not busy
	}

	else {
	  return custombanks [bank][adr-ioaddr]; //custom mapped overlapping bank locations
	}
}//ETHERr


int ETHERw (coreadr adr, coremem val) {

  static framet f;
  coremem bank;

  core_read_mem (ioaddr+BANK_SELECT, &bank);

  //write to DATA_REG
  if ( (bank==2) && (adr== ioaddr+ DATA_REG) )  {

   f.frame[f.i1] =(word) val; //fill in local frame f

   _ui_output_ws("f.i1 %d", f.i1 );
   _ui_output_ws(" and val (H'%04X)\n", val );


   switch (f.i1) {
   case 0: //status
     break;
   case 1: //val==number of bytes
     f.i2=val/2; //number of words...... assume always even
     break;
   case 2://val== dest MAC
     f.destMAC[0]=(word) val;
     _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[0] );
     break;
   case 3:
     f.destMAC[1]=(word) val;
     _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[1] );
     break;
   case 4:
     f.destMAC[2]=(word) val;
     _ui_output_ws("f.destMAC (H'%04X)\n", f.destMAC[2] );
     break;
   }

   if ( (f.i1==f.i2-1) && (f.i2-1!=0) ) {//end of frame
     f.i1=-1;
     tx2shmem (f) ;
   }

   f.i1++;
   return SUCCESS;
  } //write to DATA_REG

 else {
   custombanks [bank][adr-ioaddr] = val; //custom mapped overlapping bank locations

   //write to RCR_REG (Rx Ctrl Reg: multicast, permisc)
   //must keep a copy in shmem for each node
   if ( (bank==0) && (adr== ioaddr+ RCR_REG) )  {
     if (ourMACindex != NOTFOUND) {
       _ui_output_ws("setting RCR to (H'%04X)\n", (word) val);
       destination(ourMACindex).RCR= (word) val;
     }
  }
   return SUCCESS;
 }

}//ETHERw


//---------------------------------RADIO-------------------------------------

static enum radio_enable_type {RCV, XMT} radio_enable=RCV;
static enum remove_one_type {rOFF,rON} remove_one=rOFF;

//set Rx status
#define setrxs(r,g) do { \
            if (RADIO_TYPE== RADIO_RFMI) { \
	      rg.io.gp8_15_sts = r ; \
	    } else if (RADIO_TYPE== RADIO_VALERT || RADIO_TYPE== RADIO_XEMICS) { \
	      rg.io.gp8_15_sts = g ; \
	    } \
} while(0)


coremem radioRx () {

          if ( (! isFIFOempty(destination(ourMACindex),r1,r2)) && (radio_enable==RCV) ) {

	    if ( (destination(ourMACindex).radio[destination(ourMACindex).r2] ) == 1) {
	      _ui_output_ws("RADIOr Rxing a HIGH and r2 is %d\n",destination(ourMACindex).r2);
	      setrxs (0, IO_GP8_15_STS_STS14_MASK) ; //Rx a HIGH
	    } else {
	      _ui_output_ws("RADIOr Rxing a LOW and r2 is %d\n",destination(ourMACindex).r2);
	      setrxs (IO_GP8_15_STS_STS14_MASK, 0); //Rx a LOW
	    }

	    rg.io.gp8_15_sts |= IO_GP8_15_STS_STS15_MASK; //signal that FIFO not empty (for XEMICS dataclk)
	    _ui_output_ws("returning rg.io.gp8_15_sts as %x for RADIO_TYPE %d \n", rg.io.gp8_15_sts,RADIO_TYPE );


#if   RADIO_TYPE== RADIO_XEMICS

	    if (remove_one== rON) {//hack: force incrFIFO, assume set14 never used in PicOS
	      remove_one= rOFF; //undo
	      incrFIFO (destination(ourMACindex).r2, MAXRADIO);
	      _ui_output_ws("RADIOr incremented r2 to %d\n",destination(ourMACindex).r2);
	    }
#endif


	  } else { //FIFO empty
	    //rcvhigh used in timer interrupt to set rxlast_sense, which is used in txmradio process (phys_radio.c) to determine backoff
	    setrxs (IO_GP8_15_STS_STS14_MASK, 0); //return LOW
	    rg.io.gp8_15_sts &= (~ IO_GP8_15_STS_STS15_MASK); //signal that FIFO empty (for XEMICS dataclk)
	    _ui_output_ws("FIFO empty: returning rg.io.gp8_15_sts as %x for RADIO_TYPE %d \n", rg.io.gp8_15_sts,RADIO_TYPE );
	  }

	  return rg.io.gp8_15_sts;
}

//RFMI or VALERT
coremem RADIOr1 (coreadr adr) {

        //Rxing
	if ( adr==rg2addr(rg.io.gp8_15_sts) ) {
	  return radioRx ();

	//Txing
	} else if ( adr==rg2addr(rg.io.gp12_15_out) ) {
	  return rg.io.gp12_15_out; //simply return
	}

	ui_output_ws("ERROR: RADIOr called with wrong argument values.\n");
	return FAILURE; //should never happen
}

//XEMICS
coremem RADIOr2 (coreadr adr) {

        //Rxing
	if ( adr==rg2addr(rg.io.gp8_15_sts) ) {
	  return radioRx ();

	//Txing
	} else if ( adr==rg2addr(rg.io.gp0_3_out) ) {
	  return rg.io.gp0_3_out; //simply return
	}

	ui_output_ws("ERROR: RADIOr called with wrong argument values.\n");
	return FAILURE; //should never happen
}



#define radioTx(i,b) do { destination(i).radio[destination(i).r1] =b ; \
                          _ui_output_ws(" at r1 %d \n", destination(i).r1); \
                          incrFIFO (destination(i).r1, MAXRADIO); \
                        } while (0)

#define clear13 do { rg.io.gp12_15_out &= 0xffcf; /*clear set13 and clr13*/ \
                        } while (0)

#define clear0 do { rg.io.gp0_3_out &= 0xfffc; /*clear set0 and clr0*/ \
                        } while (0)

#define d(i,x) (destination(i). ## x ## - destination(ourMACindex). ## x ##)
#define within_range(i,range) ( d(i,x)*d(i,x) + d(i,y)*d(i,y)  <= range*range)

#define radioTxall(b) do { \
      for (i=0; i< ndestinations; i++) { \
	  if ( within_range(i,range) && (i!=ourMACindex) ) { \
		_ui_output_ws("RADIOw: Txing a %d to node %d within our range %d ", b, i, range); \
	        radioTx (i, b); \
	   } \
      } \
} while(0)

#define callradioTxall(r,c,b) do { \
if (radio_enable==XMT) { \
	if (val & IO_GP  ##  r  ##  _OUT_SET  ##  c  ##  _MASK) { \
	  b=1; \
	} else if (val & IO_GP  ##  r  ##  _OUT_CLR  ##  c  ##  _MASK) {  /* Tx a 0, can not do both */ \
       	  b=0; \
	} \
	clear  ##  c; \
	radioTxall(b); \
} \
} while(0)

//RFMI or VALERT
int RADIOw1 (coreadr adr, coremem val) {

        int i;
        bool b;

        if ( adr==rg2addr(rg.io.gp12_15_out) ) {

	  rg.io.gp12_15_out = val;

	  //for Rxing
	  if (val&IO_GP12_15_OUT_SET14_MASK)  {//my hack for set14
	    if ( !isFIFOempty(destination(ourMACindex),r1,r2) ) {

	      incrFIFO (destination(ourMACindex).r2, MAXRADIO);
	      _ui_output_ws("RADIOr incremented r2 to %d\n",destination(ourMACindex).r2);
	      rg.io.gp12_15_out &= ~IO_GP12_15_OUT_SET14_MASK; //undo
	    }
	  }
	  //for Txing/Rxing
	  if ( (val & IO_GP12_15_OUT_CLR12_MASK) ) {//xmt enable
	    radio_enable = XMT;
	  }
	  if ( (val & IO_GP12_15_OUT_SET12_MASK) ) {//rcv enable
	    radio_enable = RCV;
	  }
	  //Txing itself
	  if ((val & IO_GP12_15_OUT_SET13_MASK) || (val & IO_GP12_15_OUT_CLR13_MASK) ) {
	    callradioTxall (12_15, 13, b);
	  }
	  return SUCCESS;

	//Rxing
  	} else if ( adr==rg2addr(rg.io.gp8_15_sts) ) {
	  ui_output_ws ("ERROR: rg.io.gp8_15_sts is a Read-Only register.\n");
	  return FAILURE;
	}

	//else must be an error
	ui_output_ws("ERROR: RADIOw called with wrong argument values.\n");
	return FAILURE; //should never happen
}



//XEMICS
int RADIOw2 (coreadr adr, coremem val) {

        int i;
	bool b;

	 if ( adr==rg2addr(rg.io.gp12_15_out) ) {
	  rg.io.gp12_15_out = val;

	  //for Rxing
	  if (val&IO_GP12_15_OUT_SET14_MASK)  {//my hack for set14
	    if ( !isFIFOempty(destination(ourMACindex),r1,r2) ) {
	    remove_one= rON;
	    rg.io.gp12_15_out &= ~IO_GP12_15_OUT_SET14_MASK; //undo
	    _ui_output_ws("RADIOw: set remove_one rON\n\n ");
	    } else
	      _ui_output_ws("RADIOw: FIFO empty\n");
	  }
	  return SUCCESS;

	} else if ( adr==rg2addr(rg.io.gp0_3_out) ) {
	    rg.io.gp0_3_out = val;

	    //Txing itself
	    if ((val & IO_GP0_3_OUT_SET0_MASK) || (val & IO_GP0_3_OUT_CLR0_MASK) ) {
	      _ui_output_ws("radio_enable is %d\n",radio_enable);
	      callradioTxall (0_3, 0, b);
	    }
	    return SUCCESS;

    	//Rxing
  	} else if ( adr==rg2addr(rg.io.gp8_15_sts) ) {
	  ui_output_ws ("ERROR: rg.io.gp8_15_sts is a Read-Only register.\n");
	  return FAILURE;
	}

	//else must be an error
	ui_output_ws("ERROR: RADIOw2 called with wrong argument values.\n");
	return FAILURE; //should never happen
}


#undef radioTx
#undef clearTx
#undef d
#undef within_range

/******************************************************************************
NAME
	sim_d_read

SYNOPSIS
	EXPORT coremem sim_d_read (coreadr adr, bool byte_access,
		bool proc_access,
		lword * cycles)

FUNCTION
Called whenever the processor or user attempts to read from data memory
mapped as CUSTOM. If the address is not valid the function should return
the macro NMEM. When the access is from the processor a return value of
NMEM will cause the READ_WRITEONLY_IO exception. Normally the function will
return the value that would be read from the address. When the access is
from the processor the function is responsible for generating the number of
processor cycles that were used to perform the access and returning the
value via the pointer cycles.
This function is required.

******************************************************************************/

EXPORT coremem sim_d_read (coreadr adr, bool byte_access, bool proc_access,
				lword * cycles) {

        static const char		myName[] =	"sim_d_read";

	/*
	// illegal stuff -- trap
	if (byte_access) {
		ui_output_ws("%s: Illegal byteAccess(%d), addr(H'%04X)\n>",
					myName, byte_access, adr);
		return NMEM;
	}
	*/
	*cycles = 3;
	/*
	if (cycles != NULL)
          *cycles = 3; //whatever
	*/

	//------LCD
	if (adr==rg2addr (rg.io.gp8_11_out) || adr==rg2addr (rg.io.gp16_19_out) || adr==rg2addr (rg.io.gp20_23_out)) {
	  return LCDr (adr);
	}

	//------DUART
	else if ( adr==rg2addr(rg.duart.a_sts) || adr==rg2addr(rg.duart.b_sts) || \
	     adr==rg2addr(rg.duart.a_rx) || adr==rg2addr(rg.duart.b_rx) || adr==rg2addr(rg.duart.b_tx16) ||
	     adr==rg2addr(rg.duart.a_int_en) || adr==rg2addr(rg.duart.a_int_dis) || adr==rg2addr(rg.duart.b_int_en) || adr==rg2addr(rg.duart.b_int_dis)	) {
	 return DUARTr (adr);
	}

	//------CLOCK
	else if ( adr==rg2addr(rg.ssm.cpu) || adr==rg2addr(rg.tim.int_en1) || adr==rg2addr(rg.tim.int_dis1) ) {
	  return CLOCKr (adr);
	}
	else if ( adr==rg2addr(rg.tim.ctrl_en) ) {
	  return rg.tim.ctrl_en;
	}
	else if ( adr==rg2addr(rg.tim.ctrl_dis) ) {
	  return rg.tim.ctrl_dis;
	}

	//-------ETHER
	else if ( (adr== ioaddr+ADDR0_REG) || (adr== ioaddr+ADDR1_REG) || (adr== ioaddr+ADDR2_REG) || \
	     (adr== ioaddr+ RXFIFO_REG) || (adr== ioaddr+ DATA_REG) || (adr== ioaddr+ MMU_CMD_REG) ) {
	  return ETHERr (adr);
	}

	//-------BEEPER
	else if ( adr==rg2addr(rg.ssm.tap_sel2) || adr==rg2addr(rg.ssm.rst_set) ) {
	  return BEEPERr (adr);
	}

#if RADIO_TYPE==RADIO_RFMI || RADIO_TYPE==RADIO_VALERT

	//-------RADIO
	else if ( adr==rg2addr(rg.io.gp8_15_sts) || adr==rg2addr(rg.io.gp12_15_out) ) {
	  return RADIOr1 (adr);
	}

#else if RADIO_TYPE== RADIO_XEMICS

	//-------RADIO
	else if ( adr==rg2addr(rg.io.gp8_15_sts) || adr==rg2addr(rg.io.gp0_3_out) ) {
	  return RADIOr2 (adr);
	}
	else if ( adr==rg2addr(rg.io.gp12_15_out) ) {
	  return rg.io.gp12_15_out;
	}

#endif

	else if ( adr==rg2addr(rg.io.gp20_23_cfg) ) {
	  return rg.io.gp20_23_cfg;
	}

	//------------------nothing else, must be an error
	ui_output_ws("%s: Illegal addr(H'%04X)\n", myName, adr);
	return NMEM;
}




/******************************************************************************
NAME
	sim_d_write

SYNOPSIS
   EXPORT int sim_d_write (coreadr adr, coremem val, bool byte_access,
				bool proc_access, lword * cycles)

FUNCTION
Called whenever the processor or user writes to data memory mapped as
CUSTOM. The function should check the address, and if it is valid it should
act on the data. This function should return SUCCESS or FAILURE to indicate
to the caller whether the operation was successful. An error message is
printed if this function returns FAILURE. When the access is from the
processor the function is responsible for generating the number of
processor cycles that were used to perform the access and returning the
value via the pointer cycles.

This function is required.

COSA NOSTRA
This thing is invoked to open, then for each byte, and finally to close
a single write buffer "transaction". Worse: it loops to write to all
available neighbours.

Byte access twists are cut off.

RETURNS
	SUCCESS or FAILURE.
******************************************************************************/


EXPORT int sim_d_write(coreadr adr, coremem val, bool byte_access,
		       bool proc_access, lword * cycles) {

	static const char 		myName[] =	"sim_d_write" ;

	/*
	if (byte_access) {
		ui_output_ws("%s: Illegal byteAccess(%d), addr(H'%04X)",
			myName, byte_access, adr);
		return FAILURE;
	}
	*/
	*cycles =7;
	/*
	if (cycles != NULL) {
	  *cycles = 7; //whatever
	}
	*/

	//-----------LCD
	if (adr==rg2addr (rg.io.gp8_11_out) || adr==rg2addr (rg.io.gp16_19_out) || adr==rg2addr (rg.io.gp20_23_out)) {
	  return LCDw (adr, val);
	}

	//-----------DUART
	else if (adr==rg2addr (rg.duart.a_tx8) || adr==rg2addr (rg.duart.b_tx8) || \
	  adr==rg2addr (rg.duart.a_tx16) || adr==rg2addr (rg.duart.b_tx16) || \
	  adr==rg2addr(rg.duart.a_int_en) || adr==rg2addr(rg.duart.a_int_dis) || adr==rg2addr(rg.duart.b_int_en) || adr==rg2addr(rg.duart.b_int_dis) ) {
	  return DUARTw (adr, val);
	}

	//------------CLOCK
	else if (adr==rg2addr(rg.ssm.cpu) || adr==rg2addr(rg.tim.int_en1) || adr==rg2addr(rg.tim.int_dis1)) {
	  return CLOCKw (adr, val);
	}
	else if ( adr==rg2addr(rg.tim.ctrl_en) ) {
	  rg.tim.ctrl_en= val;
#if RADIO_TYPE== RADIO_XEMICS
	  if (val & TIM_CTRL_EN_CNT1_CNT_MASK) {
	    radio_enable=XMT;
	    _ui_output_ws ("radio_enable=XMT\n");
	  }
#endif
	  return SUCCESS;
	}
	else if ( adr==rg2addr(rg.tim.ctrl_dis) ) {
	  rg.tim.ctrl_dis= val;
#if RADIO_TYPE== RADIO_XEMICS
	  if (val & TIM_CTRL_DIS_CNT1_CNT_MASK) {
	    radio_enable=RCV;
	    _ui_output_ws ("radio_enable=RCV\n");
	  }
#endif
	  return SUCCESS;
	}


	//------------ETHER
	else if ( (((adr-ioaddr)>=2) && ((adr-ioaddr)<=4)) || (adr-ioaddr==0) ) {
	  return ETHERw (adr, val);
	}

	//------------BEEPER
	else if ( adr==rg2addr(rg.ssm.tap_sel2) || adr==rg2addr(rg.ssm.rst_set) ) {
	  return BEEPERw (adr, val);
	}

#if	RADIO_TYPE != RADIO_XEMICS
	//-----------LEDs
	else if ( adr==rg2addr (rg.io.gp0_3_out)) {
	    return LEDw (adr, val);
	}
#endif

#if RADIO_TYPE==RADIO_RFMI || RADIO_TYPE==RADIO_VALERT
	//------------RADIO
	else if ( adr==rg2addr(rg.io.gp8_15_sts) || adr==rg2addr(rg.io.gp12_15_out) ) {
	  return RADIOw1 (adr, val);
	}

#else if RADIO_TYPE== RADIO_XEMICS
	//-----------RADIO
	else if ( adr==rg2addr(rg.io.gp8_15_sts) || adr==rg2addr(rg.io.gp0_3_out) || adr==rg2addr(rg.io.gp12_15_out) ){
	   return RADIOw2 (adr, val);
	}
#endif

	else if ( adr==rg2addr(rg.io.gp20_23_cfg) ) {
	   rg.io.gp20_23_cfg = val;
	   return SUCCESS;
	}

	//--------------nothing else: must be an error
	ui_output_ws("%s: Illegal addr(H'%04X)\n", myName, adr);
	return FAILURE;
}

#undef inSDRAM


/******************************************************************************
NAME
   sim_p_read

SYNOPSIS
   EXPORT coremem sim_p_read( coreadr adr, bool	proc_access, lword * cycles )

FUNCTION
   Called whenever the processor or user attempts to read from program memory
   mapped as CUSTOM. If	the address is not valid the function should return
   the macro NMEM. When	the access is from the processor a return value	of
   NMEM will cause the INSTRUCTION_NONE exception. Normally the function will
   return the value that would be read from the	address. When the access is
   from the processor the function is responsible for generating the number
   of processor cycles that were used to perform the access and returning
   the value via the pointer cycles.

   This function is required.

RETURNS

******************************************************************************/

EXPORT coremem sim_p_read( coreadr adr, bool proc_access, lword * cycles )
{
   /* There is no program memory in this model.	*/
   return NMEM ;
}



/******************************************************************************
NAME
   sim_p_write

SYNOPSIS
   EXPORT int sim_p_write( coreadr adr, coremem	val, bool proc_access,
			   lword * cycles )

FUNCTION
   Called whenever the processor or user writes	to program memory mapped as
   CUSTOM. The function	should check the address, and if it is valid it	should
   act on the data. This function should return	SUCCESS or FAILURE to indicate
   to the caller whether the operation was successful. An error message is
   printed if this function returns FAILURE. When the access is from the
   processor the function is responsible for generating the number of
   processor cycles that were used to perform the access and returning the
   value via the pointer cycles.

   This function is required.

RETURNS
   SUCCESS or FAILURE.
******************************************************************************/

EXPORT int sim_p_write(	coreadr adr, coremem val, bool proc_access,
			lword * cycles )
{
   /* There is no program memory in this model.	*/
   return FAILURE ;
}



/******************************************************************************
NAME
   sim_sif

SYNOPSIS
   EXPORT int sim_sif( void )

FUNCTION
   Called whenever the eCOG could respond to a SIF request - when the
   processor is asleep or a SIF instruction occurs. sim_sif should decide
   whether a SIF cycle is pending, and if so call core_sif to actually perform
   the SIF cycle.

   This function is required.

RETURNS
   SUCCESS or FAILURE.
******************************************************************************/

EXPORT int sim_sif( void )
{
   /*  functionality template for io_sif()  */

   /*
    *  int	result;
    *  lword	addr, data_in, data_out;
    *
    *  if (sif cycle has been requested)
    *  {
    *	  result = core_sif (addr, data_in, &data_out);
    *	  return result;
    *  }
    *  else return SUCCESS;
    */

   return SUCCESS ;
}




/******************************************************************************
NAME
   sim_clock

SYNOPSIS
   EXPORT int sim_clock( lword cycles )

FUNCTION
   Called after every instruction, and every cycle when the eCOG is asleep.
   Can be used to model	complicated hardware, which may cause the eCOG to
   wake-up or reset.

   This function is required.

RETURNS
   SUCCESS or FAILURE.
******************************************************************************/

EXPORT int sim_clock (lword cycles) {

  static lword timerfreq=1, cnt1freq=1, cpuclockfreq;
  static lword ttimes=0, ctimes=0;

  coremem val;

  proc_cycles = cycles;

  //----------------for CLOCK---------------

  //check that timer interrupts enabled
   if (timerinter == ON) {

     //initialise
     if (proc_cycles < timerfreq) { //reset
       _ui_output_ws ("resetting: proc_cycles %d, timerfreq %d, proc_cycles / timerfreq %d \n", proc_cycles, timerfreq, proc_cycles / timerfreq);

       ttimes =0;
       timerinter = OFF;
       return SUCCESS ;
     }

     if (ttimes ==0 ) {
       cpuclockfreq= cpuclockfreq_inHz ();
       timerfreq= cpuclockfreq / timerfreq_inHz (SSM_DIV_SEL_TMR_MASK, rg2addr(rg.ssm.tap_sel3), SSM_TAP_SEL3_TMR_MASK, rg2addr(rg.tim.tmr_ld));
       ttimes=proc_cycles / timerfreq -1;

       core_write_mem (rg2addr(rg.tim.int_sts1), TIM_INT_STS1_CNT1_EXP_MASK); //CNT1 expires right away, no baud modelling yet
       core_read_mem (rg2addr(rg.tim.int_sts1), &val);
       _ui_output_ws ("int_sts1 is %x\n", val);

     }

     //call timer (rtc) irs
     if (proc_cycles / timerfreq > ttimes) {
       ttimes++;
       //ui_output_ws ("ttimes %d, proc_cycles %d, timerfreq %d, proc_cycles / timerfreq %d \n", ttimes, proc_cycles, timerfreq, proc_cycles / timerfreq);

                //  if ( (proc_cycles - previous) >= timerfreq ) {
                //        previous = proc_cycles;
		//        * vector = 0x0E ; // better don't move the interrupt...
       core_irq_set(0xE, 0);
       if (core_proc_state() == SLEEPING) {
	 core_wake_up();
       }
     }
   } //timerinter==ON

   //----------------for RADIO== XEMICS Tx baud ---------------
   //NOTE: taken out as this makes it veery slow

   /*
   if (radio_enable==XMT) {

     //initialise
     if (proc_cycles < cnt1freq)
       ctimes =0;

     //check if CNT1 expires
     if (proc_cycles / cnt1freq > ctimes) {
       ctimes++;
       cnt1freq= cpuclockfreq / timerfreq_inHz (SSM_DIV_SEL_CNT1_MASK, rg2addr(rg.ssm.tap_sel2), SSM_TAP_SEL2_CNT1_MASK, rg2addr(rg.tim.cnt1_ld)); //update each time as the user may set a new Tx baud rate

       core_read_mem (rg2addr(rg.tim.int_sts1), &val);
       core_write_mem (rg2addr(rg.tim.int_sts1), val | TIM_INT_STS1_CNT1_EXP_MASK); //CNT1 expires
       //core_read_mem (rg2addr(rg.tim.int_sts1), &val);
       //_ui_output_ws ("int_sts1 is %x and cnt1freq is %d\n", val, cnt1freq);

     }
   } //radio_enable==XMT
   */

   return SUCCESS ;
}



/******************************************************************************
NAME
   sim_check_irq

SYNOPSIS
   EXPORT int sim_check_irq( coreadr * vector )

FUNCTION
   Called by the Simulator to check if any interrupt requests are pending. The
   function returns SUCCESS and a vector address via vector if an interrupt is
   ready. The brach address is then the 16 bit value read from code memory at
   the vector address, which is sign extended to 24 bits. Valid vector
   addresses are H'4 to	H'3F. FAILURE indicates	that there is no pending
   interrupt.

   This function is required.

RETURNS
   SUCCESS or FAILURE.
******************************************************************************/

//the sequence of if's implies interrupt priority

#define core_or_mem(rg,val,MASK) do { \
  core_read_mem (rg2addr(rg), &val); \
  val |= MASK; \
  core_write_mem (rg2addr(rg), val); \
} while (0)

#define yes2tcv_interrupt    core_or_mem(rg.io.gp0_7_sts, val, IO_GP0_7_STS_STS7_MASK)
//somewhere may need to say to no2tcv_interrupt

#define yes2int_radio        core_or_mem(rg.io.gp16_23_sts, val, IO_GP16_23_STS_INT21_MASK)
//somewhere may need to say to no2int_radio

EXPORT int sim_check_irq( coreadr * vector )
{

  coremem val;

  //---------gpio interrupt: ETHER and RADIO XEMICS


  //--------------check for ETHER interrupts
  //check that ETHER interrupts enabled
  core_read_mem (ioaddr+INT_REG, &val); //could custom map this location and do the rest from there when it's set

  if (val & (FLG_ENRCV<<8)) { //IM_RCV_INT=0x01 and FLG_ENRCV=0x0001

    if ( ! isFIFOempty (destination(ourMACindex),i1,i2) ) {

        rx1fromshmem ();
	yes2tcv_interrupt;
      	core_irq_set(0x37, 0); //gpio_interrupt, not ether only, see new kernel.asm

      // *vector =0x37 ;
      //return SUCCESS;
    }
  }


  //--------------check for RADIO XEMICS interrupts
  //check that XEMICS interrupts enabled

  if (rg.io.gp20_23_cfg & IO_GP20_23_CFG_INT21_MASK ) { //int on


    if ( ! isFIFOempty (destination(ourMACindex),r1,rlag) ) {

      destination(ourMACindex).rlag=destination(ourMACindex).r1; //update rlag

      _ui_output_ws ("rg.io.gp20_23_cfg is %x\n",rg.io.gp20_23_cfg );

      yes2int_radio;
      core_read_mem (rg2addr(rg.io.gp16_23_sts), &val);
      _ui_output_ws ("rg.io.gp16_23_sts is %x\n", val);

      core_irq_set(0x37, 0); //gpio_interrupt, not ether only, see new kernel.asm

    }

  }

  return FAILURE ; //we pass thru core_irq_set
}





