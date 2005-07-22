	 MODULE		rasm
	.LARGE
;
; Assembly language helper for radio driver: receive one pulse
;
; ============================================================================ 
;                                                                              
; Copyright (C) Olsonet Communications Corporation, 2002, 2003                 
;                                                                              
; Permission is hereby granted, free of charge, to any person obtaining a copy 
; of this software and associated documentation files (the "Software"), to     
; deal in the Software without restriction, including without limitation the   
; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  
; sell copies of the Software, and to permit persons to whom the Software is   
; furnished to do so, subject to the following conditions:                     
;                                                                              
; The above copyright notice and this permission notice shall be included in   
; all copies or substantial portions of the Software.                          
;                                                                              
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
; IN THE SOFTWARE.                                                             
;                                                                              
; ============================================================================ 

;

;
; This is set for reception through GPIO 14. As parameterizing this is a bit
; tricky (without adding a separate include file for the assembler code), let
; us try to make it a standard ;-).
;
PORT_AD	 EQU		H'ffb6	; GPIO 8-15 STS
PORT_BI	 EQU		12	; Bit position counting from the right
;
; UP_THS	 EQU		11	; > 11
; DN_THS	 EQU		5	; < 5
; Adjusted, July 9. Seems to be a bit better this way
;
UP_THS	 EQU		10	; > 11
DN_THS	 EQU		6	; < 5
;

rcv	 MACRO
	 ld		AL,@PORT_AD
	 IF		PORT_BI
	 lsr		#PORT_BI
	 ENDIF
	 and		AL,#1
	 st		AL,@rc_sig
	 ENDMAC

	.SEG		VAR

rc_cnt	 ds		1	; iteration counter
rc_sig	 ds		1	; received signal status
rc_his	 ds		1	; last history
rc_hic	 ds		1	; "on" count accumulated over history
rc_upt	 ds		1	; up threshold
rc_dnt	 ds		1	; down threshold

	.CODE
;
$rc_ch:

	st	XH,@(-2,Y)
	st	X,@(-1,Y)
;
	ld	AL,#0
	st	AL,@rc_cnt	; the number of iterations
;
rc_loop:
;
;  What makes this code tricky is that its execution time cannot depend on
;  the last-received signal level. In other words, we can only have an exit
;  condition and no internal branches. In yet other words, a non-exit turn
;  of the loop must execute all instructions.
;
	rcv			; get signal status
;
	ld	AL,@rc_his
	ld	AH,@rc_sig	; the new bit
	lsr	#1		; the new bit gets in, the oldest bit gets out
	st	AL,@rc_his
	ld	X,@rc_hic
	subc	X,#0		; subtract the bit getting out
	add	X,@rc_sig	; add the new bit
	st	X,@rc_hic
;
;  Exit condition
;
	ld	AL,#0
	sub	X,@rc_dnt	; down threshold
	addc	AL,#0
	ld	X,@rc_upt	; up threshold
	sub	X,@rc_hic
	addc	AL,#0
;
	bne	rc_exit		; signal level changes
;
;  Keep going
;
	ld	X,@rc_cnt
	cmp	X,@$zzz_rc_limit
	beq	rc_exit		; limit reached
	add	X,#1
	st	X,@rc_cnt
	bra	rc_loop
;
;
rc_exit:
	ld	XH,@(-2,Y)
	ld	X,@rc_cnt
;
	cmp	X,@$zzz_rcl_thrshld+0
	bcc	rc_ex1
	ld	AL,#1
	bra	@(-1,Y)
;
rc_ex1:	cmp	X,@$zzz_rcl_thrshld+1
	bcc	rc_ex2
	ld	AL,#2
	bra	@(-1,Y)

;
rc_ex2:	cmp	X,@$zzz_rcl_thrshld+2
	bcc	rc_ex3
	ld	AL,#3
	bra	@(-1,Y)
;
rc_ex3:	ld	AL,#4
	bra	@(-1,Y)

;
;  Set the last perception of the signal level
;
$rc_setlev:
;
	cmp	AL,#0
	bne	rc_sl1
;
;  The level is 0 (this is in absolute terms), so the expected level is 1
;
	ld	AL,#UP_THS
	st	AL,@rc_upt
	ld	AL,#0
	st	AL,@rc_dnt
	st	AL,@rc_his
	st	AL,@rc_hic
;
	bra	rc_sl2
;
rc_sl1:
;
; The level is 1, so we are expecting zero
;
	ld	AL,#H'7fff
	st	AL,@rc_upt
	ld	AL,#DN_THS
	st	AL,@rc_dnt
	ld	AL,#H'ffff
	st	AL,@rc_his
	ld	AL,#16
	st	AL,@rc_hic
;
rc_sl2:
	bra	0,X

	ENDMOD	rasm
