In this version of H-Net (see directory H-Net2), we use the uniform RPC
traffic pattern (see files 'rpctrfl.h' and 'rpctrfl.cc' in
directory 'IncLib'). Everything is as in H-Net2, except for the
following two changes:

1. Files 'rpctrfl.h' and 'rpctrfl.cc' are included instead of files
   'utraffic.h' and 'utraffic.cc';

2. A non-standard traffic exposure (defined in 'rpctrfl.cc') is invoked
   by the root process after termination, to print out the local delays.
   
To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lwrt.t' from
directory Templates as a user template file (dsd -u lwrt.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'rpctrfl.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lwrt.t lwrt.r) and
then include the resultant file (lwrt.r) as a template resource file
(mks -rs lwrt.r).

