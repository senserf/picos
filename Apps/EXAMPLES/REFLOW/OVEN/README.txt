This is an oven model. To compile: mks -R. To run:

	./side -- -h heating_tc -c cooling_hc -i inertia -l display_freq

heating_tc:	the heating time constant; higher values make the oven slower
		to heat up; the default is 30.0 (this is a floating point
		number)

cooling_tc:	the cooling time constant; higher values make the oven cool down
		clower; the default is 50.0

inertia:	the inertia (the persistence of previous setting); this is the
		EMA fsctor for the previous setting (the factor for the new
		setting being 1 - inertia); the default is 0.7

display_freq:	how often the model prints out its state (in seconds); the
		default is 1.
