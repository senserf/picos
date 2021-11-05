#include "rf.h"
#include "tag.h"
#include "common.h"
#include "emulator.h"

// ============================================================================
// ============================================================================

word	PegId, SampleInterval;
lword	SampleCount;
sint	RFC;			// RF descriptor

// ============================================================================

fsm sampler {

	address smp;

	state SM_APB:

		if (SampleInterval == 0) {
			// This is a signal to quit
			sensors_off ();
			finish;
		}

		// Acquire packet buffer
		smp = tcv_wnp (SM_APB, RFC, FRAME_LENGTH + SAMPLE_LENGTH + 4);
		pkt_did (smp) = PegId;
		pkt_sid (smp) = NODE_ID;
		pkt_cmd (smp) = PKT_REPORT;
		pkt_ple (smp) = SAMPLE_LENGTH + 4;

	state SM_READ:

		rds (SM_READ, (address) (pkt_pay (smp) + 4));
		// Insert sample count
		((lword*)(pkt_pay (smp))) [0] = SampleCount++;
		tcv_endp (smp);
		blink (0, 1, 128, 128, 10);
		delay (SampleInterval, SM_APB);
}

static inline void start_sampling (word interval) {

	SampleInterval = interval;
	if (!running (sampler)) {
		SampleCount = 0;
		// Power the sensors on
		sensors_on ();
		// Start the thread
		runfsm sampler;
	}
	// 4 quick blinks
	blink (0, 4, 128, 256, 200);
}

static inline void stop_sampling () {
	// Soft stop
	SampleInterval = 0;
	// 2 quick blinks
	blink (0, 2, 128, 256, 200);
}

static void process_command (const address cmd) {

	word interval;

	if (!pkt_issane (cmd))
		// Ignore
		return;

	PegId = pkt_sid (cmd);

	switch (pkt_cmd (cmd)) {

		case PKT_RUN:

			// Extract the interval
			if (pkt_ple (cmd) < 2)
				// Just ignore, bad packet
				return;
			interval = ((word*)pkt_pay (cmd)) [0];
			if (interval < MIN_SAMPLING_INTERVAL)
				interval = MIN_SAMPLING_INTERVAL;
			else if (interval > MAX_SAMPLING_INTERVAL)
				interval = MAX_SAMPLING_INTERVAL;
			// Reset the interval
			start_sampling (interval);
			return;

		case PKT_STOP:

			stop_sampling ();
			return;
	}
	// Ignore everything else
}

static void init () {

	word sid;

	phys_cc1350 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	RFC = tcv_open (NONE, 0, 0);
	sid = ((word)(host_id >> 16));
	// Set network Id
	tcv_control (RFC, PHYSOPT_SETSID, &sid);
	tcv_control (RFC, PHYSOPT_RXON, NULL);
}

fsm root {

	state RS_START:

		powerdown ();
		blink (0, 1, 512, 768, 200);
		init ();

		// Fall through, assume the role of RF receiver

	state RS_RX:

		address pkt;

		pkt = tcv_rnp (RS_RX, RFC);

		if (pkt_did (pkt) == NODE_ID)
			// If addressed to us
			process_command (pkt);

		tcv_endp (pkt);
		sameas RS_RX;
}
