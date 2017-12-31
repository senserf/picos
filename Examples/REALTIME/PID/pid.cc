identify "PD Controller";

#define	bounded(v,mi,ma)	((v) > (ma) ? (ma) : (v) < (mi) ? (mi) : (v))
#define	OBSIZE			256

station Plant {

	int VMin, VMax, Setpoint, SMin, SMax;
	double Kp, Ki, Kd, integral, lasterr;

	Process *CP, *OP;
	Mailbox *CI, *OI;

	void setup ();
	void control (int);
	void setpoint (int);
};

process Callback abstract (Plant) {

	states { Loop };

	virtual void action (int) = 0;

	perform {
		state Loop:
			action (Loop);
		sameas Loop;
	};
};

process Control : Callback (Plant) {

	void action (int st) { S->control (st); };
};

process Operator : Callback (Plant) {

	void action (int st) { S->setpoint (st); };
};

void Plant::control (int st) {

	int32_t output, input;
	int nc;
	double derivative, error, setting;

	if ((nc = CI->read ((char*)(&output), 4)) == ERROR) {
		// Stop the controller
		delete CI;
		CP->terminate ();
		CP = NULL;
		sleep;
	}

	if (nc != ACCEPTED) {
		CI->wait (4, st);
		sleep;
	}

	error = (double) Setpoint - (double) bounded (output, VMin, VMax);
	integral += error;
	derivative = error - lasterr;
	lasterr = error;
	setting = Kp * error + Ki * integral + Kd * derivative;
	input = (int32_t) bounded (setting, SMin, SMax);

	trace ("O: %1d [S: %1d, E: %1.0f, I: %1.0f, D: %1.0f, V: %1d]",
		output, Setpoint, error, integral, derivative, input);

	CI->write ((char*)(&input), 4);
}

void Plant::setpoint (int st) {

	char buf [OBSIZE];
	int s, nc;

	if ((nc = OI->readToSentinel (buf, OBSIZE)) == ERROR) {
		// Stop the operator
		delete OI;
		OP->terminate ();
		OP = NULL;
		sleep;
	}

	if (nc == 0 || sscanf (buf, "%d", &s) != 1) {
		OI->wait (SENTINEL, st);
		sleep;
	}

	Setpoint = (int32_t) bounded (s, VMin, VMax);
}

void Plant::setup () {

	readIn (VMin);
	readIn (VMax);
	readIn (Setpoint);
	readIn (SMin);
	readIn (SMax);
	readIn (Kp);
	readIn (Ki);
	readIn (Kd);

	lasterr = integral = 0.0;

	CP = OP = NULL;
	CI = OI = NULL;
}

// ============================================================================

process Connector abstract {

	Mailbox *M;
	int BSize;

	states { WaitClient };

	virtual void service (Mailbox*) = 0;

	perform;
};

process CConnector : Connector (Plant) {

	void service (Mailbox *c) {

		if (S->CP != NULL) {
			delete c;
			return;
		}

		S->CI = c;
		S->CP = create Control;
	};

	void setup (Mailbox *m) {
		M = m;
		BSize = 4;
	};
};

process OConnector : Connector (Plant) {

	void service (Mailbox *c) {

		if (S->OP != NULL) {
			delete c;
			return;
		}

		S->OI = c;
		S->OP = create Operator;
		c->setSentinel ('\n');
	};

	void setup (Mailbox *m) {
		M = m;
		BSize = OBSIZE;
	};
};

// ============================================================================
	
Connector::perform {

	state WaitClient:

		Mailbox *c;
		if (M->isPending ()) {
			c = create Mailbox;
			if (c->connect (M, BSize) != OK) {
				delete c;
				proceed WaitClient;
			}

			service (c);
		}

		M->wait (UPDATE, WaitClient);
}


process Root {

	states { Start };

	perform {

		state Start:

			Mailbox *ci, *oi;
			int port_p, port_o;
			Plant *p;

			readIn (port_p);
			readIn (port_o);

			p = create Plant;

			ci = create Mailbox;
			oi = create Mailbox;

			if (ci->connect (INTERNET + SERVER + MASTER, port_p) !=
			    OK)
				excptn ("Cannot set up plant interface");
			if (oi->connect (INTERNET + SERVER + MASTER, port_o) !=
			    OK)
				excptn ("Cannot set up operator interface");

			create (p) CConnector (ci);
			create (p) OConnector (oi);
	};
};
