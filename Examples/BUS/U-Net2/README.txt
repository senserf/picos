In this version of U-Net (see directory U-Net1), we use the variant of the
uniform traffic pattern which keeps track of the local message access
delays for each station (see files 'utraffil.h' and 'utraffil.cc' in
directory 'IncLib'). Everything is as in U-Net1, except for the
following two changes:

1. Files 'utraffil.h' and 'utraffil.cc' are included instead of files
   'utraffic.h' and 'utraffic.cc';

2. A non-standard traffic exposure (defined in 'utraffil.cc') is invoked
   by the root process after termination, to print out the local delays.
   
To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lmat.t' from
directory Templates as a user template file (dsd -u lmat.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'utraffil.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lmat.t lmat.r) and
then include the resultant file as a template resource file
(mks -rs lmat.r).

