#ifndef	__model_h__
#define	__model_h__

#include "interface.h"
#include "plant.h"
#include "params.h"

#define	CONTROLLER_HOST	"localhost"
#define	VUEE_PORT	4443		// This is the default port
#define	THE_SENSOR	0
#define	THE_ACTUATOR	0

// Macro to make sure that a bounded value is actually between the bounds
#define	enforce_bounds(a,min,max)	do { \
						if ((a) < (min)) \
							(a) = (min); \
						else if ((a) > (max)) \
							(a) = (max); \
					} while (0)

// ============================================================================

station RArm : Controller {

	double  Force,		// The motor setting transformed into force
		MinForce,	// Minimum force to move the stationary arm
		Position,	// Current position from 0 to ARM_RANGE
		Speed,		// Current speed in m/s
		Acceleration,	// Current effective acceleration in m/s
		Mass,		// Total mass in kg
		DeltaT;		// Time elapsed since last update

	TIME	LastUpdateTime;

	inline void set_force () {
		// Calculates the effective force based on the actuator
		// setting
		int act;
		readActuator (act);
		enforce_bounds (act, ACTUATOR_MIN, ACTUATOR_MAX);
		// Convert to signed: negative = back, positive = forward,
		// zero = none
		act -= ACTUATOR_ZERO;
		if (Speed > 0.0) {
			// Moving right
			if (act <= 0) {
				// zero or opposite (acting left); apply braking
				// force to the left
				Force = -MAX_FORCE_B;
				return;
			}
		} else if (Speed < 0.0) {
			// Moving left
			if (act >= 0) {
				// zero or opposite (acting right)
				Force = MAX_FORCE_B;
				return;
			}
		}
		// The arm is stationary, or the force agrees with movement;
		// make it proportioal to the actuator setting
		Force = act * MAX_FORCE_F / ACTUATOR_ZERO;
	};

	inline void set_acceleration () {
		// Calculate the effective acceleration
		double ds;

		if (Speed == 0.0) {
			// The arm is stationary. extra force is needed to
			// overcome the friction and start moving it
			if (fabs (Force) < MinForce) {
				// Too low, ignore altogether
				Acceleration = 0.0;
				return;
			}
		}
		if (Speed < 0.0 && Force < 0.0 || Speed > 0.0 && Force > 0.0) {
			// Nonzero speed, and the force agrees with the speed;
			// the factor is inversely proportional to how far the
			// speed is from the max
			ds = (MAX_SPEED - fabs (Speed)) / MAX_SPEED;
		} else {
			// The factor is 1.0
			ds = 1.0;
		}
		Acceleration = (Force  * ds) / Mass;
	};

	inline void update_position () {
		// Update the position, easy enough
		Position += Speed * DeltaT;
		enforce_bounds (Position, 0.0, ARM_RANGE);
	};

	inline void update_speed () {
		// Update the  speed
		double ns, eacc = Acceleration;

		// This is the new calculated speed resulting from applying
		// acceleration to the previous value
		ns = Speed + eacc * DeltaT;

		if ((ns < 0.0 && (Speed > 0.0 || Position <= 0.0)) ||
		    (ns > 0.0 && (Speed < 0.0 || Position >= ARM_RANGE))) {
			// This condition means that we have a direction change,
			// or the arm has hit the end; in such a case we set the
			// speed at precisely 0.0 for this turn, so we account
			// for the friction needed to start moving the arm
			// again
			Speed = 0.0;
		} else {
			// Otherwise, it is just as calculated
			Speed = ns;
		}
	};

	inline void update_sensor () {
		// Transform position to sensor reading, simple enough
		int s;
		s = SENSOR_MIN + (int) round (Position * (SENSOR_MAX -
			SENSOR_MIN) / ARM_RANGE);
		enforce_bounds (s, SENSOR_MIN, SENSOR_MAX);
		setSensor (s);
	};

	void setup (double mass) {
		// This is the total mass to move around
		Mass = mass + ARM_MASS;
		// Precalculated minimum force for the mass to overcome
		// the static fricition
		MinForce = Mass * MIN_FORCE;
		// Set up the controller
		Controller::setup (DELTA_T, CONTROLLER_HOST, VUEE_PORT, 0, 
			THE_SENSOR, THE_ACTUATOR, 0);
	};

	void showState ();

	void response ();
	void reset ();
};

process Root {

	states { Start, Never };

	void buildPlant ();

	perform;
};

process Logger (RArm) {

	TIME NextRunTime, Delta;

	states { Loop };

	void setup (int);

	perform;
};

#endif
