/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Code freely taken from haiku Terminal:
 * Copyright 2001-2023, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Jonathan Schleifer, js@webkeks.org
 *		Simon South, simon@simonsouth.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include "MTerm.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <OS.h>
#include <SupportDefs.h>
#include "GMessage.h"

/* handshake interface */
typedef struct
{
	int status;		/* status of child */
	char msg[128];	/* error message */
	unsigned short row;		/* terminal rows */
	unsigned short col;		/* Terminal columns */
} handshake_t;

/* status of handshake */
#define PTY_OK	0	/* pty open and set termios OK */
#define PTY_NG	1	/* pty open or set termios NG */
#define PTY_WS	2	/* pty need WINSIZE (row and col ) */


#ifndef CEOF
#define CEOF ('D'&037)
#endif
#ifndef CSUSP
#define CSUSP ('Z'&037)
#endif
#ifndef CQUIT
#define CQUIT ('\\'&037)
#endif
#ifndef CEOL
#define CEOL 0
#endif
#ifndef CSTOP
#define CSTOP ('Q'&037)
#endif
#ifndef CSTART
#define CSTART ('S'&037)
#endif
#ifndef CSWTCH
#define CSWTCH 0
#endif

// private
static status_t
send_handshake_message(thread_id target, const handshake_t& handshake)
{
	return send_data(target, 0, &handshake, sizeof(handshake_t));
}


static void
receive_handshake_message(handshake_t& handshake)
{
	thread_id sender;
	receive_data(&sender, &handshake, sizeof(handshake_t));
}


static void
initialize_termios(struct termios &tio)
{
	/*
	 * Set Terminal interface.
	 */

	tio.c_line = 0;
	tio.c_lflag |= ECHOE;

	/* input: nl->nl, cr->nl */
	tio.c_iflag &= ~(INLCR|IGNCR);
	tio.c_iflag |= ICRNL;
	tio.c_iflag &= ~ISTRIP;

	/* output: cr->cr, nl in not retrun, no delays, ln->cr/ln */
	tio.c_oflag &= ~(OCRNL|ONLRET|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
	tio.c_oflag |= ONLCR;
	tio.c_oflag |= OPOST;

	/* baud rate is 19200 (equal beterm) */
	tio.c_cflag &= ~(CBAUD);
	tio.c_cflag |= B19200;

	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;
	tio.c_cflag |= CREAD;

	tio.c_cflag |= HUPCL;
	tio.c_iflag &= ~(IGNBRK|BRKINT);

	/*
	 * enable signals, canonical processing (erase, kill, etc), echo.
	*/
	tio.c_lflag |= ISIG|ICANON|ECHO|ECHOE|ECHONL;
	tio.c_lflag &= ~(ECHOK | IEXTEN);

	/* set control characters. */
	tio.c_cc[VINTR]  = 'C' & 0x1f;	/* '^C'	*/
	tio.c_cc[VQUIT]  = CQUIT;		/* '^\'	*/
	tio.c_cc[VERASE] = 0x7f;		/* '^?'	*/
	tio.c_cc[VKILL]  = 'U' & 0x1f;	/* '^U'	*/
	tio.c_cc[VEOF]   = CEOF;		/* '^D' */
	tio.c_cc[VEOL]   = CEOL;		/* '^@' */
	tio.c_cc[VMIN]   = 4;
	tio.c_cc[VTIME]  = 0;
	tio.c_cc[VEOL2]  = CEOL;		/* '^@' */
	tio.c_cc[VSWTCH] = CSWTCH;		/* '^@' */
	tio.c_cc[VSTART] = CSTART;		/* '^S' */
	tio.c_cc[VSTOP]  = CSTOP;		/* '^Q' */
	tio.c_cc[VSUSP]  = CSUSP;		/* '^Z' */
}

#include "Task.h"
#include <functional>
#include <Handler.h>
using namespace Genio::Task;

MTerm::MTerm(const BMessenger& msgr) : fExecProcessID(-1), fMessenger(msgr), fReadTask(nullptr)
{

}

void
MTerm::Run(int argc, const char* const* argv)
{
	if (_Spawn(argc, argv) != B_OK) {
		printf("error\n");
		return;
	}

	fReadTask = new Task<status_t>("_ReadThread", fMessenger, std::bind(&MTerm::_ReadThread, *this));
	fReadTask->Run();
}

void
MTerm::Kill()
{
	if(fExecProcessID > -1)
		kill_thread(fExecProcessID);
	if (fReadTask)
		fReadTask->Stop();
	close(fFd);
}


status_t
MTerm::_Spawn(int argc, const char* const* argv)
{
	signal(SIGTTOU, SIG_IGN);

	// get a pseudo-tty
	int master = posix_openpt(O_RDWR | O_NOCTTY);
	const char *ttyName;

	if (master < 0) {
		fprintf(stderr, "Didn't find any available pseudo ttys.");
		return errno;
	}

	if (grantpt(master) != 0 || unlockpt(master) != 0
		|| (ttyName = ptsname(master)) == NULL) {
		close(master);
		fprintf(stderr, "Failed to init pseudo tty.");
		return errno;
	}

	/*
	 * Get the modes of the current terminal. We will duplicates these
	 * on the pseudo terminal.
	 */

	thread_id terminalThread = find_thread(NULL);

	/* Fork a child process. */
	fExecProcessID = fork();
	if (fExecProcessID < 0) {
		close(master);
		return B_ERROR;
	}

	handshake_t handshake;

	if (fExecProcessID == 0) {
		// Now in child process.

		// close the PTY master side
		close(master);

		/*
		 * Make our controlling tty the pseudo tty. This hapens because
		 * we cleared our original controlling terminal above.
		 */

		/* Set process session leader */
		if (setsid() < 0) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"could not set session leader.");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		/* open slave pty */
		int slave = -1;
		if ((slave = open(ttyName, O_RDWR)) < 0) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"can't open tty (%s).", ttyName);
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		/* set signal default */
		signal(SIGCHLD, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);

		struct termios tio;
		/* get tty termios (not necessary).
		 * TODO: so why are we doing it ?
		 */
		tcgetattr(slave, &tio);

		initialize_termios(tio);

		/*
		 * change control tty.
		 */

		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);

		/* close old slave fd. */
		if (slave > 2)
			close(slave);

		/*
		 * set terminal interface.
		 */
		 tio.c_lflag &= ~ECHO;
		if (tcsetattr(0, TCSANOW, &tio) == -1) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"failed set terminal interface (TERMIOS).");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		/*
		 * set window size.
		 */

		handshake.status = PTY_WS;
		send_handshake_message(terminalThread, handshake);
		receive_handshake_message(handshake);

		if (handshake.status != PTY_WS) {
			handshake.status = PTY_NG;
			snprintf(handshake.msg, sizeof(handshake.msg),
				"mismatch handshake.");
			send_handshake_message(terminalThread, handshake);
			exit(1);
		}

		struct winsize ws = { handshake.row, handshake.col };

		ioctl(0, TIOCSWINSZ, &ws, sizeof(ws));

		tcsetpgrp(0, getpgrp());
			// set this process group ID as the controlling terminal
		set_thread_priority(find_thread(NULL), B_NORMAL_PRIORITY);

		/* pty open and set termios successful. */
		handshake.status = PTY_OK;
		send_handshake_message(terminalThread, handshake);


		// set the current working directory, if one is given
//		if (parameters.CurrentDirectory().Length() > 0)
//			chdir(parameters.CurrentDirectory().String());

		execve(argv[0], (char * const *)argv, environ);

		// Exec failed.
		// TODO: This doesn't belong here.
		printf("ERROR\n");
		exit(1);
	}

	/*
	 * In parent Process, Set up the input and output file pointers so
	 * that they can write and read the pseudo terminal.
	 */

	/*
	 * close parent control tty.
	 */

	int done = 0;
	while (!done) {
		receive_handshake_message(handshake);

		switch (handshake.status) {
			case PTY_OK:
				done = 1;
				break;

			case PTY_NG:
				fprintf(stderr, "%s\n", handshake.msg);
				done = -1;
				break;

			case PTY_WS:
				handshake.row = 0;
				handshake.col = 0;
				handshake.status = PTY_WS;
				send_handshake_message(fExecProcessID, handshake);
				break;
		}
	}

	if (done <= 0) {
		close(master);
		return B_ERROR;
	}

	fFd = master;
	return B_OK;
}


status_t
MTerm::_ReadThread()
{
	//TODO: spawn a new thread to read..
	uchar buf[READ_BUF_SIZE];
//	int i=0;
	while(1) {
		// Read PTY
		ssize_t nread = read(fFd, buf, READ_BUF_SIZE);
		if (nread <= 0) {
			//GMessage msg = {{"what", kMTReadDone }};
			//fMessenger.SendMessage(&msg);
			return B_OK;
		}
		GMessage msg = {{"what", kMTOutputText },{"text", BString((const char*)buf, nread)}};
		fMessenger.SendMessage(&msg);
	}
}

void
MTerm::Write(const void* buffer, size_t size)
{
	write(fFd, buffer, size);
}
