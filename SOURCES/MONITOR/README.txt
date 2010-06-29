To compile the monitor by hand, execute

        make COMP=ccc PROG=ppp

where 'ccc' stands for the name of your C++ compiler (e.g., g++) and 'ppp'
stands for the monitor source file. You can select 'monsel.cc' for the UNIX
daemon version and 'monpol.cc' for the Windows single-process version

The package comes with two versions of the monitor: monsel.cc implemented as
a typical UNIX daemon that spawns a separate process for each non-trivial
service, and monpol.cc implemented as a single process emulating multiple
service threads.

By default, the first version is used under UNIX and the second under Windows.
There is no choice under Windows (owing to the lack of system calls like
fork and select, but it is possible to use the single-process version of the
monitor under UNIX.
