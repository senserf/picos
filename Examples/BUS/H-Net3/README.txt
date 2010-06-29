In this version of H-Net (see directory H-Net1), we use the variant of the
uniform traffic pattern which keeps track of the local message access
delays for each station (see files 'utraff2l.h' and 'utraff2l.cc' in
directory 'IncLib'). Everything is as in H-Net1, except for the
following two changes:

1. Files 'utraff2l.h' and 'utraff2l.cc' are included instead of files
   'utraffi2.h' and 'utraffi2.cc';

2. A non-standard traffic exposure (defined in 'utraff2l.cc') is invoked
   by the root process after termination, to print out the local delays.
   
To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lmat.t' from
directory Templates as a user template file (dsd -u lmat.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'utraff2l.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lmat.t lmat.r) and
then include the resultant file as a template resource file
(mks -rs lmat.r).

