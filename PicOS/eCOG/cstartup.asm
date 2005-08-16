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

                  MODULE   cstartup
                  .ALL

;
; This version of cstartup has been written for systems that run entirely
; from the eCOG1's internal memory. The memories are mapped as follows:
;
;     Logical                 Physical              Size
; -------------------------------------------------------
; 1.  CODE  H'0000-H'77FF     Flash H'0000-H'77FF    32K
; 2.  DATA  H'0000-H'07FF     Flash H'7800-H'7FFF    2K 
; 3.  DATA  H'E800-H'EFFF     RAM  H'0000-H'07FF     2K
; 4.  DATA  H'FEA0-H'FFCF     Registers              304
;
; At power up the memory map is as follows:
;
;     Logical                 Physical              Size
; -------------------------------------------------------
; 1.  CODE  H'0000-H'00FF     Flash H'0000-H'00FF    256
; 2.  DATA  H'0000-H'00FF     RAM   H'0000-H'00FF    256
; 3.  DATA  H'FEA0-H'FFCF     Registers              304
;

;
; C reserves DATA address 0 for the NULL pointer. The value H'DEAD is put in
; here so that it is easier to spot the effect of a NULL pointer during
; debugging. User memory for constants grows upwards from H'0001. The first
; address is given the equate symbol $??LO_ADDR.
;

                  .SEG     C_RESERVED1
                  ORG      0
                  dc       H'DEAD
$??LO_ADDR        EQU      $

; PG: starting data address
DS_ADDR		EQU 	H'E800

;
; DATA addresses H'EFE0-H'EFFF are used for scratchpad RAM in interrupt mode.
; DATA addresses H'EFC0-H'EFDF are used for scratchpad RAM in user mode.
; DATA addresses H'EFB8-H'EFBF are used for register storage in interrupt mode.
; Then follows the interrupt stack, user stack and user heap.
; User memory for variables grows downwards from the end of the user stack.
; This version of cstartup only contains one area of scratchpad RAM
; which constrains users not to write re-entrant re-interruptable
; code.
; The interrupt stack must start at IY-38 to be compatible with the C compiler.
;

                  .SEG     C_RESERVED2
                  ORG      H'EFB8

$??HI_ADDR        DEQU     $
                  ds       8           ; Interrupt register storage

                  ds       32          ; User Scratchpad
IY_SCRATCH        DEQU     $

$?irq_scratchpad? DEQU     $
                  ds       32          ; Interrupt Scratchpad

; PG  EFB8 + 32 + 32 + 8 = F000 - this is where the ROM data part ends


;
; The registers that control the functional blocks of the eCOG1 are located
; at addresses H'FEA0 to H'FFCF. The C header file <ecog1.h> declares an
; external structure that descibes the registers. This variable is defined
; below.
;

                  .SEG     REGISTERS
                  ORG      H'FEA0
$fd:
$rg               ds       304

;
; C requires the following segments:
;     CONST  - Constants in ROM. For example:
;                 const char c = 'c' ;
;                 printf( "Hello World!" ) ;
;     VAR    - Variables in RAM. These are set to zero by the cstartup code.
;              For example:
;                 int i ;              (in file scope)
;                 static int i ;       (in function scope)
;     INIT   - Initialised variables in RAM. For example:
;                 int i = 9 ;          (in file scope)
;                 static int i = 9 ;   (in function scope)
;     INITC  - Initialisation data for the INIT segment
;     HEAP   - The heap. Required if malloc() etc. are used.
;     STACK  - The stack. Always required.
;
; The memory allocated to each segment is defined by the value of
; $??<segment_name>_SIZE as set below. These sizes can be set manually or, if
; the appropriate line is tagged with !PACK and the -pack option is specified
; to ECOGCL, ECOGCL will write in the size actually required for the segment.
; The sizes of the STACK and HEAP segments must be set by the user.
;

$??ISTACK_SIZE    =        H'0040
$STACK_SIZE       =        H'0100
$??STACK_SIZE     =        $STACK_SIZE
$??HEAP_SIZE      =        H'0000  ; PG - No heap in this application

; ROM segments
$??INITC_SIZE     =        h'0009  ; !PACK
$??CONST_SIZE     =        h'03d6  ; !PACK

; RAM segments
$??INIT_SIZE      =        h'0009  ; !PACK
$??VAR_SIZE       =        h'016f  ; !PACK



; -- Locate DATA segments in memory --

;
; Segments are allocated sequentially by the ??ALLOCATE macro. They may be
; set at fixed addresses by setting ADDR prior to calling ??ALLOCATE.
;


??ALLOCATE        MACRO    seg
                  .SEG     &seg
                  ORG      ADDR
$??&seg!_LO       = ADDR
ADDR              = ADDR + $??&seg!_SIZE
$??&seg!_HI       = ADDR-1
                  ENDMAC


; Allocate DATA ROM

ADDR              = $??LO_ADDR
                  ??ALLOCATE INITC
                  ??ALLOCATE CONST

; Allocate DATA RAM
; PG I don't get it. Why going from the bottom? I am fixing it to
; start from the top and leave as much room for the stack as available

ADDR		= DS_ADDR
                  ??ALLOCATE INIT
                  ??ALLOCATE VAR
                  ??ALLOCATE HEAP

EVAR		= ADDR

; PG The stack goes from the bottom

ADDR              = $??HI_ADDR - $??ISTACK_SIZE - $STACK_SIZE
ESTK		= ADDR
                  ??ALLOCATE STACK
                  ??ALLOCATE ISTACK

; -- Memory initialisation macros --

;
; Segments may be initialised by filling with a constant value using the
; ??SEGFILL macro.  Two symbols are passed, the segment name and the value to
; fill with.  A third symbol (the size) is assumed.
;

??SEGFILL         MACRO    seg, value
                  LOCAL    fill_loop
                  IF       $??&seg!_SIZE
                  ld       x, #$??&seg
                  ld       al, #$??&seg!_SIZE
                  ld       ah, &value
&fill_loop:       st       ah, @(0,x)
                  add      x, #1
                  sub      al, #1
                  bne      &fill_loop
                  ENDIF
                  ENDMAC


;
; Segments may be initialised by copying an initialisation segment with
; the ??SEGCOPY macro. Two symbols are passed, the source and destination
; segment names.
;

??SEGCOPY         MACRO    src, dest
                  IF       $??&src!_SIZE NE $??&dest!_SIZE
                  .ERR     "Different segment sizes"
                  ENDIF
                  IF       $??&src!_SIZE
                  ld       x, #$??&src
                  ld       y, #$??&dest
                  ld       al, #$??&src!_SIZE
                  bc
                  ENDIF
                  ENDMAC


;
; Fills a block of memory with a value. Three values are passed, the start
; address for the block, the number of addresses to write to and the value
; to be written.
;

??MEMFILL         MACRO    start, length, value
                  LOCAL    fill_loop
                  ld       x, &start
                  ld       al, &length
                  ld       ah, &value
&fill_loop:       st       ah, @(0,x)
                  add      x, #1
                  sub      al, #1
                  bne      &fill_loop
                  ENDMAC

;
; This is the minimal interrupt routine. The contents of FLAGS is restored
; as the program counter is restored using rti.
;
                  .CODE
; PG This bypasses the interrupt vector. We should write an interrupt
; handler for the DUART.
                  ORG      H'40

$zzz_minimal_handler:
                  st flags,@(-33,y)    ; Store Flags
                  st al, @(-34,y)      ; Store AL

                  ld al, @(-33,y)      ; Put Flags into AL
                  or al, #h'0010       ; Set usermode
                  st al, @(-33,y)      ; Store the value to be restored to Flags

                  ld al, @(-34,y)      ; Restore AL
                  rti @(-33,y)         ; Restore PC and Flags


;
; The address exception can happen often during development. A handler
; is put here to catch the exception.
;

$zzz_address_error:
                  st flags,@(-33,y)    ; Store Flags
                  st al, @(-34,y)      ; Store AL

                  ld al, @(-33,y)      ; Put Flags into AL
                  or al, #h'0010       ; Set usermode
                  st al, @(-33,y)      ; Store the value to be restored to Flags

                  brk                  ; Alert the user if in debug mode

                  ld al, #h'a
                  st al, @h'ff69       ; Clear status in mmu.adr_err

                  ld al, @(-34,y)      ; Restore AL
                  rti @(-33,y)         ; Restore PC and Flags

; The release operation - to return from a process directly to the scheduler
; loop
$zzz_set_release:
		st	Y,@release_sp
		st	X,@release_rt
; this assumes that the release address is within the first 64K of memory,
; which, of course, is going to be the case, as we are talking about an
; address within the scheduler loop

$zzz_release:
		ld	Y,@release_sp
		ld	AL,#h'0
		mov	XH,AL
		bra	@release_rt

;
; Start of Code.
;

;
; Initialise the memory manager. The MMU permits blocks of size 2^n to be
; mapped. This means that IROM H'7780-H'7FFF actually appears in both DATA
; and CODE side. Users must ensure that their code does not exceed CODE
; address H'777F.
; At power on the DATA memory map does not contain the IROM. This means
; that the code must not use DATA memory until the memory map is
; configured.
;


$?cstart_code:

                  ld x, #h'ff43
                  ld al, #h'10
                  st al, @(0,x)        ; mmu.translate_en
; PG shouldn't this be done at the end - just for the sake of cleanness?


;
; 1.  CODE  H'0000-H'77FF     IROM H'0000-H'77FF    32K
;
                  ld al, #h'0
                  st al, @(1,x)        ; mmu.flash_code_log
                  st al, @(2,x)        ; mmu.flash_code_phy
                  ld al, #h'7F
; PG: 7F is 7 bits, thus the segment size is 2^7 * 256 = 128 * 256 = 8000
; If I understand this correctly, there's an overlap in this mapping, because
; DATA 0000-07FF is mapped into IROM 7800-7FFF, but this is OK.
                  st al, @(3,x)        ; mmu.flash_code_size


;
; 2.  DATA  H'0000-H'07FF     IROM H'7800-H'7FFF    2K 
;
                  ld al, #h'0
                  st al, @(h'D,x)      ; mmu.ram_data_log
                  ld al, #h'78
                  st al, @(h'E,x)      ; mmu.ram_data_phy
                  ld al, #h'7
; PG: Now we have 2^3 * 256 = 2K
                  st al, @(h'F,x)      ; mmu.ram_data_size
;
; 3.  DATA  H'E800-H'EFFF     IRAM H'0000-H'07FF    4K
;
                  ld al, #h'E8
                  st al, @(h'10,x)     ; mmu.ram_data_log
                  ld al, #h'0
                  st al, @(h'11,x)     ; mmu.ram_data_phy
                  ld al, #h'7
                  st al, @(h'12,x)     ; mmu.ram_data_size
; PG: registers are pre-mapped and there's no way to change that

;
; Clear the address exception. These can be active when the software has
; done a self reset using the if_reset and cpu_reset bits.
;

                  ld al, #h'a
                  st al, @h'ff69       ; Clear status in mmu.adr_err
;
; Initialise the CACHE.
; This mode allows the Emulator to insert breakpoints in the cache.
;

; PG: this maps cache registers into DATA
                  ld al, #h'10
                  st al, @h'ff59
                  ld al, #h'12
                  st al, @h'ff5a

; PG: this makes the mapping effective (cache disabled on startup, I guess)
                  ld al, @h'ff43
                  or al, #h'180
                  st al, @h'ff43

; #########################################################################
; PG: the cache consists of two blocks of 512 entries, each entry occupying
; 2 words. The first 256 words are the contents, the subsequent 256 words
; are the tags, with LSB being the lock bit. As the maximum address is 24
; bits, and the tag refers to the upper 15 bits of the address, this makes
; all entries effectively point out of memory.
; #########################################################################
                  ??MEMFILL #h'1000, #h'100, #h'0
                  ??MEMFILL #h'1100, #h'100, #h'8000
                  ??MEMFILL #h'1200, #h'100, #h'0
                  ??MEMFILL #h'1300, #h'100, #h'8000

; PG: this unmaps cache registers from DATA
                  ld al, @h'ff43
                  and al, #h'fe7f
                  st al, @h'ff43

; PG: and this activates the cache. The x'10 is 'lock bit enable' - to be
; used for tracing. All other options are cleared, which makes sense.
                  ld al, #h'10
                  st al, @h'ff41
; PG: 3 = both banks enable
                  ld al, #3
                  st al, @h'ff42

                  nop                  ; Three NOPs are required following
                  nop                  ; the enable
                  nop


;
; Initialise segments. The HEAP and STACK are filled with H'9999 and H'aaaa
; respectively so that their maximum runtime extents can be checked. The
; INIT segment is set from the ROM initialisers in the INITC segment. The non
; initialised RAM segment VAR is set to zero (compiler puts 0 initialised
; variables in these segments as well as uninitialised ones,x).
;

                  ??SEGFILL HEAP, #h'9999
                  ??SEGFILL STACK, #h'B779
                  ??SEGFILL ISTACK, #h'B77A
                  ??SEGCOPY INITC, INIT
                  ??SEGFILL VAR, #h'0


; Set interrupt stack pointer.

                  ld y, #IY_SCRATCH

; clear possible address exception in mmu

                  ld al, #h'a
                  st al, @h'ff69

; Set user mode flag to allow interupts.

                  st flags, @(-1,y)
                  ld al, @(-1,y)
                  or al, #h'10
                  st al, @(-1,y)
                  ld flags, @(-1,y)


; Set usermode stack pointer

                  ld y, #$??STACK_HI

		  bra $zzz_sched
; No return from there

		.SEG	VAR
release_sp	ds	1
release_rt	ds	1

		.SEG	CONST
; Make the boundaries of data and stack available to the program
$evar_		dc	EVAR
$estk_		dc	ESTK
                  ENDMOD
