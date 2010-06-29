#define	BUFFSIZE 128
#define	MAXUARTS 32

mailbox UART (int);

struct packet_s;

typedef struct packet_s {

	packet_s *Next;
	int Size;
	char Stuff [];

} packet_t;

// ============================================================================

packet_t *PQueue = NULL,
	 *PQLast;

void queue_packet (packet_t *p) {

	if (PQueue == NULL) {
		PQueue = PQLast = p;
		Monitor->signal (&PQueue);
	} else {
		PQLast->Next = p;
		PQLast = p;
	}

	p->Next = NULL;

}

char *uart_name (int i) {

	return form ("/dev/com%1d:", i);
}
		
process reader {

	UART *Input;
	char *Buffer;
	packet_t *P;

	void setup (UART *mb) {

		Input = mb;
		Input->setSentinel ('\n');
		Buffer = new char [BUFFSIZE];
	};

	states { Wait };

	perform {

		int nc;

		state Wait:

			if ((nc = Input->readToSentinel (Buffer, BUFFSIZE)) ==
			    0) {
				Input->wait (SENTINEL, Wait);
				sleep;
			}

			// Queue the data for sending out
			P = (packet_t*) new char [sizeof (packet_t) + nc];

			P->Size = nc;
			bcopy (Buffer, &(P->Stuff), nc);

			queue_packet (P);
			proceed Wait;
	}
};

process writer {

	UART *Uarts [MAXUARTS];
	int NUarts, CU;
	packet_t *CBuf;
	void setup ();

	states { Init, Loop, Write };

	perform;
};

void writer::setup () {
//
// Identify all UARTS
//
	int i;
	UART *u;

	NUarts = 0;

	u = create UART;

	for (i = 0; i < 100; i++) {

		if (u->connect (DEVICE+RAW+READ+WRITE, uart_name (i), 0,
			BUFFSIZE, 9600, 0, 0))
				continue;

		// Success
		Uarts [NUarts++] = u;
		u = create UART;

		if (NUarts == MAXUARTS)
			break;

	}

	Ouf << NUarts << " UARTs found\n";

	// Spawn the readers

	for (i = 0; i < NUarts; i++)
		create reader (Uarts [i]);
}

writer::perform {

	state Init:

		if (NUarts == 0) {
			Kernel->terminate ();
			terminate;
		}

	transient Loop:

		if (PQueue == NULL) {
			Monitor->wait (&PQueue, Loop);
			sleep;
		}

		PQueue = (CBuf = PQueue) -> Next;

		CU = 0;

	transient Write:

		if (CU == NUarts) {
			delete CBuf;
			proceed Loop;
		}

		// Handle UART number CU

		if ((Uarts [CU] -> write (CBuf->Stuff, CBuf->Size)) !=
			ACCEPTED) {

				Uarts [CU] -> wait (OUTPUT, Write);
				sleep;
		}

		CU++;
		proceed Write;
}

process Root {

	states { Start, Stop };

	perform {

		state Start:

			create writer;
			Kernel->wait (DEATH, Stop);

		state Stop:

			terminate;
	};
};
