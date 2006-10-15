In this version of U-Net (see directories U-Net1 and U-Net2), we use the
'file server' traffic pattern (files 'fstrfl.cc' and 'fstrfl.h')
in 'IncLib'. Everything is as in U-Net2, except that files
'fstrfl.cc' and 'fstrfl.h' are included instead of 'utraffil.cc'
and 'utraffil.h'.

To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lrwt.t' from
directory Tmplts as a user template file (dsd -u lrwt.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'fstrfl.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lrwt.t lrwt.r) and
then include 'lrwt.r' as a template resource file (mks -rs lmat.r).

