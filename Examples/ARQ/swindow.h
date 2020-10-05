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

// This class implements a generic sliding window of objects of any (fixed)
// length accompanied by status information of any (fixed) length

class SlidingWindow {

	private:

	char *storage;
	int  NBuffers, PSize, SSize, TSize, start;

	public:

	SlidingWindow (int nb, int psize, int ssize) {
		// nb    - capacity, i.e., the window size
		// psize - size (in bytes) of the packet data structure
		// ssize - size (in bytes) of the status record
		TSize = (PSize = psize) + (SSize = ssize);
		storage = new char [(NBuffers = nb) * TSize];
		start = 0;
	};

	void getStatus (void *sp, int pos) {
		assert (pos >= 0 && pos < NBuffers,
			"SlidingWindow->getStatus: out of range");
		pos = (start + pos) % NBuffers;
		memcpy (sp, (void*)(storage + pos * TSize), SSize);
	};

	void setStatus (void *sp, int pos) {
		assert (pos >= 0 && pos < NBuffers,
			"SlidingWindow->setStatus: out of range");
		pos = (start + pos) % NBuffers;
		memcpy ((void*)(storage + pos * TSize), sp, SSize);
	};

	void put (const void *p, int pos) {
		assert (pos >= 0 && pos < NBuffers,
			"SlidingWindow->put: out of range");
		pos = (start + pos) % NBuffers;
		memcpy ((void*)(storage + SSize + pos*TSize), p, PSize);
	};

	void get (void *p, int pos) {
		assert (pos >= 0 && pos < NBuffers,
			"SlidingWindow->get: out of range");
		pos = (start + pos) % NBuffers;
		memcpy ((void*)p, (void*)(storage + SSize + pos*TSize), PSize);
	};

	void slide (int by = 1) {
		start = (start + by) % NBuffers;
	};
};
