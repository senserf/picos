This is the same as WARSAW_10SHT, except that the SHT array is only 6 sensors
long. The remaining 4 slots (the last 4 slots from 10SHT) are used as analog
sensors.

Sensors 0-5 (the SHT array) are numbered as in WARSAW_10SHT. Sensors 6-9 refer
to pins P6.3, P6.4, P6.5, P6.6 (in this order, i.e., in the reverse order of
those slots in WARSAW_10SHT). Sensor 6 (pin P6.3) is intended for the sonar.
Its sampling time is long (about 550 msec) because of the delay needed to start
the sensor after power up. This assumes that the sensor's power supply is
connected to P1.7, i.e., the SHT clock doubling as the on switch for the sonar
(P1.7 is pulled up 0.5 sec before taking a measurement from sensor 6 [P6.3],
then it goes down again - to its idle state).
