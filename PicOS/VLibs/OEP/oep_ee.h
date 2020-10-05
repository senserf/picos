/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__oep_ee_h_pg
#define	__oep_ee_h_pg

//+++ "oep_ee.cc"

byte oep_ee_snd (word, byte, lword, lword);
byte oep_ee_rcv (lword, lword);
void oep_ee_cleanup ();

#endif
