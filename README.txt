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

R061216B:

    Lots of changes. It is now possible to keep a VUEE model and PICOS praxis
    as the same set of sources.

R061223A:

    Agent connection paradigm revised, udaemon reimplemented. Now there is a
    single socket set up by VUEE for all agent connections. Its number is 
    4443 (can be made flexible later, e.g., definable in data). An agent, like
    udaemon, connects to this socket and specifies the service type by sending
    8 bytes with the following contents:

    Bytes 0-1 - magic shortword 0xBAB4 in network format, i.e., MSB first;
    Bytes 2-3 - the request code (a short int in network format):
                    1 - UART
                    2 - Pin control
                    3 - LEDs display
                    4 - mobility
    Bytes 4-7 - the node number (according to the numeration in the data file).

    VUEE responds with a single byte, which is 129 decimal if the request is OK.
    Otherwise, it means that there was an error (see agent.h in VUEE/PICOS for
    the current list of error codes.

    For UART, things work pretty much as before. After the initial handshake,
    the clien't (agent's) socket is directly connected to the UART. Note that,
    unlike before, the connection is established after the VUEE model starts,
    so some UART output can be potentially lost. This can be remedied as
    follows.

    The data format describing a UART has changed. See this for an example:

	    <uart rate="9600" bsize="12">
		<input source="socket" type="untimed"></input>
		<output target="socket" type="held"></output>
	    </uart>

    Now there is no hostname/port specification (because the connection is
    reversed). Type HELD for the output end of the socket means that any
    UART output before the first connection is to be saved (it goes to a
    temporary buffer in memory) and will be presented (immediately flushed
    to the socket) on first connection. From then on, if the agent disconnects,
    the output WILL NOT be saved. As soon as the agent connects again, the
    new output will be showing up on the socket.

    It is very easy to save the output always when the UART is disconnected.
    Another option? I am not sure as this may put stress on memory requirements
    of the simulator, when you abandon a UART with lots of output to come.

    Udaemon's terminals have a HEX option to display and accept data in
    hexadecimal.

    Pin control, LEDs and mobility protocols still to be devised.

--------:

    Global HEX option added to udaemon.

R070119A:

    Lots of updates to VUEE. Agent protocol implemented. Udaemon extended by
    CLOCK, PINS, LEDS, MOVER. Documentation coming up.

R070203A:

    EEPROM (FLASH) model brought up to date with recent changes in PicOS.
    EETest revritten according to the compatibility rules and moved to
    PICOS/Apps/VUEE.

R070217A:

    Udaemon polished up a bit, document added to VUEE/Doc, tested on Cygwin,
    32-bit and 64-bit Linux.

R070325A:

    MOVER renamed ROAMER and made available from the input data. Random
    way point mobility model implemented. Added preinits, RSSI and SETPOWER
    conversion tables in the input data. Agent protocol poished up.

R070402B:

    Channel and random waypoint mobility models cleaned and moved to the
    standard SMURPH library (IncLib) from where they can be used in non-PicOS
    models. File chan_shadow.h need not be included by the praxis anymore.
    The <cutoff> parameters is no longer the absolute distance, but the
    signal level (factored by the receiver sensitivity) below which its impact
    on the receiver is deemed irrelevant. It should be set at a fraction of
    background noise. Operations running and killal implemented.

R070718A:

    Cosmetics to make VUEE compatible with the improved channel model in
    SIDE.
    
