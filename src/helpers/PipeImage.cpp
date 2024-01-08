/*
 * Copyright 2023, Andrea Anzani 
 *
 * Source code derived from AGMSScriptOCron
 * 	Copyright (c) 2018 by Alexander G. M. Smith.
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PipeImage.h"

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <image.h>

#include <Locker.h>

 //lock access play with stdin/stdout
BLocker* PipeImage::sLockStdFilesPntr = new BLocker ("Std-In-Out Changed Lock");

status_t
PipeImage::Init(const char **argv, int32 argc, bool dupStdErr, bool resume)
{
	// first we should backup current STDIO/STDOUT
	// then we prepare the pipes for the 'child'
	// we load_image
	// if ok we fix the stdio/stdout to link to the child
	// if ko we revert original state.
	// we run the thread. crossing finger.

	fDupStdErr = dupStdErr;

	sLockStdFilesPntr->Lock();

	int originalStdIn = dup (STDIN_FILENO);
	int PipeFlags = fcntl (originalStdIn, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdIn, F_SETFD, PipeFlags);

	int originalStdOut = dup (STDOUT_FILENO);
	PipeFlags = fcntl (originalStdOut, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdOut, F_SETFD, PipeFlags);

	int originalStdErr = 0;
	if (fDupStdErr) {
		originalStdErr = dup(STDERR_FILENO);
		PipeFlags = fcntl (originalStdErr, F_GETFD);
		PipeFlags |= FD_CLOEXEC;
		fcntl (originalStdErr, F_SETFD, PipeFlags);
	}

  sLockStdFilesPntr->Unlock();


	if (pipe (fOutPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.

	if (pipe (fInPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.

	if (fDupStdErr) {
		if (pipe (fErrPipe) != 0) // Returns -1 if failed, 0 if okay.
			return errno; // Pipe creation failed.
	}

	// Write end of the outPipe NOT used by the child, make it close on exec.
	PipeFlags = fcntl (fOutPipe[WRITE_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fOutPipe[WRITE_END], F_SETFD, PipeFlags);

	// Read end of the inPipe NOT used by the child, make it close on exec.
	PipeFlags = fcntl (fInPipe[READ_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fInPipe[READ_END], F_SETFD, PipeFlags);

	if (fDupStdErr) {
		// Read end of the errPipe NOT used by the child, make it close on exec.
		PipeFlags = fcntl (fErrPipe[READ_END], F_GETFD);
		PipeFlags |= FD_CLOEXEC;
		fcntl (fErrPipe[READ_END], F_SETFD, PipeFlags);
	}

	dup2(fInPipe[WRITE_END], STDOUT_FILENO);
	close(fInPipe[WRITE_END]);
	dup2(fOutPipe[READ_END], STDIN_FILENO);
	close(fOutPipe[READ_END]);

	if (fDupStdErr) {
		dup2(fErrPipe[WRITE_END], STDERR_FILENO);
		close(fErrPipe[WRITE_END]);
	}

	status_t stat = B_OK;
	fChildpid = load_image (argc, argv, const_cast<const char **>(environ));
	if (fChildpid < 0)
	{
		if (fDupStdErr) {
			close(fErrPipe[READ_END]);
		}
		close(fOutPipe[WRITE_END]);
		close(fInPipe[READ_END]);
		stat = fChildpid;
	}
	else
	{
		setpgid (fChildpid, fChildpid);
		if (resume)
			resume_thread (fChildpid); // rock'n'roll!
	}
	if (fDupStdErr) {
		close(fErrPipe[WRITE_END]);
	}
	close(fOutPipe[READ_END]);
	close(fInPipe[WRITE_END]);

	sLockStdFilesPntr->Lock();

	dup2 (originalStdIn, STDIN_FILENO);
	dup2 (originalStdOut, STDOUT_FILENO);
	if (fDupStdErr) {
		dup2 (originalStdErr, STDERR_FILENO);
	}
	sLockStdFilesPntr->Unlock();

	return stat;
}

void
PipeImage::Close()
{
	close(fOutPipe[WRITE_END]);
	if (fDupStdErr) {
		close(fErrPipe[READ_END]);
	}
	close(fInPipe[READ_END]);
}

PipeImage::~PipeImage()
{
	Close();
}

pid_t
PipeImage::GetChildPid()
{
	return fChildpid;
}

ssize_t
PipeImage::ReadError(void* buffer, size_t size)
{
	return read(fErrPipe[READ_END], buffer, size);
}


ssize_t
PipeImage::Read(void* buffer, size_t size)
{
	return read(fInPipe[READ_END], buffer, size);
}

ssize_t
PipeImage::Write(const void* buffer, size_t size)
{
	return write(fOutPipe[WRITE_END], buffer, size);
}

