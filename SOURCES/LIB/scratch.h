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

/* --- */

/* ------------------------------------------------------------- */
/* SMURPH version of the C++ library (the minimum needed subset) */
/* ------------------------------------------------------------- */

#define	OBUFSIZE  256		// Size of an ostream buffer

#include        <sys/types.h>
#include	<sys/wait.h>
#include        <sys/time.h>
#include        <sys/times.h>
#include 	<sys/stat.h>
#include 	<sys/file.h>
#include        <setjmp.h>
#include        <fcntl.h>
#include        <signal.h>
#include        <unistd.h>
#include        <stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<errno.h>
#include	<stdarg.h>
#include	<time.h>
#include	<math.h>

#include	<iostream>
#include	<fstream>

/* --- END STANDARD INCLUDES --- */
