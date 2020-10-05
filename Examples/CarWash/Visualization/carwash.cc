/*
	Copyright 1995-2020 Pawel Gburzynski

	This file is part of SMURPH/SIDE.

	SMURPH/SIDE is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	SMURPH/SIDE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SMURPH/SIDE. If not, see <https://www.gnu.org/licenses/>.
*/
identify (Car Wash);

// Services types
const int standard = 0, extra = 1, deluxe = 2;

const char *service_name [] = { "standard", "extra", "deluxe" };

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

int Ticket = 0;

class car_t {

	public:

	int service;
	int number;

	car_t (int s) {
		service = s;
		number = Ticket++;
		trace ("car %1d arriving for %s service",
			number, service_name [service]);
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
			trace ("car %1d service %s start", the_car->number,
				service_name [the_car->service]);
			Timer->delay (st, DoneWashing);
			TotalServiceTime += st;
			NumberOfProcessedCars++;

		state DoneWashing:

			trace ("car %1d service %s end", the_car->number,
				service_name [the_car->service]);
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

			setResync (1000, 1.0/60.0);

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
