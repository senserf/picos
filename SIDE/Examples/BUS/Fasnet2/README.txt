In this version of Fasnet, the transmitter and receiver are shared with
DQDB. The traffic pattern is uniform with message access time measured
locally for each station (files 'utraff2l.h' and 'utraff2l.cc').

To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lmat.t' from
directory Templates as a user template file (dsd -u lmat.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'utraff2l.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lmat.t lmat.r) and
then include the resultant file as a template resource file
(mks -rs lmat.r).

