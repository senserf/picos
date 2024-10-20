#ifndef __pg_tag_h
#define	__pg_tag_h

#include "sysio.h"
#include "mpu9250.h"

// Accel is sensor number 1
#define	SENSOR_MPU9250			1

// Sensor configuration
#define	sensors_on()	mpu9250_on (MPU9250_SEN_ACCEL | \
				    MPU9250_LPF_20 | \
				    MPU9250_ACCEL_RANGE_2, 0)
#define sensors_off()	mpu9250_off ()

#define	MIN_SAMPLING_INTERVAL		256
#define	MAX_SAMPLING_INTERVAL		(16 * 1024)

#endif
