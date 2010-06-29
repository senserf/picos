This is a skeleton of DQDB for a dual bus network. The traffic is uniform
(files 'utraff2l.h' and 'utraff2l.cc'); each station has two packet
buffers and two separate transmitters operating in parallel.

To create a simulator instance, execute 'mks' in this directory. If you
want to use DSD to monitor the protocol, specify file 'lmat.t' from
directory Templates as a user template file (dsd -u lmat.t). This way you
will be able to take advantage of the non-standard screen exposure for
the traffic pattern defined in 'utraff2l.cc'. On a Macintosh, you will
have to preprocess the template file by 'tplt' (tplt lmat.t lmat.r) and
then include the resultant file as a template resource file
(mks -rs lmat.r).

