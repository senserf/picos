Version 0.5

===============================================================================
Quick start:

1. Install SICLE. It can be fetched (cvs export SICLE) from the same repository
   as VUEE. Read the README file in SICLE for hints. To check if SICLE works,
   enter this simple Tcl program from a Cygwin terminal:

                 tclsh
                 package require siclef 1.0
                 crypt abcde
                 exit

   If the call to crypt succeeds and produces some (random looking) output, it
   means that the installation has been successful.

2. Install PICOS. Make sure that VUEE is unpacked in the same directory as
   PICOS, i.e., the two appear at the same level. This is needed because VUEE
   links to PICOS.

3. Install SIDE (wherever). It can be fetched (cvs export SIDE) from the same
   repository as VUEE. Read file INSTALL.txt in SIDE's root directory. 
   IMPORTANT: in response the the maker's question about include library (point
   5.5 in INSTALL.txt), specify the full (or home-relative) path to
   VUEE/PICOS (a subdirectory of VUEE).

4. Go to VUEE/PICOS and execute ./mklinks . This will link VUEE to some files
   in PICOS.

5. Things are set. Go to VUEE/MODELS/RFPING and create the simulator. Execute
   mks -W in that directory. mks is the SIDE compiler.

6. In VUEE/UDAEMON, execute udaemon . This is a Tk script that emulates
   multiple UARTs perceptible by nodes modeled in VUEE.

7. Start the model. In VUEE/MODELS/RFPING, execute side data.txt . Two UART
   terminal windows should pop up (for the two nodes described in the model).
   Remember that diags are written to the emulator's stdout (such that they
   show up even if the node has no UART).

   Enjoy!!

===============================================================================

R061015A:

   Original import. Lots of things to do: reset doesn't work, ways to initially
   distribute ESN's, generic mobility engine driven by external programs. And
   of course GUI, i.e., Tk interface. Wladek's stuff has to be accommodated.

R061021A:

    DIAG messages tagged with Node Id. This is the 'creation' Id corresponding
    to the node number specified in the data file.
    RESET implemented and tested on RFPing. TAGSANDPEGS modified to enable
    reset (previously disabled). A few cosmetic changes.

R061025A:

    A few mods by Wlodek. udaemon patched a bit - to be slightly less stupid.
    diag fixed to properly display formatted items. Segmentation errors on
    reset eliminated.

R061111A:

    A few mods by Wlodek + cosmetics needed to make the stuff compile on 64-bit
    Linux systems.

R061115A:

    Cosmetics. Added app to test LBT.

R061208A:

    NVRAM (EEPROM + FIM) model added.

R061216A:

    Lots of changes. It is now possible to keep a VUEE model and PICOS praxis
    as the same set of sources.
