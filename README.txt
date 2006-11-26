   -------------------------------------------------------------------------

          %%%%%%    %%   %%    %     %    %%%%%%     %%%%%%     %     %
         %          % % % %    %     %    %     %    %     %    %     %
         %          %  %  %    %     %    %     %    %     %    %     %
          %%%%%     %  %  %    %     %    %%%%%%     %%%%%%     %%%%%%%
               %    %     %    %     %    %  %       %          %     %
               %    %     %    %     %    %   %      %          %     %
         %%%%%%     %     %     %%%%%     %    %%    %          %     %

   -------------------------------------------------------------------------

                     %%%%%%    %%%    %%%%%%     %%%%%%%
                    %           %     %     %    %
                    %           %     %     %    %
                     %%%%%      %     %     %    %%%%%% 
                          %     %     %     %    %
                          %     %     %     %    %
                    %%%%%%     %%%    %%%%%%     %%%%%%%

   -------------------------------------------------------------------------



                               VERSION 2.80

        Author:
                    Pawel Gburzynski
                    University of Alberta
                    Department of Computing Science
                    321 Athabasca Hall
                    Edmonton, Alberta, CANADA T6G 2E8
                    tel.:  (780) 492-2347
                    fax:   (780) 492-1071
                    email: pawel@cs.ualberta.ca

  ----------------------------------------------------------------------------
 |  Note: The following book about SMURPH/SIDE is available:                  |
 |        Pawel Gburzynski                                                    |
 |        Protocol Design for Local and Metropolitan Area Networks            |
 |        Prentice Hall, 1996                                                 |
 |        ISBN 0-13-554270-7                                                  |
  ----------------------------------------------------------------------------

  ----------------------------------------------------------------------------
 |  Note: Unpacking/installing  instructions  are  in  the  MANUAL (directory |
 |        MANUAL), chapter: SIDE under UNIX.  Please read that chapter before |
 |        you start.                                                          |
  ----------------------------------------------------------------------------

  ----------------------------------------------------------------------------
 |  Note: This version of SIDE runs under Linux and Cygwin                    |
  ----------------------------------------------------------------------------

  ----------------------------------------------------------------------------
 |  Copyright (C) 1994-2006 P. Gburzynski                                     |
  ----------------------------------------------------------------------------

       ----------------------------------------------------------------
      >  SMURPH (a System for Modeling Unslotted Real-time PHenomena)  <
      >  is  a  package for simulating communication protocols at the  <
      >  medium access control (MAC) level. SMURPH can be viewed as a  <
      >  combination of a protocol specification  language  based  on  <
      >  C++   and  an  event-driven,  discrete-time  simulator  that  <
      >  provides  a   virtual   (but   realistic)   and   controlled  <
      >  environment  for  protocol execution. SMURPH can be used for  <
      >  designing (prototyping)  low-level  communication  protocols  <
      >  and   investigating  their  quantitative  (performance)  and  <
      >  qualitative (correctness) properties.                         <
      >                                                                <
      >  SMURPH descends from LANSF --  an  earlier  version  of  the  <
      >  package  (Software  Practice  and  Experience,  vol.  21(1),  <
      >  January 1991, pp. 51-76) which was based on plain C.          <
      >                                                                <
      >  A SMURPH program consisting of  the  protocol  source  code,  <
      >  network description, and traffic specification is translated  <
      >  into  a program in C++. The code in C++ is then compiled and  <
      >  linked with the SMURPH  library.  This  way  a  stand-alone,  <
      >  executable  module  is  built  which  can  be  viewed  as  a  <
      >  simulator for the system  described  by  the  user  program.  <
      >                                                                <
      >  When you get a copy of SMURPH or LANSF, please send  a  note  <
      >  to  'pawel@cs.ualberta.ca'.  I  would  like  to  know who is  <
      >  using the packages and (if it is  not  a  secret)  for  what  <
      >  purpose.                                                      <
      >                                                                <
      >  The  SMURPH  project  was  supported in part by NSERC grants  <
      >  OGP9110 and OGP9183, and by a grant from  Lockheed  Missiles  <
      >  and Space Company, Inc.                                       <
       ----------------------------------------------------------------

       ----------------------------------------------------------------
       ----------------------------------------------------------------
      > The present version of SMURPH has been renamed  "SIDE",  which <
      > stands for Sensors In a Distributed Environment. SIDE includes <
      > SMURPH as its subset and is fully compatible with SMURPH 1.8x, <
      > except  for  checkpointing,  which has been disabled  (perhaps <
      > temporarily) in SIDE.                                          <
       ----------------------------------------------------------------
       ----------------------------------------------------------------


 Summary of changes:
 ===================

 11/12/90: Special preprocessor added (SMPP) to convert smurph constructs into
           C++ constructs. Previously this was handled by the compiler
           preprocessor and the smurph constructs were regular macros.
           The syntax of many constructs has changed as now we are not
           restricted by the somewhat limited power of the C macro apparatus.

 15/12/90: New environment variable (TheExitCode) added. This variable is set
           upon the Kernel's DEATH event to indicate the termination reason.

 16/12/90: Fixed miscellaneous small and a few serious problems in DSD.

 18/12/90: Textual state identifiers are no longer moved around by SMURPH;
           they are stored statically (SMPP) and then referenced upon demand.
           This should improve the execution time a bit.

 19/12/90: Process states specified in 'inspect' (observers) are now
           automatically associated with the proper process. This means that
           when a process state is specified in 'inspect' (it is not ANY),
           the process type must be specified as well, i.e. it cannot be ANY.

 19/12/90: Fixed a bug in DSD which caused that removal of EObjects was not
           recognized properly.

 20/12/90: Fixed a bug in mks which caused that '-b 0' was not treated properly.

 20/12/90: Fixed a bug in global.h causing compilation errors when smurph was
           created with '-b 0'.

 22/12/90: Fixed a bug in link.c causing that in Kernel exposure with mode 0
           the event id was confused.

 23/12/90: Fixed a problem in main.c that caused segmentation violation for
           certain configurations of link events. The code for deallocating
           request chain has been moved before restarting the process (and
           simplified a little).

 25/12/90: Fixed a bug in main.c that caused incomplete output when the
           simulator was called with the '-c' option. Simulator call arguments
           are now included in the output header.

 27/12/90: Fixed a bug in client.c that caused incorrect mode 2 exposure of
           Client.

 30/12/90: After a successful getPacket, the environment variable TheMessage
           (Info01) returns a pointer to the message from which the packet
           has been acquired, if the message still remains queued; or NULL,
           otherwise. Previously Info01 returned the packet pointer. That was
           somewhat redundant and not much useful.

 30/12/90  maker now asks for the pathname of a directory where include files
           for protocol programs are to be kept. It is possible to have a
           standard library of protocol include files.

 01/01/91  SMPP now detects 'expose' not preceded by 'symbol::', which is a
           confusing error leading to a recursion loop. Detection of
           nonexistent exposure mode has been modified to avoid problems when
           supertype exposure is called from a subtype exposure.

 01/01/91  Fixed DSD to recognize properly display modes for multiple templates
           specifying base and standard names.

 02/01/91  Fixed a bug in client.c causing incorrect calculation of scaling
           for Client and Traffic mode 2 exposures.

 07/01/91  Fixed a bug in link.c causing (incorrectly) the recognition of the
           EOT and EMP events for aborted packets on a PLink.

 09/01/91  '-t' option added to mks.

 10/01/91  SMPP fixed to recognize 'terminate' for 'this' process. Such a
           'terminate' is automatically followed by 'sleep' to make sure that
           the terminated process does not regain control.

 10/01/91  Code for 'create' (generated by SMPP) changed slightly. The order of
           'start' and 'setup' for observers has been reversed. Previously an
           observer was started before its setup was executed.

 11/01/91  'trace' processing added to SMPP.

 13/01/91  'create' syntax augmented to include (optionally) the identifier of
           the station in the context of which the object is to be created.
           Upgrade to version 1.01

 31/01/91  A private version of i/o library and malloc added. This version
           can be optionally selected upon initialization (maker).
           Upgrade to version 1.02

 01/02/91  Improved code in DSD for reading templates so that now templates
           are read much faster.

 07/02/91  The simulator executable is now automatically 'stripped' unless
           mks was called with the '-g' option.

 15/02/91  The packet Receiver (and Sender) attribute can now be NONE which
           is recognized as a special case, e.g. a packet with no specific
           address (which is different from 'broadcast').

 16/02/91  The method of linking the private SMURPH library has been changed.
           Previously, the private version of malloc was not recognized
           (at least on some machines) and the standard version was taken
           instead.

 18/02/91  The default precision of type BIG assumed by mks when '-b' is not
           specified is now 2 (previously it was 1).

 08/03/91  Fixed a bug in lib.c + iolink.c (private i/o library) causing
           checkpoint restart errors.

 11/03/91  Fixed a bug in maker causing problems with creating a non-existent
           'bin' directory.

 15/03/91  Fixed 'busy' and 'idle'

 20/03/91  Fixed a minor bug in SMPP which caused incorrect line numbering
           in a file containing smurph constructs spanning multiple lines.

 21/03/91  Fixed a bug in display.c (flushSupplist) that might cause
           segmentation violation in a situation involving generation of
           many displayable objects between two display flushes to DSD.

 24/03/91  Rewritten some parts of SMPP, global.h, etc. to make SMURPH
           compile with the AT&T cfront compiler.

 25/03/91  It is now possible to send a signal to the System station. Some
           processes, e.g. non-standard traffic generators, can be created
           (and communicated) in the context of the System station.

 26/03/91  SMPP now detects and diagnoses attempts to use 'new' for SMURPH
           object types. Upgrade to version 1.10

 01/04/91  Fixed a bug in Timer's wait that caused occasional modification
           of the delay argument. The problem showed up when clock tolerance
           was non-zero and BIG_precision was != 1.

 06/04/91  Fixed rvar.c to avoid taking sqrt of a possibly negative value
           in calculating standard deviation for 0 variance.

 07/04/91  Fixed a bug in global.h causing compilation errors with the '-i'
           option of mks. Fixed a bug in rvar.c causing 'division by zero'
           errors while updating BIG-type RVariables.

 08/04/91  It is now illegal to terminate a process that owns some exposable
           objects. Previously, the termination of such a process resulted in
           dangling pointers in the ownership hierarchy which was confusing
           for DSD. This doesn't apply to the Kernel process (but Kernel is
           never actually terminated).

 10/04/91  The output file produced by smurph is now terminated with
           "@@@ End of output". This line is intended to be used by programs
           for automated experiment supervision (SERDEL) to determine whether
           the output file is complete or not.

 15/04/91  Code for checkpointing rewritten and rearranged. Now smurph checks
           for a checkpoint request in its main event-processing loop, so that
           checkpoints are always performed at a "safe" state. Previously there
           were some random (and difficult to diagnose) problems with
           checkpointing -- most likely resulting from interrupted system
           calls. They may be gone now.

 19/04/91  It is now possible to define a limit on the size of message queues:
           individually for each station, individually for each traffic
           pattern, or globally. New method 'setQSLimit' (also callable as a)
           global function) added to 'Traffic' and 'Station'. New option '-q'
           added to 'mks'.

 19/04/91  Mode 2 global screen exposure for Client fixed to display the
           message count rather than the combined bit count. The manual says
           that it should be the message count and I think that the manual is
           right.

 15/05/91  Fixed a bug in main.c that caused memory allocation problems when
           stdin was used by default as the input file.

 24/05/91  Fixed a bug in connect.c that caused incorrect socket port
           specification on little endian machines.

 29/07/91  Removed complaints about an empty receiver set for traffic patterns
           handled by the standard Client. The receiver set can be empty in
           which case the Receiver attribute of a generated message is NONE.

 29/07/91  The definition of ALL (special argument for addSender/addReceiver)
           changed to ((Station*)ALL). Previously, explicit ALL didn't work as
           the argument was treated as an integer.

 30/07/91  Initialization for the multiple precision arithmetic package is now
           correctly performed before any user-introduced initialization,
           possibly involving BIG arithmetic. Previously, static initialization
           of a BIG type object from an expression involving BIG or floating
           point values did not work.

 30/07/91  The 'usage' menu of mks fixed to say that the default precision for
           BIG objects is 2, not 1.

 30/07/91  An ostream buffer added in the nonstandard i/o package to speed up
           disk i/o across NFS.

 18/08/91  Hacks added to make SMURPH work with the Sun version of the AT&T
           cfront compiler. The code for determining the compiler type has been
           centralized and put into version.h.

 20/08/91  Random generators for toss and flip rewritten such that they don't
           use fp operations. Option '-8' added to mks. With this option the
           user may select standard random number generators (the drand48)
           family instead of the SMURPH's private generator. The standard
           generator is slightly slower, but it has a longer cycle.

 28/08/91  Fixed connect to detect the situation when the same port is
           connected twice to two different links. Until now such mistakes
           would pass unnoticed with disastrous effects.

 29/08/91  Fixed a bug in SMPP which caused that occasionally (quite seldom)
           the tail of the SMPP output was truncated, if data happened to
           hit the circular buffer boundary with an unfortunate configuration
           of pointers.

 29/08/91  The smurph.h file no longer has to be explicitly included in
           protocol files. It is included automatically by SMPP. The name of
           the include file has been changed to Smurph.h. The smurph.h file
           is now empty, so that its inclusion doesn't affect anything.

 06/09/91  Fixed the "too many errors" problem in SMPP.

 11/09/91  Fixed some non fatal problems in standard exposures. SMPP now
           accepts 'wait' preceded by '.', e.g. S->MyPort.wait (BOT, ...);
           Upgrade to version 1.12.

 11/09/91  DSD fixed to properly recognize parameters -t and -u (as advertised
           in the documentation).

 12/09/91  Fixed a bug in DSD causing status misinterpratation for some
           templates selected for base types.

 19/09/91  Fixed a few glitches in description files for EXAMPLES.

 19/09/91  mks -r now works with the AT&T compiler. Fixed a bug in client.c
           causing display problems when the simulator was created with -i.

 24/09/91  The Signal and Queue AIs have been removed and replaced with
           the dynamic Mailbox AI. Upgrade to version 1.5 which is no longer
           compatible with the previous versions.
           DSD modified to present menu of window types with textual comments
           when a window is added. One doesn't have to remember any longer what
           particular modes stand for. The template format extended to provide
           room for a textual comment to be associated with a template.

 24/09/91  setEtu/setItu checks whether it is called at the very beginning
           of simulation, before objects have been created.

 25/09/91  Network topology exposure (System->printTop, System->printATop) now
           forces 'buildNetwork' automatically. Thus, it is (formally) legal
           to invoke this exposure at any moment, but it is illegal to create
           new network objects after the network has been exposed.

 26/09/91  Preprocessing of packet setup and user declared traffic performance
           measuring methods added to SMPP. It is now legal to specify 
           message/packet subtype pointers (not just Message/Packet pointers)
           as argument types for these methods.

 28/09/91  A process that fails to issue at least one wait request in its
           current state becomes terminated. Previously such a process was left
           as a zombie.

 05/12/91  The environment variables at the target state of 'proceed' retain
           their previous values, as they were before 'proceed' was executed.

 17/12/91  Fixed a bug in main.c causing addressing errors while restarting
           processes waiting for process events.

 18/12/91  Improved the printDef exposure of Client. Previously, the weights
           of communication groups were displayed in the internal (confusing)
           format when the exposure was called after the protocol had been
           started.

 19/12/91  Fixed a bug causing that a process that terminated itself implicitly
           without issuing any wait requests at its last state was actually
           terminated twice. This bug was introduced as part of the feature
           introduced on 28/09/91

 19/12/91  It is now possible to send simple signals directly to processess.
           This feature has been implemented as a minor extension to the
           process AI. Inter-process signals augment the Mailbox mechanism
           and are more natural in many cases.

 19/12/91  Removed the code for setting Info01/Info02 by some functions for
           which this feature seemed dubious.

 01/02/92  It is now legal to have a code-less process type. The code method
           is then inherited from the supertype. In consequence, "perform"
           must be explicitly announced in a process type declaration, if it
           is ever defined for the process type.

 08/02/92  It is now legal to define subtypes of a user-defined observer
           type.

 26/02/92  Fixed problem with linking SMURPH on an sgi running system 4.0.1

 02/03/92  Fixed a bug causing segmentation violation when observers were
           active and a process explicitly terminated itself.

 07/03/92  Fixed a bug in Mailbox::erase () for a queue-type mailbox causing
           'inconsistent count' problems.

 10/03/92  Fixed a bug in CGroup setup causing segmentation violation when
           the senders' weights weren't specified, but the receivers' weights
           were.

 13/03/92  Fixed a bug in client.c (select_station) causing segmentation
           violation when an SGroup was created with NULL station list (all
           stations) and then it was used to define a communication group
           with explicit weights. The error occurred when an attempt was
           made to select a station from the SGroup.

 16/04/92  Upgrade to version 1.52. Hacks added to compile SMURPH with the
           g++ compiler version 2. The problems mostly resulted from the
           new organization of includes.

 07/06/92  Fixed a bug in the checkpoint restart code (introduced with the
           previous modification) causing restart errors. Sorry about that.

 21/06/92  Fixed a bug in DSD causing problems when the smurph list was
           longer than the screen height.

 26/06/92  It is now possible to suspend and resume the Client (as well as
           individual traffic patterns) dynamically. It is also possible to
           reset the standard performance measures for the Client (and
           individually for each traffic pattern). Three new methods:
           suspend, resume, and resetSPF have been added to Traffic and
           Client.

 05/07/92  I suddenly discovered that '-b 0' didn't work with g++, version
           1.39.0 on a sparc, due to a malicious bug in the compiler. It
           seems to be fine, though, with other compilers and also with
           1.39.0 on a Sun 3/80. I don't have an older version of g++, but
           I am pretty sure that everything was fine with 1.37. Anyway,
           I am not going to try to bypass the problem (a few simple
           attempts didn't work) as: 1. '-b 0' is not really useful on a sparc,
           2. g++ 1.39.0 is now (almost) completely obsolete (although I am
           still using it for sentimental reasons :-).

 05/07/92  As a byproduct of the above research, I also discovered that
           '-b 0 -i' didn't work (bus error) with g++ version 2.2.2 on a sparc.
           The same problem showed up when BIG counters were used with
           user-defined random variables (and '-b 0' was selected). The problem
           has been fixed by aligning the counter within the RVariable
           structure to a doubleword boundary (although I am not sure that I
           understand why). Note: the alignment is only forced for g++
           (version >= 2) and when '-b 0' is selected.

 05/07/92  Note: be careful with '-f' (optimization) if you are using g++
           version 2.2.2. It seems to generate wrong code sometimes. It did
           it once for me with '-b 0 -f'. The moral: don't use '-b 0' on a
           sparc. Is there a machine on which it is useful?

 08/07/92  New mks option '-p' added. With this option, wait requests accept
           an optional third parameter (long int) called the order of the
           wait request. If multiple events occur within the same ITU, the
           wait request with the lowest order is used to restart the process.
           With '-p', the implied priorities of port events are not obeyed:
           the user has better control over these priorities and no tricks
           are necessary.

 01/08/92  Fixed a bug in SMPP causing errors in declarations as:
                        traffic Trf (Mes, Pack);
           i.e., with implicit empty body.

 27/09/92  Fixed a memory leak in observer.c. Previously, SMURPH failed to
           deallocate memory used to represent observer timeout requests.
	   Fixed a problem causing that RVariables owned by observers were
           not accessible by DSD.
           Fixed a bug causing compilation errors when '-p' and '-b 1'
           were used together.
           Port to Macintosh completed.

 08/10/92  Fixed a minor problem in SMPP causing (harmless) compiler warnings
           for user-defined exposure methods. This problem only occurred with
           g++ version 2.

 27/10/92  Full exposure of RVariable includes now relative confidence
           intervals at 95% and 99% (the screen exposure only includes the
           95% interval). The relative interval is the ratio of half the
           length of the confidence interval to the absolute value of the
           mean.

 03/11/92  I had to shuffle around some 'friend inline' declarations is 
           global.h to make SMURPH work with g++ version 2.3.1. 2.3.1 seems
           to be garbage, even worse than its predecessor.

 03/11/92  Fixed the problem with '-b 1 -p'. Previously, SMURPH didn't
           compile with this configuration of options.

 15/11/92  It is now possible to declare a fault rate for a link. Packets
           transmitted on such a link will be randomly "damaged" with the
           specified rate (per bit). New option '-z' added to mks. Faulty
           links are only available if the simulator has been created with this
           option.

 18/11/92  "Fixed" distribution type added to Traffic. Previously the fixed
           distribution types for message inter-arival time, message length,
           burst inter-arrival time and size was simulated by uniform
           distribution type with the minimum equal to the maximum.

 24/11/92  The configuration of counters associated with Link rearranged
           slightly. Link exposure (1) modified accordingly.

 02/01/93  DSD now interprets one segment attribute, namely, the display
           mode. Previously DSD ignored all attributes and the only display
           mode was "isolated points".

 20/01/93  Fixed a bug in operator>> (stream, BIG) causing that spaces were
           read as BIG zeros.

 03/02/93  Fixed a bug in setFaultRate (link.c) causing that occasionally
           the link fault rate setting was ignored.

 05/02/93  Fixed a minor bug in mks causing that in some obscure circumstances
           '-q' was ignored.

 09/02/93  Fixed a bug in SMPP causing compilation errors when a process
           type derived from another user-defined process type  was state-less.

 11/02/93  Added a new port method p1->distTo (p2) giving the distance from
           p1 to p2.

 16/02/93  The Packet's method fill now sets the PF_full flag of the packet.

 18/02/93  Fixed a bug in SMPP causing compilation errors for null-body
           declarations with the AT&T compiler.

 08/03/93  If the simulator has been created with '-g' (debugging), each
           packet carries one additional long attribute called 'Signature'.
           For packets acquired from the Client (via 'getPacket'), this
           attribute is set to subsequent nonnegative numbers starting
           from zero which are different for different packets. Intentionally,
           signatures are to be used for packet tracing, e.g., by observers.

 12/06/93  It is now possible to change the transmission rate of a port by
           calling p->setTRate (r), or the transmission rate of all ports
           connected to a given link by calling lk->setTRate (r).

 16/07/93  SMURPH now runs under Linux. Version number changed to 1.71

 28/07/93  SMURPH now runs on DEC-stations under Ultrix.

 25/09/93  Patched to work with SUNPRO CC version 2.01.

 18/11/93  Ported to DEC Alpha under OSF (g++ version 2.5.3). Version number
           changed to 1.80. Added examples from the book.

 16/12/93  Ported to SPARC SOLARIS SVR4

 23/01/94  Fixed a problem in DSD resulting in integer overflow when
           DSD was created with type LONG of size 4 bytes and the
           size of LONG for the simulator was 8 bytes (e.g., long long).
           This caused some time values to be displayed incorrectly.

 18/02/94  Dummy constructors added to circumvent a bug in g++.2.5.8. These
           constructors don't seem to be doing any harm so they may stay
           even after the bug is fixed (if ever). They can be reverted
           by unsetting the ZZ_DCO symbol in version.h.

 18/02/94  Two new client events added: SUSPEND and RESUME. The semantics
           of suspending and resuming traffic patterns (and the Client)
           changed. Now it is possible to suspend and resume user-defined
           pilot processes in a reasonable way.

 21/02/94  One more client event added: RESET. The event is triggered on
           a traffic pattern each time resetSPF is executed on that traffic
           pattern.

 18/03/94  Protocol tracing can now be restricted to a single station 
           whose Id is specified with '-t' (after the time specification).
           Version number changed to 1.81.

 31/07/94  Some adjustments required to make SMURPH run on SGI under
           IRIX Release 5.2. Note: at this moment SMURPH only runs with CC
           (CFront compiler). The gnu compiler seems to have some serious
           problems under IRIX 5.2.

 31/07/94  Fixed mks and smpp to avoid linker warnings on some machines/systems
           resulting from multiple symbol definitions. Upgrade to version 1.82.

 10/11/94  Message length generated by the Client (method genMLE) is now
           rounded up or down to the nearest multiple of 8, not always rounded
           up as before.

 15/11/94  The name of the process exposure method printWai changed to
           "printWait"; the name of the station exposure method printPrt
           changed to "printPort".

 26/11/94  Fixed to run with gcc-2.6.2. This seems to be getting out of hand.
           With every new release of gcc there is a new bug in the compiler.

 27/12/94  Changed the output format in print for floating point numbers.

 07/01/95  Fixed the problem with "identify". Some compilers/linkers treat
           externals as commons while some others don't. SMURPH now correctly
           figures out whether a protocol Id (see identify) has been assigned
           and if not, it uses a default Id ("unidentified protocol").

 02/05/95  Fixed a bug in display.c causing that the -d option of the simulator
           didn't work on systems without asynchronous i/o.

 28/06/95  Ported to cxx under OSF/1 V3.0 on Alpha. Checkpointing doesn't work
           yet, although it used to under older versions of OSF.

 20/11/96  SMURPH turned into SIDE, version number changed to 2.0. When mks
           is called with -R, it creates a real-time version of SMURPH that
           can be used to control a real reactive environment. The interface
           is provided by binding mailboxes to TCP/IP sockets. Slight change
           in the semantics of a capacity-0 mailbox: put always results in
           inItem getting called. A new event, CLEAR, is triggered by a
           process when its signal repository is emptied.

 15/04/97  DSD rewritten in Java. Option -D added to mks. With that option
           the processing of events scheduled to occur at the same time is
           deterministic.

 01/06/97  File names revised to make sure that they comply with old DOS
           limitations -- in case SMURPH/SIDE is ported to Windows NT. The
           GNU C++ environment under NT doesn't recognize long file names.
           Sorry about that.

 11/06/97  Checkpointing code disabled. Checkpointing ceased to work and some
           effort will be needed to bring it back. Perhaps I should rethink
           the whole idea and either give it up completely, or come up with
           a better and more general solution.

 22/06/97  Fixed a few problems with DSD. I still have no clue why DSD is
           ten times slower under Windows than under Linux.

 26/09/97  Fixed a minor problem in display.cc causing that objects owned
           by the Root process were only displayed with -s. Fixed a problem
           with the html version of the manual which caused that the
           manual couldn't be displayed with Netscape Communicator.

 27/10/97  Fixed a minor bug in iolink.c causing memory alignment problems
           on some systems.

 28/10/97  SMURPH's malloc now works on Linux. The improvement in execution
           speed: 10-15%.

 26/11/97  Mailboxes can also be bound to devices. Upgrade to version 2.10.

 20/12/97  Mailboxes can be journalled.

 05/02/98  Fixed a bug in main.cc causing the process's state wait
           request not to work properly.

 18/02/98  MASTER server mailboxes implemented.

 21/02/98  Fixed a problem with PLink causing that the number of distance
           entries needed to specify the distance matrix wasn't always
           the minimum possible.

 22/02/98  Fixed a bug in SMPP causing problems with destructor declarations.

 30/12/98  Fixed a few problems under RedHat 5.2 (using glibc).

 12/06/99  Fixed scheduler problems causing time lag when many events were
           scheduled at the same time.

 04/07/99  Added getDelay and setDelay to Timer AI. Added STALL to the list
           of events that can be awaited on Kernel. STALL is triggered when
           the simulator runs out of events.

 25/07/99  Option -F (avoiding FP operations and forcing -S, -D, -R) added.
           Fixed a bug causing improper timing of events (RT mode only)
           after long I/O waits.

 20/08/99  It is now possible to specify the hostname in socket->connect
           as "xxx.xxx.xxx.xxx" - to indicate a direct IP number.

 29/08/99  SIDE fixed to work with gcc version 2.95.

 17/10/00  Added 'clone' as a packet method.

 25/01/02  Miscellaneous (and rather insignificant) fixes related to the new
           version of GCC (3.0.2). Fixed a problem (in smpp) with the setup
           method of Root.

 11/02/02  Fixed to work under CYGWIN (DSD tested under jdk-1.4.x)

 20/11/03  Fixed to work with gcc 3.3.1. The only platform recommended for this
           version of SIDE is GNU (Linux + CYGWIN)

 11/03/06  Radio channels added. Version 2.5.

           ---- New method of tagging for CVS ----

 R060329B  Interfrence thresholds: INTLOW/INTHIGH added for Transceivers.

 R060430A  Lots of changes. Now we have something close to a decent version.
           DSD tested with the new templates. So far, so good. Manual updated.

 R060430B  Final fix of a few irrelevancies in REALTIME examples.

 R060709A  Radio example redone and cleaned up a bit.

 R060803A  Changed zz_wait to wait in all AIs, such that it is now legal to
           invoke wait from methods (not only directly from the process code)
           TheSignal and TheSender switched to be compatible with TheItem and
           TheMailbox for mailboxes.
           Barrier mailboxes introduced. The mailbox AI is getting messy a bit,
           so I had to significantly redo the corresponding sections from the
           manual to improve their legibility.
           TheBarrier points to Info01 (as an int).

 R060806A  Symbols __SIDE__ and __SMUPRH__ are now defined for every program
           compiled and set to the version number (a string).
	   Added pfmPDE/pfmMDE to detect packet/message deallocation.
	   Destructing link activities made subtler to account for the fact that
	   some activities are jams and, as such, should not have their "packet"
	   destructors invoked upon deallocation.

 R060809A  Introduced a variant of sleep (automatically selected by smpp) to be
	   used in functions called from processes. This one uses longjmp
	   instead of straighforward 'return'.
	   Visualization mode added whereby the simulator tries to fit the
	   advancement of virtual time to real time. Hey, we are beginning to
	   arrive at something decent, aren't we?

 R060811A  Added a virtual destructor to Process to make it possible to have
	   subtype destructors - to clean up when processes are deallocated.
	   Added a new AI: Monitor implementing simple global numerical
	   signals a la PicOS.
	   
 R060830B  Added void delay (double etus, int state) to Timer. This is a
	   shortcut for wait (etuToItu (etus), state).
	   Added a 'touch' to mks to eliminate Cygwin's make complaints about
	   clock skew.
	   Added an XML parser implementing an alternative interface for
	   reading input data (and possibly writing output results).
	   Exception/assert functions redone to accept variable number of
	   arguments, i.e., format + output items.
	   Added parseNumbers to numio.cc.
	   Constants SOL_VACUUM, SOL_COAX, SOL_COPPER, SOL_FIBER
	   MemoryUsed finally eliminated. It was a relic from the days when
	   SMURPH had its own memory allocator.
	   Added setTolerance for Station, getTolerance (global + method)
	   RFPing example working.

 ==============================================================================

 R061014A  First VUEE tree. At this point we discontinue the previous SIDE CVS
	   repository (on sheerness) and start a new one on the Olsonet server.
	   I am archiving the sheerness repository, and the older versions of
	   SIDE will no longer be retrievable directly.

 R061021A  When a process is killed, and its descendants list is nonempty, the
	   descendants are moved to its parent process. Previously, it was
	   illegal to kill a process with a non-empty list of decendants.
	   A new event added to the Process AI: CHILD. This event is triggered
	   whenever a child of the process terminates. Info01 and Info02 are
	   then set to their values in the children, as for DEATH.

	   There are no more zombies, i.e., processes that have been killed
	   while somebody is waiting for them. Wait request that used to be
	   queued at such processes are now moved to a special list.

           Added terminate () as a Station method, which terminates all
	   processes run (owned) by the station. This is intended as a prereq
	   for reset.

	   Added Boolean transmitting () to Port and Transceiver. The method
	   returns YES, if the Port/Transmitter is currently emitting an
	   activity.

	   Note: manual still has to be updated.

 R061022A  Improved deallocation of killed processes. Previously if a process
	   wanted to take advantage of its destructor to deallocate its
	   descendants, the outcome was different depending on whether the
	   process was terminated "normally" (by regular terminate) or by
	   reset (Station::terminate). In the latter case, the non-process
	   descendants were deallocated by the system before the process's
	   desctructor was able to run, which might cause errors. Now, the
	   forced deallocation of descendants takes place after the process
	   subtype desctructor has run, which should be always safe.

 R061030A  Fixed lots of nonsense to make this thing compile (and hopefully
	   run) under Linux, including x86_64. 

 R061121A  Fixed a bug in double IHist::max (double, double) detected by Nick.

 R061122A  Two more bugs found by Nick. Good work!!

 R061125A  Corrections to the manual by Nick.
