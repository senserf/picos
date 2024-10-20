/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__pg_phys_cc3000_h
#define	__pg_phys_cc3000_h	1

// Security modes for WLAN connection
#define CC3000_WLAN_SEC_UNSEC 	0
#define CC3000_WLAN_SEC_WEP   	1
#define CC3000_WLAN_SEC_WPA   	2
#define CC3000_WLAN_SEC_WPA2  	3

// WLAN connection policies
#define	CC3000_POLICY_DIRECT	0		// Explicit connection
#define	CC3000_POLICY_PROFILE	1		// From stored profiles
#define	CC3000_POLICY_LAST	2
#define	CC3000_POLICY_ANY	3
#define	CC3000_POLICY_OLD	4		// Don't change
#define	CC3000_POLICY_DONT	5		// Autoconnect

// Driver states
#define	CC3000_STATE_DEAD	0		// Disabled, switched off
#define	CC3000_STATE_INIT	1		// Initializing
#define	CC3000_STATE_WBIN	2		// Waiting for buffer info
#define	CC3000_STATE_POLI	3		// Setting connection policy
#define	CC3000_STATE_RCON	4		// Connecting to AP
#define CC3000_STATE_APCN	5		// Connected to AP
#define	CC3000_STATE_SOKA	6		// Socket available
#define	CC3000_STATE_CSNG	7		// Closing server connection
#define	CC3000_STATE_ESTB	8		// Connection established
#define	CC3000_STATE_RECV	9		// Receive poll
#define	CC3000_STATE_READ	10		// Read received packet
#define	CC3000_STATE_SENT	11		// Packet sent

//+++ "phys_cc3000.c"

typedef struct {

	// Security mode is one of the above WLAN_SEC values; ssid_length
	// tells the initial portion of the string used for the SSID, the
	// remaining portion (bounded by the standard sentinel) is the key.

	byte	policy;
	byte	security_mode;
	byte	ssid_length;
	char	ssid_n_key [33];

} cc3000_wlan_params_t;

typedef struct {

	// Socket parameters to connect to; for now we assume that there is
	// only one socket in the game, make it stream for now; if it works
	// then we will probably add more options later.

	byte ip [4];
	word port;

} cc3000_server_params_t;

typedef struct {

	// PHY status info

	byte	dstate;
	byte	freebuffers;
	word	mkalcnt;
	word	dkalcnt;

} cc3000_phy_status_t;

// The third argument of phys_cc3000 is a flag:
#define	CC3000_FLAG_OFF		0x01		// The device is OFF

void phys_cc3000 (int, int, cc3000_server_params_t*, cc3000_wlan_params_t*);

#endif
