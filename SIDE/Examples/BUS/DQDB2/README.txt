This is DQDB for a dual bus network with file server traffic
(files 'fstrfl.h' and 'fstrfl.cc'); each station has one packet
buffer and only one transmitter.

To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lrwt.t' from
directory Tmplts as a user template file (dsd -u lrwt.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'fstrfl.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lrwt.t lrwt.r) and
then include the resultant file as a template resource file
(mks -rs lrwt.r).

