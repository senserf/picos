#include "sysio.h"
#include "ecog1.h"
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

extern void lcd_send (int data) ;


void _lcd_out (const char *m) {

 char *p;

 for (p=(char*)m; (*p) !=0; p++) {

	/* Data */
	rg.io.gp8_11_out = IO_GP8_11_OUT_SET8_MASK;

	rg.io.gp8_11_out = IO_GP8_11_OUT_CLR9_MASK;
	/* Set E high to enable write */
	rg.io.gp8_11_out = IO_GP8_11_OUT_SET10_MASK;

	/* Second stage */
	lcd_send (*p);
  }

}

//never giving up the CPU to keep up with those faster simulated devices!
void sim_lcd (const char *m) {

  _lcd_out("\nLCD: ");
  _lcd_out(m);
  _lcd_out("\n");
}
