Quickstart:
===========

1. The recommended platform is Linux or Cygwin. Make sure you have gcc/g++
   (version >= 3.3.1) and stuff (like make, etc.) installed. If something is
   missing, you will see. Please keep adding things as they are needed.

2. Make sure you have a directory named BIN or bin in your home directory
   and that it appears in your PATH.

3. cd to SOURCES and execute ./make_smurph

This will set things up with some (usually more than adequate) defaults.

To test:
========

1. cd to Examples/WIRELESS/Aloha

2. Execute mks (the program was put in BIN [or bin] during setup); the program
   should nicely compare without errors.

3. Execute ./side data.txt out.txt (the simulator should finish in a second or
   two).

4. Run diff out.txt output.txt (the files should be close, but not necessarily
   identical).
