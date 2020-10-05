/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

char *ptsname (int);
int grantpt (int);
int unlockpt (int);

int tty_setraw (int desc, int speed, int parity, int stopb) {

	struct termios NewTTYStat;

	if (tcgetattr (desc, &NewTTYStat) < 0)
		return -1;

	NewTTYStat.c_iflag |= IGNBRK | IGNPAR;
	NewTTYStat.c_iflag &=
                            ~ISTRIP & ~INLCR & ~IGNCR & ~ICRNL & ~IXON & ~IXOFF;

	NewTTYStat.c_lflag &=
                            ~ECHO & ~ECHOE & ~ECHOK & ~ECHONL & ~ICANON & ~ISIG;
	NewTTYStat.c_oflag &= ~OPOST;

	NewTTYStat.c_cflag &= ~PARENB;
        if (parity) {
	  NewTTYStat.c_cflag &= ~CSIZE;
	  NewTTYStat.c_cflag |= CS7;
	  NewTTYStat.c_cflag |= PARENB;
	  NewTTYStat.c_cflag &= ~PARODD;
          if (parity % 2) NewTTYStat.c_cflag |= PARODD;
        }
        if (stopb) {
	  NewTTYStat.c_cflag &= ~CSTOPB;
          if (stopb > 1) NewTTYStat.c_cflag |= CSTOPB;
        }
	NewTTYStat.c_cc [VEOF] = 1;     // Immediately read any single char

        switch (speed) {
          case 50:
            cfsetospeed (&NewTTYStat, B50);
            cfsetispeed (&NewTTYStat, B50);
          case 0:
            break;
          case 75:
            cfsetospeed (&NewTTYStat, B75);
            cfsetispeed (&NewTTYStat, B75);
            break;
          case 110:
            cfsetospeed (&NewTTYStat, B110);
            cfsetispeed (&NewTTYStat, B110);
            break;
          case 134:
            cfsetospeed (&NewTTYStat, B134);
            cfsetispeed (&NewTTYStat, B134);
            break;
          case 150:
            cfsetospeed (&NewTTYStat, B150);
            cfsetispeed (&NewTTYStat, B150);
            break;
          case 200:
            cfsetospeed (&NewTTYStat, B200);
            cfsetispeed (&NewTTYStat, B200);
            break;
          case 300:
            cfsetospeed (&NewTTYStat, B300);
            cfsetispeed (&NewTTYStat, B300);
            break;
          case 600:
            cfsetospeed (&NewTTYStat, B600);
            cfsetispeed (&NewTTYStat, B600);
            break;
          case 1200:
            cfsetospeed (&NewTTYStat, B1200);
            cfsetispeed (&NewTTYStat, B1200);
            break;
          case 1800:
            cfsetospeed (&NewTTYStat, B1800);
            cfsetispeed (&NewTTYStat, B1800);
            break;
          case 2400:
            cfsetospeed (&NewTTYStat, B2400);
            cfsetispeed (&NewTTYStat, B2400);
            break;
          case 4800:
            cfsetospeed (&NewTTYStat, B4800);
            cfsetispeed (&NewTTYStat, B4800);
            break;
          case 9600:
            cfsetospeed (&NewTTYStat, B9600);
            cfsetispeed (&NewTTYStat, B9600);
            break;
          case 19200:
            cfsetospeed (&NewTTYStat, B19200);
            cfsetispeed (&NewTTYStat, B19200);
            break;
          case 38400:
            cfsetospeed (&NewTTYStat, B38400);
            cfsetispeed (&NewTTYStat, B38400);
            break;
#ifdef	B57600
          case 57600:
            cfsetospeed (&NewTTYStat, B57600);
            cfsetispeed (&NewTTYStat, B57600);
            break;
#endif
#ifdef	B115200
          case 115200:
            cfsetospeed (&NewTTYStat, B115200);
            cfsetispeed (&NewTTYStat, B115200);
            break;
#endif
#ifdef	B230400
          case 230400:
            cfsetospeed (&NewTTYStat, B230400);
            cfsetispeed (&NewTTYStat, B230400);
            break;
#endif
          default:
		return -1;
        }
	if (tcsetattr (desc, TCSANOW, &NewTTYStat) < 0)
		return -1;

	return 0;
}

int main () {

	int fdm0, fdm1, ps;
	char c;

	if ((fdm0 = open ("/dev/ptmx", O_RDWR)) < 0) {
PSErr:
		perror ("Couldn't open pseudo tty");
		exit (99);
	}

	if ((fdm1 = open ("/dev/ptmx", O_RDWR)) < 0)
		goto PSErr;

	if (tty_setraw (fdm0, 115200, 0, 1) < 0) {
PAErr:
		perror ("Couldn't set tty params");
		exit (99);
	}

	if (tty_setraw (fdm1, 115200, 0, 1) < 0)
		goto PAErr;

	if ((ps = fork ()) < 0) {
		perror ("Cannot fork");
		exit (99);
	}

	if (ps) {
		// Switch sides
		ps = fdm0;
		fdm0 = fdm1;
		fdm1 = ps;
	}

	grantpt (fdm0);
	unlockpt (fdm0);

	printf ("TTY name: %s\n", ptsname (fdm0));

	ps = 0;
	while (1) {
		if (read (fdm0, &c, 1) != 1) {
			if (ps) {
				usleep (1000);
				continue;
			}
			printf ("EOF on %s\n", ptsname (fdm0));
			ps++;
			continue;
		}
		ps = 0;
		if (write (fdm1, &c, 1) != 1) {
			perror ("TTY output failed");
			exit (99);
		}
	}
}
