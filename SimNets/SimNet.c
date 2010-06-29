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
/* FILE SimNet.c
/*   file with custom commands.                                                 */
/* =============================================================================*/

// for shmem
#include <windows.h>
#include <memory.h>

// mandatory order? <stdio.h>, then "global.h, then "cli.h" 
#include <stdio.h>
#include "global.h"
#include "cli.h"

#include "core.h"
#include "ui.h"


// Custom Command API
EXPORT const char *			cmd_get_name (void);
EXPORT const char *			cmd_get_version (void);
EXPORT int				cmd_get_num_cmds (void);
EXPORT const CLI_COMMAND *	        cmd_get_cmd (int num);

#include "SimNet.h"

/* CUSTOM COMMANDS */

static int custom_ui(int argc, char *argv[]);
static int custom_node (int argc, char *argv[]);

shmemType * shmemPtr = NULL; // pointer to shared memory
uartt uart[NUARTS]; //local duart(s)

static const char version[]   =	"2003";
static const char name[]      =	"SimNet";

static const CLI_COMMAND cmds[]	=
{
	{
	"ui", 2, 3,
	(CLI_SIM | CLI_SYS), CLI_NONE, custom_ui,
	"ui \"s\" ",
	"User-input a string"
	},

	{
	"node", 1, 5,
	(CLI_SIM | CLI_SYS), CLI_NONE, custom_node,
	"node i x y r",
	"Set/get parameters for node i: coordinate x, coordinate y, and range r"
	},

};

static const int num_cmds = sizeof(cmds) / sizeof(CLI_COMMAND) ;


/* Custom Command API (some untouched, as per eCog practice) */

/******************************************************************************
NAME
	cmd_get_name

SYNOPSIS
	EXPORT const char * cmd_get_name( void )

FUNCTION
	Returns the name of the Custom Module.
	This function is required.

RETURNS
   Name of Custom Moudle.
******************************************************************************/

EXPORT const char * cmd_get_name( void )
{
	return name ;
}


/******************************************************************************
NAME
	cmd_get_version

SYNOPSIS
	EXPORT const char * cmd_get_version( void )

FUNCTION
	Returns the version of the Custom Module.
	This function is required.

RETURNS
	Version of Custom Moudle.
******************************************************************************/

EXPORT const char * cmd_get_version( void )
{
	return version ;
}


/******************************************************************************
NAME
	cmd_get_num_cmds

SYNOPSIS
	EXPORT int cmd_get_num_cmds( void )

FUNCTION
	Returns the number of commands in the Custom Module. This number is the
	sizeof the CLI_COMMAND array	and therefore includes any menu separators.
	This function is required.

RETURNS
	Number of commands in Custom Module.
******************************************************************************/

EXPORT int cmd_get_num_cmds( void )
{
	return num_cmds ;
}


/******************************************************************************
NAME
	cmd_get_cmd

SYNOPSIS
	EXPORT const CLI_COMMAND * cmd_get_cmd( int cmd )

FUNCTION
	Returns details of a specific command from the table.
	This function is required.

RETURNS
	Command details.
******************************************************************************/

EXPORT const CLI_COMMAND * cmd_get_cmd( int cmd )
{
	if (cmd < 0 || cmd >= num_cmds)
		return NULL ;
	return cmds + cmd ;
}


/* utilities */


/*-Custom Commands.---------------------------------------------------------*/



/*==============================================================================
	ui command -- it uses the SimNet's uart buffer.
	Insertions are forceful (overwrite buffers).
==============================================================================*/

reset () {
          ui_output_ws("Resetting emulated eCOG...\nPress \"Go\" after the reset.\n");
	  core_reset_all (); //core_reset_proc(); core_reset_counts(); core_reset_io();
}

static int custom_ui (int argc, char *argv[]) {
	COREVAL	typed_res;
	bool selector = 0;

	if (!OK(core_eval_expr(argv[1], &typed_res, TRUE)))
		return FAILURE;

	if (typed_res.type != CORE_STRING) {
		ui_output_ws("Arg must be a string");
		return FAILURE;
	}

	uart[selector].currIndex = 0; // currIndex for SimNetIO::ser_in()
	strcpy (uart[selector].msgBuf, typed_res.value.val_as_string);
	strcat (uart[selector].msgBuf, "\n"); //APPEND \n AS PER EXPECTATIONS IN THE PICOS LIB ( __inserial.c)
	uart[selector].msgLen = strlen(uart[selector].msgBuf);

	_ui_output_ws("in custom_ui got %s\n",uart[selector].msgBuf );

	if (strcmp(uart[selector].msgBuf,"reset\n") ==0) { //reset
	  uart[selector].msgBuf[0]='\0';
	  uart[selector].msgLen = 0;
	  reset();
	  return SUCCESS;
	}

	//the isr will wake up the DUART driver to read
	if (selector==0) {
	  if (rg.duart.a_int_clr == 0) { //duart A rx interrupts enabled
	    _ui_output_ws ("setting irq 0x33...\n");
	    core_irq_set (0x33, 0);  //duart a rx rdy
	  }
	  else
	    _ui_output_ws ("duart A rx interrupts must have been disabled...\n");
	}

	if (core_proc_state() == SLEEPING) {
	   core_wake_up();
	}

	return SUCCESS;
}

extern int ourMACindex;

//node i x y r
static int custom_node (int argc, char *argv[]) {
  int i;

        if (argc ==1) {
	  i= ourMACindex;
	  ui_output_ws("Get parameters for node %d: x=%d, y=%d, r=%d\n", i, destination(i).x, destination(i).y, destination(i).range);
	  return SUCCESS;
	}

	i = atoi(argv[1]);

	if (i>= MAXDESTS) {
	  ui_output_ws("Node index is too large \n");
	  return FAILURE;
	}

	if (argc ==2) {
	  ui_output_ws("Get parameters for node %d: x=%d, y=%d, r=%d\n", i, destination(i).x, destination(i).y, destination(i).range);
	  return SUCCESS;
	}

	if (argc !=5) {
	  ui_output_ws("Format: node [i, i x y r] \n");
	  return FAILURE;
	}

	destination(i).x= atoi(argv[2]);
	destination(i).y= atoi(argv[3]);
	destination(i).range= atoi(argv[4]);
	ui_output_ws("Set new parameters for node %d: x=%d, y=%d, r=%d\n", atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	return SUCCESS;
}
