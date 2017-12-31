identify (Car Wash);

// Services types
const int standard = 0, extra = 1, deluxe = 2;

typedef struct {
	double min, max;
} service_time_bounds_t;

// Assume these are in minutes
const service_time_bounds_t service_time_bounds [] = {
	// These are in minutes
	{ 4.0,  6.0 },
	{ 6.0,  9.0 },
	{ 9.0, 12.0 }
};

// Cumulative frequency of cars asking for specific service
const double service_percentage [] = { 0.8, 0.92, 1.00 };

// ============================================================================

class car_t {

	public:

	int service;

	car_t (int s) {
		service = s;
	};
};

mailbox car_queue_t (car_t*) {

	void setup () {
		setLimit (MAX_Long);
	};
};

car_queue_t *the_lineup;

// ============================================================================

// Here is what we measure:

double TotalServiceTime = 0.0;
Long NumberOfProcessedCars = 0;

// ============================================================================

process entrance {

	double IAT;

	void setup (double iat) {
		IAT = iat;
	};

	states { NothingHappening, CarArriving };

	perform {

		state NothingHappening:

			Timer->delay (dRndPoisson (IAT), CarArriving);

		state CarArriving:

			int s;
			double d = dRndUniform (0.0, 1.0);

			for (s = standard; s < deluxe; s++)
				if (d <= service_percentage [s])
					break;

			the_lineup->put (new car_t (s));

			sameas NothingHappening;
	};
};

process washer {

	car_t *the_car;

	states { WaitingForCar, Washing, DoneWashing };

	double service_time (int service) {
		// Returns a beta-distributed variate between min and max
		return dRndTolerance (
			// Service time is in minutes
			service_time_bounds [service] . min,
			service_time_bounds [service] . max,
			3
		);
	};

	perform {

		state WaitingForCar:

			if (the_lineup->empty ()) {
				the_lineup->wait (NONEMPTY, WaitingForCar);
				sleep;
			}

			the_car = the_lineup->get ();

		transient Washing:

			double st;

			st = service_time (the_car->service);
			Timer->delay (st, DoneWashing);
			TotalServiceTime += st;
			NumberOfProcessedCars++;

		state DoneWashing:

			delete the_car;
			sameas WaitingForCar;
	};
};

// ============================================================================

process Root {

	states { Start, Stop };

	perform {

		state Start:

			double d;

			// Internal time unit == 1 second
			setEtu (60.0);

			// Interarrival time
			readIn (d);

			the_lineup = create car_queue_t ();
			create entrance (d);
			create washer ();

			// Simulation time
			readIn (d);
			setLimit (0, d);

			Kernel->wait (DEATH, Stop);

		state Stop:

			// Output results
			print (TotalServiceTime, "Busy time:", 10, 26);
			print ((TotalServiceTime / ETime) * 100.0,
				"Normalized throughput:", 10, 26);
			print (NumberOfProcessedCars, "Cars washed:", 10, 26);
			print (the_lineup->getCount (), "Cars queued:", 10, 26);

			Kernel->terminate ();
	};
};
