/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __interface_h__
#define	__interface_h__

// Sensor/actuator identifiers
#define	THERMOCOUPLE	0
#define	OVEN		0

// Sensor range
#define	THERMOCOUPLE_MIN	0
#define	THERMOCOUPLE_MAX	1023

// ============================================================================
// Max setting of PW (pulse witdth). The idea is that a value between 0 and
// 1024, inclusively, determines the number of PicOS milliseconds per second
// that the oven is turned on.
// ============================================================================
#define	MAXPWI		1024

// Actuator range
#define	OVEN_MIN		0
#define	OVEN_MAX		MAXPWI

// ============================================================================
// The range of Kp, which we call "the span". The idea is that the span
// determines the proportional adjustable range of error. Any error above
// span translates into a full setting (1024) of the oven.
// ============================================================================
#define	SPAN_MIN		0	// on-off
#define	SPAN_DEFAULT		200
#define	SPAN_MAX		OVEN_MAX

// ============================================================================
// The remaining coefficients; the range is the same as for SPAN
// ============================================================================
#define	INTEGRATOR_GAIN_DEFAULT		10
#define	DIFFERENTIATOR_GAIN_DEFAULT	5

// ============================================================================
// Thermocouple calibration
// ============================================================================

#define	THERMOCOUPLE_CALIBRATION { 	\
			{  50,  21 },	\
			{ 175,	75 },	\
			{ 185,	80 },	\
			{ 195,	85 },	\
			{ 200,	87 },	\
			{ 210,	92 },	\
			{ 220,	97 },	\
			{ 230,	102 },	\
			{ 240,	107 },	\
			{ 250,	112 },	\
			{ 260,	116 },	\
			{ 270,	121 },	\
			{ 280,	126 },	\
			{ 290,	131 },	\
			{ 300,	136 },	\
			{ 320,	142 },	\
			{ 340,	151 },	\
			{ 360,	161 },	\
			{ 380,	170 },	\
			{ 400,	174 },	\
			{ 420,	188 },	\
			{ 440,	197 },	\
			{ 460,	207 },	\
			{ 480,	216 },	\
			{ 500,	225 },	\
			{ 520,	234 },	\
			{ 540,	243 },	\
			{ 560,	252 },	\
			{ 580,	262 },	\
			{ 600,	273 },	\
			{ 650,	294 },	\
			{ 700,	317 },	\
			{ 750,	340 },	\
			{ 800,	371 },	\
			{ 850,	379 },	\
			{ 900,	418 },	\
			{ 950,	422 },	\
			{ 980,	427 },	\
			{ 1000,	455 },	\
			{ 1050,	466 },	\
			{ 1100,	489 },	\
			{ 1150,	509 },	\
			{ 1200,	532 },	\
			{ 1250,	543 },	\
			{ 1300,	564 },	\
			{ 1320,	571 },	\
			{ 1400,	605 },	\
			{ 1430,	615 },	\
			{ 1500,	645 },	\
			{ 1550,	660 },	\
			{ 1600,	687 },	\
			{ 1690,	709 },	\
			{ 1700,	718 },	\
			{ 1760,	744 },	\
			{ 1800,	762 },	\
			{ 1840,	770 },	\
			{ 1900,	798 },	\
			{ 1940,	814 },	\
			{ 1990,	822 },	\
			{ 2000,	834 },	\
			{ 2040,	848 },	\
			{ 2060,	857 },	\
			{ 2100,	869 },	\
			{ 2130,	879 },	\
			{ 2170,	888 },	\
			{ 2200,	900 },	\
			{ 2250,	917 },	\
			{ 2270,	927 },	\
			{ 2300,	936 },	\
			{ 2330,	943 },	\
			{ 2350,	951 },	\
			{ 2380,	961 },	\
			{ 2400,	968 },	\
			{ 2450,	985 },	\
			{ 2490,	998 },	\
			{ 2500,	1002 },	\
			{ 2530,	1014 },	\
			{ 2550,	1021 },	\
			{ 2560,	1023 },	\
	}

// ============================================================================
// This is the default profile (taken from Gerry's document):
// duration, temp [in degs * 10, 1000 == 100 deg C]
// (the same default profile is imoplanted into the GUI program)
// ============================================================================
#define	DEFAULT_PROFILE	{		\
			{   0, 1500 },  \
			{ 101, 1830 },	\
			{  88, 2200 },	\
			{  37, 1830 },	\
			{  19, 1500 },	\
			{   0,  800 },	\
	}

#define	DEFAULT_PROFILE_N	6	// The number of points

// Maximum temperature in deci-degrees (for a profile entry)
#define	MAXTEMP		3000	// 300 degrees C

// Maximum duration of a segment, so we have some limit
#define	MAXSEGDUR	1000	// seconds

// Minimum and maximum number of entries
#define	MINPROFENTRIES	3
#define	MAXPROFENTRIES	24


#endif
