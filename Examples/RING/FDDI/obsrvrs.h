/*
	Copyright 1995-2018, 2019 Pawel Gburzynski

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
observer TokenMonitor {
  TIME TokenPassingTimeout;
  states {Resume, Verify, Duplicate, Lost};
  void setup () {
    TokenPassingTimeout = TTRT + TTRT;
  };
  perform;
};

observer FairnessMonitor {
  TIME MaxDelay;
  void setup (TIME md) { MaxDelay = md; };
  states {Resume, CheckDelay};
  perform;
};
