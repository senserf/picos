#ifndef	__app_node_data_h__
#define	__app_node_data_h__
/*
 * Application-specific part of a Node
 */

int	sfd, last_snt, last_rcv, last_ack;
Boolean	XMTon, RCVon, rkillflag, tkillflag;

int rcv_start ();
int rcv_stop ();
int snd_start (int);
int snd_stop ();

#endif
