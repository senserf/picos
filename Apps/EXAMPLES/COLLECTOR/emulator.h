#ifndef	__pg_emulator_h
#define	__pg_emulator_h

#include "sysio.h"
#include "tag.h"

#if defined(__SMURPH__) && defined(EMULATE_SENSOR)

// If compiled for virtual execution, the sensor is emulated

//+++ emulator.cc
void rds (word, address);

#else

// The real sensor (or straightforward VUEE model)
#define	rds(a,b)	read_sensor (a, SENSOR_MPU9250, b)

#endif

#endif
