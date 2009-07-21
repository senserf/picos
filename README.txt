Version 0.7

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
    
R070818A:

    ser_outb added and __outserial improved as per Wlodek's fixes. Also scan
    fixed to use the proper operand size for accessing local word args.

R070821A:

    Fixed a bug in reset causing occasional SIGSEGVs. Redone reset to make
    it ready for turning nodes on/off from the agent (should be next commit).
    Manual to be updated (next commit?) as the init/reset declarations in
    node.cc have been changed. Also NODENAME is no longer needed.

R070822A:

    It is now possible to start/stop/reset a node both from the data as well as
    via udaemon. Changes to the praxis layout:

	- node.cc, node.h (see Apps/VUE/[RFPing/TNP] in PicOS (R070822) and
	  read pages 11 and 12 in the manual. Note: in TNP, see files 
	  node_peg.cc, node_tag.cc, node_peg.h, node_tag.h.

	- NODENAME no longer needed: file app.h [app_tag.h, app_peg.h] (just
	  remove the definition)

    A new component (PANEL) in udaemon, hopefully self explanatory.

R070824A:

    Deallocating NULL was illegal in VUEE (sorry about that). It is now.

R070824B:

    Added a few safety hatches (matching those in PicOS) accounting for various
    NULLs (ser_in, ser_out, vscan, ...)

R070830A:

    Fixed a couple of minor problems in board.cc resulting in some data that
    should be sent to the SMURPH output file being in fact writtent to standard
    output.
    Function seconds () after reset now starts from zero.
    Fixed a minor problem in find_strpool (board.cc) resulting in unnecessary
    allocation of duplicate strings.
    Fixed initialization of EEPROF/IFLASH to make sure that all cases involving
    defaults are handled properly.
    
R070917A:

    Fixed a bug in rfmodule.cc causing delayed RX off, i.e., one packet after
    RX off was received.

R071031A:

    Matching fix to PicOS (the same date): removal of TARP macros (defineable
    by the praxis), which have been replaced by functions. Note that SIDE
    R071028A is required to run this.

R071114A:

    Added ee_size, which was accidentally missing from the set of ee ops.

R071206A:

    Added a couple of links to PICOS. Had to split some includes to eliminate
    complaints of the CYAN compiler. Matches PICOS R071206A.

R071206C:

    Added ldelay and lhold. Not added dleft, ldleft, lhleft (perhaps nobody
    needs them, and they are a bit tricky)

R071210A:

    Added support for sensors and actuators.

R071211A:

    PHYSOPT_STATUS extended as per R071211A of PICOS.

R080313A:

    Fixed a problem with memfree.

R080326A:

    Using a receive buffer in rfmodule, instead of recycling the internal
    "Packet" buffer. The way it was, the plugin could modify the internal copy
    of the packet confusing receivers at other nodes.

R080330A:

    Changed (and debugged) the handling of the so-called pin (pulse) monitor,
    aka counter and notifier. Read section 6.6 from the manual (version 0.65).

R080412A:

    Fixed a memory leak caused by not cleaned packet payloads. The argument of
    udaemon now allows you to specify the host as well as the port, e.g.,

		udaemon sheerness.cs.ualberta.ca:4431
		udaemon sheerness.cs.ualberta.ca	(default port)
		udaemon sheerness.cs.ualberta.ca:	(same thing)
		udaemon 5533				(localhost)
		udaemon :5533				(same thing)

R080417A:

    Fixed a bug in agent.cc causing that sensors initialized from <defaults>
    were shared by all nodes.

R080715A:

    Added capability to have multiple channels and multiple rates with
    different propagation properties.
    Process number limit.
    Note: the format of input file has changed slightly (old files won't work).
    I have tried to modify all XML data sets. Look in Apps/VUEE/* for examples.
    Requires SIDE R080714B.

R080723A:

    Added aliases: ptleft () for crunning (NULL), getcpid () for running (NULL).

R080820A:

    Added methods setseconds and sectomin to make things compatible with
    PicOS R080820A. sectomin is a stub the minute clock will probably go.

R081110A:

    Cosmetic changes to agent.cc to appease new gcc on Linux.

R090312A:

    Cosmetics required to accommodate modifications to EEPROM driver in
    PicOS (ee_open, ee_close - they are void in VUEE).

R090519A:

    The UART rate specified in the data file is now stored as a UART attribute
    and sent to UDAEMON upon connection. The user-specified rate is verified by
    VUEE against the standard set of values acceptable by PicOS.

    The UDAEMON protocol has been modified a bit, such that the status code
    returned by VUEE agent after a connection is now one word, instead of one
    byte (as it used to be). This way, more info can be returned by the agent,
    notably the bit rate for UART, which is needed by the U-U feature of
    UDAEMON's UART window. This feature is described in the updated document.
    See also R090519A in PICOS.

R090526A:

    It is now possible to have radio-less nodes (see the document). The format
    of input data has changed slightly (I have modified all the relevant data
    files in PicOS praxes) by including a new <radio> element encapsulating
    all the radio related parameters of a node.

    By the same token the <channel> specification in the data set is now
    optional, e.g., you can set up the model of a single node not interfaced
    to a radio channel.

    PHY interface for UART added (mode 'N' described in Serial.pdf in PicOS).
    This interface is selected by using the mode attribute of <uart>, e.g.,
    <uart rate="9600" bsize="12" mode="packet"> (see Apps/VUEE/XRSTest for
    an illustration).

    XRS (aka AB) protocol included. You can use it with any PHY, e.g., with
    the packet mode of UART (see Serial.pdf in PicOS).

    Lots of cosmetics. Not everything has been tested yet. I am planning to
    add LCDG, OEP, and the functions operating on images (libLCDG), so this
    is just a snapshot.

R090528A:

    EEPROM/IFLASH model redone and greatly enhanced. Main features:

	- preinitialization from the data set and from files
	- backing files (i.e., the storage can be directly mirrored to/from
	  a file)
	- it is possible to change the default "erase byte" (from 0xFF) to
	  anything

    Read the red parts from VUE_doc.pdf.

R090529A:

    Fixed a problem with UDAEMON's UART U-U connection. The selection didn't
    work sometimes (you needed a second click) because "button release" in
    tk_optionMenu (which I originally used) is not the right event for the
    occasion.

R090609A:

    Removed node type NNode and TNode. Added a model for Nokia LCDG.
    Lots of cosmetics and re-shuffling. Read RTAGS from PICOS for details.
    Changes to the document are forthcoming.

R090629A:

    Fixed a bug in board.cc causing segmentation faults when the data file
    didn't include <eeprom> at all.

    Manual updated, the recommended conversion procedure significantly
    simplified. Requires PICOS R090629A.

R090719A:

    Added lcdg_end () to the library of LCDG driver functions to indicate
    the end of image rendering (the end of an lcdg_render sequence). This
    operation is void in PicOS, but in VUEE it reduces the update overhead.
    Requires PicOS R090719B.

R090720A:

    udaemon now accepts a geometry file specifying locations for its windows.
    By default, the file is assumed to be geometry.xml and sought in the
    directory in which udaemon is called. You can say:

	udaemon -G filename

    to point to it explicitly. If no geometry file is provided, the script
    will position windows as before.

    A sample geometry.xml file is provided in PICOS/Apps/VUE/SEAWOLF/YEP.
    I will document it soon.

    When LCDG is displayed and the node goes off, the display is cleared, i.e.,
    forced into the off state.
