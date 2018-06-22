Quick start and demo:

1. Install the modified version of elvis. On Cygwin, just execute ./instelvis
   in this directory. On Linux, become root first and then do the same.

   Note: on Linux, check if you have the elvis package installed. Use the
   Synaptic Package Manager to find out if you have a package named "elvis".
   Most likely you don't have it (it doesn't get installed by default), but if
   you do, remove it first. Our elvis is not installed as a package (perhaps
   some day the whole thing will become a package), but just "installed" as an
   executable program in /usr/bin with some files going into /usr/share/elvis/.

   Note: you can copy ELVIS/elvisrc (in this directory) to your HOME directory
   as .elvisrc. It will turn your elvis windows green on black (which I
   personally prefer to the standard black on gray). Needlees to say, you can
   play with the settings in that file to build your best personal
   configuration of colors.

2. Install PIP. Just execute ./instpip in this directory.

   Note: if you worry about the polution of your PC caused by the above
   installs, here is the complete list of things that get stored (so you can
   easily remove them by hand):

	- files elvis[.exe] and elvtags[.exe] are written to /usr/bin
	- subdirectory elvis gets written to /usr/share
	- script pip.tcl and link pip -> pip.tcl are put into your personal
	  bin (or BIN) directory

3. Make sure that you have the newest version of PICOS, VUEE, and SIDE
   installed (deployed), as usual.

4. On Cygwin, start Cygwin and the X server. PIP will not work without the 
   X server present.

5. We shall use new_il_demo for illustration. Start PIP by executing pip in an
   xterm window (under X). Note that, unlike piter, you cannot start PIP just
   by clicking on pip.tcl in your BIN directory, because, unlike piter, PIP
   needs Cygwin DLL and PATH for the lots of subtle execs that it issues to get
   its job done. Similar to piter, PIP needs Tcl8.5, which doesn't belong to
   Cygwin, which additionally complicates matters. Of course, all those
   problems are completely alien to Linux.

6. Click File->Open project and navigate to PICOS/VUEE/new_il_demo. Select that
   directory and click OK. Note that PIP only handles "new" projects, i.e.,
   ones that are compatible with picomp.

   Note: up to six last visted projects are available from the File menu via
   shortcuts. So the next time around you will not have to fish for the same
   project. The project selection dialog is somewhat more friendly on Linux.

   You will see that the tree view list in the left pane has been populated
   with files. You should be able to edit those files by clicking on them.

   Note: the behavior of elvis windows is more friendly on Linux, where those
   windows pop to the front properly in response to important events (like
   presenting a tag clicked in another window). For some reason, I couldn't
   achieve the same on Cygwin. I have noticed that the problem affects other
   applications as well, so it isn't just a bug in elvis.

7. The project hasn't been configured yet, so you cannot compile it, except 
   for VUEE (only the VUEE option is available in the Build menu). To configure
   the project, click Configuration->CPU+Board. In the configuration window
   that pops up check "Multiple" (because the praxis is for multiple boards).
   When you do that, the window will change its layout presenting you with a
   selection of boards for the three programs. Select the appropriate boards
   for all programs and click Done.

   You will see that Build offers now more options.

   Note: your configuration settings are saved in a file named config.prj in
   the project's directory, so they will be available when you visit the
   project again. Nothing overly harmful will happen when you delete this file,
   except that you will have to configure project-related things from scratch.

8. Try "Pre-build col ..." from the Build menu. This basically invokes mkmk
   for the "col" program. Wait until done (it will say so). You will see that
   the Build option for col in the Build menu is now enabled. Click it. This
   will invoke the compiler. There is no Image load function yet, but I will
   see what I can do.

   Note: mkmk and the compiler work dozens of times faster on Linux. This has
   nothing to do with PIP.

9. Configure VUEE. Click Configuration->VUEE and fill in the blanks. Do check
   the "Compile all functions as idiosyncratic" box, because the praxis needs
   that. Also check "Always run with udaemon". For the praxis data file,
   choose, e.g., mlakes_27_2.xml. For the udaemon geometry file use mlakes.geo
   (the only sensible choice available). Then click Done.

10.Click Build->VUEE. This will compile the VUEE model. A digression: note that
   you can Abort a compilation/execution in progress by clicking Abort in
   Build or Execute.

11.Now for the real show. Click Execute->Run VUEE.
