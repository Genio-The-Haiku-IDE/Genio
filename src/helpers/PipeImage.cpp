/*
 * Copyright 2023, Andrea Anzani 
 *
 * Source code derived from AGMSScriptOCron
 * 	Copyright (c) 2018 by Alexander G. M. Smith.
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PipeImage.h"
#include "Log.h"
#include <image.h>


 //lock access play with stdin/stdout
BLocker* PipeImage::LockStdFilesPntr = new BLocker ("Std-In-Out Changed Lock");

status_t
PipeImage::Init(const char **argv, int32 argc, bool resume)
{
	// first we should backup current STDIO/STDOUT
	// then we prepare the pipes for the 'child'
	// we load_image
	// if ok we fix the stdio/stdout to link to the child
	// if ko we revert original state.
	// we run the thread. crossing finger.

	LockStdFilesPntr->Lock();

	int originalStdIn = dup (STDIN_FILENO);
	int PipeFlags = fcntl (originalStdIn, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdIn, F_SETFD, PipeFlags);

	int originalStdOut = dup (STDOUT_FILENO);
	PipeFlags = fcntl (originalStdOut, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdOut, F_SETFD, PipeFlags);

#if 0
	int originalStdErr = dup(STDERR_FILENO);
	PipeFlags = fcntl (originalStdErr, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdErr, F_SETFD, PipeFlags);
#endif

	LockStdFilesPntr->Unlock();

	if (pipe (fOutPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.

	if (pipe (fInPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.
#if 0
	if (pipe (fErrPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.
#endif
	// Write end of the outPipe not used by the child, make it close on exec.
	PipeFlags = fcntl (fOutPipe[WRITE_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fOutPipe[WRITE_END], F_SETFD, PipeFlags);
#if 0
	// Write end of the errPipe not used by the child, make it close on exec.
	PipeFlags = fcntl (fErrPipe[WRITE_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fErrPipe[WRITE_END], F_SETFD, PipeFlags);
#endif

	// Read end of the inPipe not used by the child, make it close on exec.
	PipeFlags = fcntl (fInPipe[READ_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fInPipe[READ_END], F_SETFD, PipeFlags);

	dup2(fInPipe[WRITE_END], STDOUT_FILENO);
	close(fInPipe[WRITE_END]);
	dup2(fOutPipe[READ_END], STDIN_FILENO);
	close(fOutPipe[READ_END]);
#if 0
	dup2(fErrPipe[READ_END], STDERR_FILENO);
	close(fErrPipe[READ_END]);
#endif

	status_t stat = B_OK;
	fChildpid = load_image (argc, argv, const_cast<const char **>(environ));
	if (fChildpid < 0)
	{
		#if 0
		close(fErrPipe[WRITE_END]);
		#endif
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
	#if 0
	close(fErrPipe[READ_END]);
	#endif
	close(fOutPipe[READ_END]);
	close(fInPipe[WRITE_END]);

	LockStdFilesPntr->Lock();
	dup2 (originalStdIn, STDIN_FILENO);
	dup2 (originalStdOut, STDOUT_FILENO);
	#if 0
	dup2 (originalStdErr, STDERR_FILENO);
	#endif
	LockStdFilesPntr->Unlock();

	return stat;
}

void
PipeImage::Close()
{
	close(fOutPipe[WRITE_END]);
	#if 0
	close(fErrPipe[WRITE_END]);
	#endif
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

#include <unistd.h>

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

