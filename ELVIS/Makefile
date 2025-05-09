# Makefile - configured for generic Unix
# $Id: Makefile.in,v 2.121 2003/10/21 02:31:00 steve Exp $
#
# Makefile.in should not be modified!  The "configure" script reads Makefile.in
# and writes a customized Makefile by editing the values of the following
# macros.  If the "configure" script doesn't support your version of UNIX,
# I suggest you do a plain "configure unix" and then hand-edit the resulting
# Makefile.  That will also create a "config.h" file which may require some
# tweaking.

################################################################################
# This is the name of the compiler, along with any flags that should be used
# both for compiling and for linking.  Typical values are "cc -g" or "cc -O".
# If you're compiling with X-windows support, you may need to add a
# "-I/usr/X11/include" flag or something similar.
#CC=gcc -g -Wall -pedantic -Wtraditional -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -I/usr/include/freetype2 -I/usr/X11R6/include
CC=gcc -O2

################################################################################
# This macro gives any arguments which will be needed during linking.
# Mostly, this means "-Llibdir" and "-llib" flags.  If you're compiling with
# X-windows support, then you'll have to add a "-lX11" and maybe a
# "-L/usr/X11/lib" flag or something similar.
LIBS= -lX11 -lcurses  -lresolv

################################################################################
# This should be "unix" for all UNIX variants.  It causes the compiler to use
# files from the osunix subdirectory, and do a UNIX-style installation.
OS=unix

################################################################################
# This is a command for compiling files in a subdirectory, and placing the
# resultant object files in the current directory.  Most UNIX compilers behave
# this way naturally, so you probably won't need to modify this.
OSCC=$(CC) -I. $(CFLAGS) $(FLAGC)

################################################################################
# This is the separator between a subdirectory name and a file name.  For UNIX
# this should always be a "/" character.
SLASH=/

################################################################################
# This is the file name extension for object files.  For UNIX, this is almost
# always ".o"
OBJ=.o

################################################################################
# This is the file name extension for executable files.  For UNIX, this is
# always a null string because UNIX doesn't use file name extensions to mark
# executable files.
EXE=

################################################################################
# This is the shell command which is used for deleting a file if it exists.
# The "-f" causes "rm" to be silent if the file doesn't exist.
RM=rm -f

################################################################################
# This is the compiler flag which causes compilation but not linking.  For
# most UNIX compilers, this is "-c".
FLAGC=-c

################################################################################
# This is the compiler flag for adding a directory to the list of directories
# to be searched for #include files.  For most UNIX compilers, this is "-I".
FLAGI=-I

################################################################################
# This is a list of programs to be created by "make all".  The possible list
# includes elvis$(EXE), ref$(EXE), ctags$(EXE), elvtags$(EXE), and fmt$(EXE).
# ctags and elvtags are the same program under different names.  The $(EXE) on
# the end is just in case this Makefile is reconfigured for an operating
# system which requires the EXE macro to be something like ".exe".
ALL=elvis$(EXE) ref$(EXE) elvtags$(EXE) elvfmt$(EXE)

################################################################################
# The installation directories are relative to this
PREFIX=/usr

################################################################################
# This is the directory where "make install" will place the executable programs
BINDIR=$(PREFIX)/bin

################################################################################
# This is the directory where "make install" will place elvis' support files,
# such as the help text and the default initialization scripts.  If you change
# the value here, then you also need to change it in "config.h"
DATADIR=$(PREFIX)/share/elvis/
DOCDIR=$(PREFIX)/share/elvis/doc/

################################################################################
# This is the shell command for copying one file to another
CP=cp

###############################################################################
###############################################################################
###############################################################################
###########                                                         ###########
###########                         S T O P !                       ###########
###########                                                         ###########
###########  You shouldn't need to modify anything below this line  ###########
###########                                                         ###########
###############################################################################
###############################################################################
###############################################################################

###############################################################################
# Portable compilation rules

VERSION=2.2_0
MISC=	BUGS README.html COPYING INSTALL Makefile.in configure instman.sh\
	elvis.lsm makwin32.bat makmsdos.bat makos2.cmd e2 \
	elvis.spec.in Announce-$(VERSION)
OSSRCS=	os$(OS)$(SLASH)osblock.c os$(OS)$(SLASH)osdir.c\
	os$(OS)$(SLASH)osprg.c os$(OS)$(SLASH)ostext.c\
	os$(OS)$(SLASH)osnet.c os$(OS)$(SLASH)tcaphelp.c
OSOBJS=	osblock$(OBJ) osdir$(OBJ) osprg$(OBJ) ostext$(OBJ) osnet$(OBJ)
OSOBJS2=tcaphelp$(OBJ)
OSHDRS=	os$(OS)$(SLASH)osdef.h
XBM=	guix11/elvis.xbm guix11/elvispin.xbm guix11/gray.xbm
X11SRCS=guix11/guix11.c guix11/xclip.c guix11/xevent.c guix11/xmisc.c\
	guix11/xscroll.c guix11/xstatus.c guix11/xtext.c guix11/xtool.c \
	guix11/xdialog.c
X11OBJS=guix11$(OBJ) xclip$(OBJ) xevent$(OBJ) xmisc$(OBJ) xscroll$(OBJ)\
	xstatus$(OBJ) xtext$(OBJ) xtool$(OBJ) xdialog$(OBJ)
X11HDRS=guix11/guix11.h guix11/xclip.h guix11/xdialog.h guix11/xevent.h\
	guix11/xmisc.h guix11/xscroll.h guix11/xstatus.h guix11/xtext.h\
	guix11/xtool.h

GNOMESRCS=
GNOMEOBJS=
GHOMEHDRS=

GUISRCS=$(GNOMESRCS) $(X11SRCS) guicurs.c guitcap.c guiopen.c
GUIOBJS=$(GNOMEOBJS) $(X11OBJS) guicurs$(OBJ) guitcap$(OBJ) guiopen$(OBJ)
GUIHDRS=$(GNOMEHDRS) $(X11HDRS)
LPSRCS=	lpescape.c lpovrtyp.c lpps.c
LPOBJS=	lpescape$(OBJ) lpovrtyp$(OBJ) lpps$(OBJ)
HDRS=	$(OSHDRS) $(GUIHDRS) autocmd.h buffer.h buffer2.h calc.h color.h\
	cut.h descr.h digraph.h display.h display2.h draw.h draw2.h elvis.h\
	elvisio.h event.h ex.h gui.h gui2.h input.h lowbuf.h lp.h map.h\
	mark.h message.h message2.h misc.h more.h move.h need.h operator.h\
	opsys.h optglob.h options.h regexp.h region.h safe.h scan.h session.h\
	spell.h state.h state2.h tag.h tagsrch.h tagelvis.h version.h vi.h\
	vicmd.h window.h config.h
SRCS=	$(OSSRCS) $(GUISRCS) $(LPSRCS) autocmd.c buffer.c calc.c color.c cut.c\
	descr.c digraph.c display.c dmhex.c dmmarkup.c dmnormal.c dmsyntax.c\
	draw.c event.c ex.c exaction.c exconfig.c exedit.c exmake.c exsubst.c\
	fold.c ftp.c gui.c http.c input.c io.c lowbuf.c lp.c main.c map.c\
	mark.c message.c misc.c more.c move.c need.c operator.c optglob.c\
	options.c regexp.c region.c regsub.c safe.c scan.c search.c session.c\
	spell.c state.c tag.c tagsrch.c tagelvis.c url.c vi.c vicmd.c window.c
OBJ1=	$(OSOBJS) optglob$(OBJ) options$(OBJ) safe$(OBJ) session$(OBJ)
OBJ2=	buffer$(OBJ) calc$(OBJ) color$(OBJ) descr$(OBJ) digraph$(OBJ)\
	display$(OBJ) gui$(OBJ) lowbuf$(OBJ) mark$(OBJ) misc$(OBJ) io$(OBJ)\
	dmhex$(OBJ) dmmarkup$(OBJ) dmnormal$(OBJ) dmsyntax$(OBJ) scan$(OBJ)
OBJ3=	$(OSOBJS2) autocmd$(OBJ) cut$(OBJ) draw$(OBJ) event$(OBJ) ex$(OBJ)\
	exaction$(OBJ) exconfig$(OBJ) exedit$(OBJ) exmake$(OBJ) exsubst$(OBJ)\
	fold$(OBJ) ftp$(OBJ) http$(OBJ) input$(OBJ) lp$(OBJ) map$(OBJ)\
	message$(OBJ) move$(OBJ) more$(OBJ) need$(OBJ) operator$(OBJ)\
	regexp$(OBJ) region$(OBJ) regsub$(OBJ) search$(OBJ) spell$(OBJ)\
	state$(OBJ) tinytcap$(OBJ) tag$(OBJ) tagsrch$(OBJ) tagelvis$(OBJ)\
	url$(OBJ) vi$(OBJ) vicmd$(OBJ) window$(OBJ)
OBJS=	$(OBJ1) $(OBJ2) $(OBJ3)
REFOBJS=ref$(OBJ) tag$(OBJ) tagsrch$(OBJ) safe$(OBJ)
CTOBJS=	ctags$(OBJ) tag$(OBJ) safe$(OBJ)
CFLAGS=	$(FLAGI)os$(OS)
EVERY=	$(ALL) verify$(EXE) elvdump$(EXE) calc$(EXE)
SHELL=	/bin/sh

DISTRIB=elvis-$(VERSION)
DOSEXE=exedos
WIN32EXE=exewin32
OS2EXE=exeos2

all: $(ALL)

every: $(EVERY)

elvis$(EXE): $(HDRS) main$(OBJ) $(OBJS) $(GUIOBJS) $(LPOBJS)
	$(CC) $(CFLAGS) main$(OBJ) $(OBJS) $(GUIOBJS) $(LPOBJS) $(LIBS) -o elvis$(EXE)

ctags$(EXE): $(CTOBJS)
	$(CC) $(CFLAGS) $(CTOBJS) -o ctags$(EXE)

doc/elvtags.man: doc/ctags.man
	sed 's/\<CTAGS\>/ELVTAGS/g; s/ctags\>/elvtags/g; s/ant" elvtags/ant" ctags/' doc/ctags.man >doc/elvtags.man

elvtags$(EXE): ctags$(EXE)
	$(CP) ctags$(EXE) elvtags$(EXE)

elvfmt$(EXE) doc/elvfmt.man: fmt$(EXE) doc/fmt.man
	$(CP) fmt$(EXE) elvfmt$(EXE)
	sed 's/\<FMT\>/ELVFMT/g; s/\<fmt\>/elvfmt/g' doc/fmt.man >doc/elvfmt.man

ref$(EXE): $(REFOBJS)
	$(CC) $(CFLAGS) $(REFOBJS) -o ref$(EXE)

fmt$(EXE): fmt.c os$(OS)$(SLASH)osdir.c
	$(CC) $(CFLAGS) fmt.c -o fmt$(EXE)

verify$(EXE): $(HDRS) verify$(OBJ) $(OBJ1) $(OBJ2)
	$(CC) $(CFLAGS) verify$(OBJ) $(OBJ1) $(OBJ2) -o verify$(EXE)

elvdump$(EXE): $(HDRS) elvdump$(OBJ) $(OBJ1)
	$(CC) $(CFLAGS) elvdump$(OBJ) $(OBJ1) -o elvdump$(EXE)

calc: $(HDRS) calc.c
	$(CC) $(CFLAGS) -DTRY calc.c -o calc

###############################################################################
# Rules for os-dependent files

osblock$(OBJ): os$(OS)$(SLASH)osblock.c
	$(OSCC) os$(OS)$(SLASH)osblock.c

osdir$(OBJ): os$(OS)$(SLASH)osdir.c
	$(OSCC) os$(OS)$(SLASH)osdir.c

osprg$(OBJ): os$(OS)$(SLASH)osprg.c
	$(OSCC) os$(OS)$(SLASH)osprg.c

ostext$(OBJ): os$(OS)$(SLASH)ostext.c
	$(OSCC) os$(OS)$(SLASH)ostext.c

osnet$(OBJ): os$(OS)$(SLASH)osnet.c
	$(OSCC) os$(OS)$(SLASH)osnet.c

tcaphelp$(OBJ): os$(OS)$(SLASH)tcaphelp.c
	$(OSCC) os$(OS)$(SLASH)tcaphelp.c

###############################################################################
# Rules for GUI-dependent files.  Note that we can't really generalize here
# as well as we could for OS-dependent files, because different GUIs may be
# broken down differently.

guix11$(OBJ): guix11/guix11.c $(XBM)
	$(OSCC) guix11/guix11.c

xclip$(OBJ): guix11/xclip.c
	$(OSCC) guix11/xclip.c

xevent$(OBJ): guix11/xevent.c
	$(OSCC) guix11/xevent.c

xmisc$(OBJ): guix11/xmisc.c
	$(OSCC) guix11/xmisc.c

xscroll$(OBJ): guix11/xscroll.c
	$(OSCC) guix11/xscroll.c

xstatus$(OBJ): guix11/xstatus.c
	$(OSCC) guix11/xstatus.c

xtext$(OBJ): guix11/xtext.c
	$(OSCC) guix11/xtext.c

xtool$(OBJ): guix11/xtool.c
	$(OSCC) guix11/xtool.c

xdialog$(OBJ): guix11/xdialog.c
	$(OSCC) guix11/xdialog.c

guignome$(OBJ): guignome/guignome.c
	$(OSCC) guignome/guignome.c

guignome/elvisCorba-common.c: guignome/elvisCorba.idl
	(cd guignome; orbit-idl elvisCorba.idl)

guignome/elvisCorba-skels.c: guignome/elvisCorba-common.c
guignome/elvisCorba-stubs.c: guignome/elvisCorba-common.c
guignome/elvisCorba.h: guignome/elvisCorba-common.c

###############################################################################
# Handy commands

clean: clean.$(OS)
	$(RM) $(ALL)

clean.unix:
	$(RM) *$(OBJ)
	$(RM) guignome/*$(OBJ) guignome/elvisCorba-* guignome/elvisCorba.h
	$(RM) core
	$(RM) errlist
	$(RM) verify.elv
	$(RM) gdk_imlib.h
	$(RM) $(DISTRIB).tar.gz
	$(RM) doc/elvtags.man
	$(RM) elvis.rpm
	$(RM) ctags

clean.msdos:
	$(RM) *$(OBJ)

clobber: clean
	$(RM) $(EVERY)
	$(RM) tags
	$(RM) detail
	$(RM) .elv*
	$(RM) foo
	$(RM) stderr

tmp:
	$(RM) /var/tmp/elvis*.ses
	$(RM) core

sure: verify
	$(RM) verify.elv
	verify >detail || gdb verify core

wc: $(SRCS) $(HDRS)
	wc $(SRCS) $(HDRS) | sort -n

tags: $(SRCS) $(HDRS)
	elvtags $(SRCS) $(HDRS)

ciall: $(SRCS) $(HDRS)
	ci -u$(REV) Makefile $(SRCS) $(HDRS)

cinever:
	@for i in *.[ch]; do [ -f RCS/$$i,v ] || echo $$i; done
	@for i in os*/.; do (cd $$i; for j in *.[ch]; do [ -f $$j,v -o -f RCS/$$j,v ] || echo `dirname $$i`/$$j; done); done
	@for i in gui*/.; do (cd $$i; for j in *.[ch]; do [ -f $$j,v -o -f RCS/$$j,v ] || echo `dirname $$i`/$$j; done); done

tarball:
	rm -f $(DISTRIB).tar.gz
	make $(DISTRIB).tar.gz

$(DISTRIB).tar.gz:
	rm -rf $(DISTRIB)
	mkdir $(DISTRIB)
	for i in os*; do if [ -d $$i ]; then mkdir $(DISTRIB)/$$i; fi; done
	for i in gui*; do if [ -d $$i ]; then mkdir $(DISTRIB)/$$i; fi; done
	cp *.c *.h $(MISC) $(DISTRIB)
	rm -f $(DISTRIB)/sample.c
	mkdir $(DISTRIB)/data
	cp data/*.* $(DISTRIB)/data/
	for i in icons scripts stubs themes; do mkdir $(DISTRIB)/data/$$i; cp data/$$i/* $(DISTRIB)/data/$$i/; done
	mkdir $(DISTRIB)/doc
	cp doc/*.* $(DISTRIB)/doc
	chmod +w $(DISTRIB)/doc/*
	for i in doc/*.html; do sed 's/<title>Elvis[^ ]*/<title>Elvis-$(VERSION)/;s/ELVIS[^ <]*</ELVIS-$(VERSION)</' $$i >$(DISTRIB)/$$i; done
	for i in doc/*.man; do sed 's/Elvis 2\.[0-9x][0-9a-z-]*/Elvis $(VERSION)/' $$i >$(DISTRIB)/$$i; done
	for i in os*/*.c os*/*.h os*/*.mak os*/*.lib os*/*.ICO osos2/*.def osos2/*os2*; do cp $$i $(DISTRIB)/$$i; done
	for i in gui*/*; do if [ -f $$i ]; then cp $$i $(DISTRIB)/$$i; fi; done
	rm -f $(DISTRIB)/osunix/osconfig.h
	rm -f $(DISTRIB)/config.h
	rm -f $(DISTRIB)/Makefile
	rm -f $(DISTRIB)/doc/elvtags.man
	rm -f $(DISTRIB)/doc/elvfmt.man
	find $(DISTRIB) -type f -exec chmod +w {} \;
	tar czf $(DISTRIB).tar.gz $(DISTRIB)
	rm -rf $(DISTRIB)

$(DISTRIB)-win32.tar.gz: $(WIN32EXE)
	rm -rf bin-win32
	mkdir bin-win32
	cp $(WIN32EXE)/*.[eE][xX][eE] bin-win32
	sed 's/$$//' <README.html >bin-win32/README.html
	sed 's/$$//' <BUGS >bin-win32/BUGS
	sed 's/$$//' <COPYING >bin-win32/COPYING
	cp --recursive data bin-win32
	cp --recursive doc bin-win32
	chmod a+rw bin-win32/* bin-win32/*/* bin-win32/data/*/*
	rm -rf bin-win32/data/RCS
	rm -rf bin-win32/doc/RCS
	showrgb | sed 's/ *\([0-9]\+\) *\([0-9]\+\) *\([0-9]\+\)[ 	]\+\(.*\)/\1	\2	\3	\4/; s/ //g' | tr '[A-Z]' '[a-z]' | sort -u +3 >bin-win32/data/rgb.txt
	for i in doc/*.html; do sed 's/<title>Elvis[^ ]*/<title>Elvis-$(VERSION)/;s/ELVIS[^ <]*</ELVIS-$(VERSION)</' $$i >bin-win32/$$i; done
	for i in doc/*.man; do sed 's/Elvis 2\.[0-9x][0-9a-z-]*/Elvis $(VERSION)/' $$i >bin-win32/$$i; done
	sed 's/$$//' <doc/printdoc.bat >bin-win32/doc/printdoc.bat
	sed 's/$$//' <doc/license.html >bin-win32/doc/license.html
	sed 's/$$//' </usr/share/dict/words >bin-win32/data/words
	(cd bin-win32/data/icons; xpm2ico *.xpm)
	(cd bin-win32; tar czf ../$(DISTRIB)-win32.tar.gz *)

$(DISTRIB)-msdos.tar.gz: $(DOSEXE)
	rm -rf bin-msdos
	mkdir bin-msdos
	cp $(DOSEXE)/*.[eE][xX][eE] bin-msdos
	sed 's/$$//' <README.html >bin-msdos/README.html
	sed 's/$$//' <BUGS >bin-msdos/BUGS
	sed 's/$$//' <COPYING >bin-msdos/COPYING
	cp --recursive data bin-msdos
	cp --recursive doc bin-msdos
	chmod a+rw bin-msdos/* bin-msdos/*/* bin-msdos/data/*/*
	rm -rf bin-msdos/data/RCS
	rm -rf bin-msdos/doc/RCS
	for i in doc/*.html; do sed 's/<title>Elvis[^ ]*/<title>Elvis-$(VERSION)/;s/ELVIS[^ <]*</ELVIS-$(VERSION)</' $$i >bin-msdos/$$i; done
	for i in doc/*.man; do sed 's/Elvis 2\.[0-9x][0-9a-z-]*/Elvis $(VERSION)/' $$i >bin-msdos/$$i; done
	sed 's/$$//' <doc/printdoc.bat >bin-msdos/doc/printdoc.bat
	sed 's/$$//' <doc/license.html >bin-msdos/doc/license.html
	(cd bin-msdos; tar czf ../$(DISTRIB)-msdos.tar.gz *)

$(DISTRIB)-os2.tar.gz: $(OS2EXE)
	rm -rf bin-os2
	mkdir bin-os2
	cp $(OS2EXE)/*.[eE][xX][eE] bin-os2
	sed 's/$$//' <README.html >bin-os2/README.html
	sed 's/$$//' <BUGS >bin-os2/BUGS
	sed 's/$$//' <COPYING >bin-os2/COPYING
	cp --recursive data bin-os2
	cp --recursive doc bin-os2
	for i in doc/*.html; do sed 's/<title>Elvis[^ ]*/<title>Elvis-$(VERSION)/;s/ELVIS[^ <]*</ELVIS-$(VERSION)</' $$i >bin-os2/$$i; done
	for i in doc/*.man; do sed 's/Elvis 2\.[0-9x][0-9a-z-]*/Elvis $(VERSION)/' $$i >bin-os2/$$i; done
	sed 's/$$//' <doc/printdoc.bat >bin-os2/doc/printdoc.bat
	sed 's/$$//' <doc/printdoc.bat >bin-os2/doc/printdoc.cmd
	sed 's/$$//' <doc/license.html >bin-os2/doc/license.html
	sed 's/$$//' </usr/dict/words >bin-os2/data/words
	(cd bin-win32/data/icons; xpm2ico *.xpm)
	cp osos2/elvis.INF bin-os2/data/
	cp osos2/README.os2 bin-os2/doc/
	chmod a-w bin-os2/data/* bin-os2/doc/*
	(cd bin-os2; tar czf ../$(DISTRIB)-os2.tar.gz *)

doc$(SLASH)elvistrs.msg: $(SRCS)
	sed -n '/%[cds]/d; s/\[[a-zA-Z]*\]//; s/\\\\/\\/g; s/.*msg(MSG_[A-Z]*, "\([^ 	"][^"]*\)".*/\1/p' *.c os*$(SLASH)*.c gui*$(SLASH)*.c | sort -u >doc$(SLASH)elvistrs.msg

###############################################################################

install: install.$(OS)

install.unix: $(ALL) doc/elvtags.man
	rm -f doc/*.bak
	[ -d $(BINDIR) ] || mkdir $(BINDIR)
	cp $(ALL) $(BINDIR)
	(cd $(BINDIR); chmod 0755 $(ALL))
	rm -rf $(DATADIR)
	mkdir -p $(DATADIR)
	cp data/*.* $(DATADIR)
	for i in icons scripts stubs themes; do mkdir $(DATADIR)/$$i; cp data/$$i/* $(DATADIR)/$$i/; done
	rm -rf $(DOCDIR)
	mkdir -p $(DOCDIR)
	cp doc/*.html $(DOCDIR)
	cp doc/*.man $(DOCDIR)
	cp doc/*.txt $(DOCDIR)
	cp doc/elvistrs.msg $(DOCDIR)
	cp doc/printdoc.bat $(DOCDIR)
	find $(DATADIR) -type f -exec chmod 0644 {} \;
	find $(DOCDIR) -type f -exec chmod 0644 {} \;
	(cd doc; for i in *.html; do sed 's/<title>Elvis[^ ]*/<title>Elvis-$(VERSION)/;s/ELVIS[^ <]*</ELVIS-$(VERSION)</' $$i >$(DOCDIR)/$$i; done)
	(cd doc; for i in *.man; do sed 's/Elvis 2\.[0-9x][0-9a-z-]*/Elvis $(VERSION)/' $$i >$(DOCDIR)/$$i; done)
	-(cd $(DATADIR); $(BINDIR)/elvtags -gitv stubs/*.c || ctags stubs/*.c)
	chmod 0755 $(DATADIR)/*/. $(DATADIR) $(DOCDIR)
	sh instman.sh -p$(PREFIX) $(ALL)
	-[ -d /etc/elvis ] || mkdir /etc/elvis
	-chmod 0755 /etc/elvis/
	-@ : >/etc/elvis/README && ( \
	  echo "This directory contains system-dependent versions of elvis' configuration" \
	  echo "files.  The standard versions are available in $(DATADIR)"  \
	  echo "To customize any of these configuration files for this system,"  \
	  echo "first copy the standard version of the file into /etc/elvis and" \
	  echo "then edit the copy.  Leave the standard version unchanged." \
	) >>/etc/elvis/README
	-chmod 0644 /etc/elvis/README
	@echo "This directory contains the standard versions of elvis' configuration" >$(DATADIR)/README
	@echo "files.  DO NOT MODIFY THESE FILES!  To perform host-specific customization," >>$(DATADIR)/README
	@echo "copy files from this directory into /etc/elvis/, and then modify the copies." >>$(DATADIR)/README
	@echo "To make user-specific changes, create a ~/.elvis/ directory (if it doesn't" >>$(DATADIR)/README
	@echo "exist already) and copy files from this directory into ~/.elvis/ and then" >>$(DATADIR)/README
	@echo "modify the copies.  Any file that elvis finds in ~/.elvis/ or /etc/elvis/" >>$(DATADIR)/README
	@echo "overrides the similarly named file in this directory." >>$(DATADIR)/README
	chmod 0644 $(DATADIR)/README


###############################################################################

uninstall: uninstall.$(OS)

uninstall.unix:
	(cd $(BINDIR); $(RM) $(ALL))
	rm -rf $(DATADIR)
	rm -rf $(DOCDIR)
	sh instman.sh -r -p$(PREFIX) $(ALL)

###############################################################################

elvis.rpm: elvis.spec.in $(DISTRIB).tar.gz
	@topdir=`rpm --showrc|sed -n 's/%{_usrsrc}/\/usr\/src/;s/^[-0-9]*: _topdir[ 	]//p'`; \
	 test -d $$topdir || topdir=/usr/src/redhat; \
	 test -d $$topdir || topdir=/usr/src/packages; \
	 echo topdir=$$topdir; \
	 MANDIR=`sh instman.sh -d -p$(PREFIX)`; \
	 if test -d $$topdir; \
	 then \
		 sed "s/VERSION/$(VERSION)/;s,MANDIR,$$MANDIR,g" elvis.spec.in >$$topdir/SPECS/elvis.spec; \
		 cp $(DISTRIB).tar.gz $$topdir/SOURCES; \
		 origdir=`pwd`; \
		 cd $$topdir; \
		 rpm -bb SPECS/elvis.spec && cp $$topdir/RPMS/i386/elvis-$(VERSION)-*.rpm $$origdir/elvis.rpm; \
	 else \
		 echo "Can't find rpm's topdir"; \
		 exit 2; \
	 fi
