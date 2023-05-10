/*
 * Copyright 2023, Andrea Anzani 
 *
 * Source code derived from AGMSScriptOCron
 * 	Copyright (c) 2018 by Alexander G. M. Smith.
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "LSPClient.h"
#include "Log.h"
#include <image.h>
#define READ_END 0
#define WRITE_END 1

 //lock access play with stdin/stdout
BLocker *g_LockStdFilesPntr = new BLocker ("Std-In-Out Changed Lock");

status_t 
LSPClient::Init(const char *argv[])
{
	// first we should backup current STDIO/STDOUT
	// then we prepare the pipes for the 'child'
	// we load_image
	// if ok we fix the stdio/stdout to link to the child
	// if ko we revert original state.
	// we run the thread. crossing finger.
 	
	g_LockStdFilesPntr->Lock();
	
	int originalStdIn = dup (STDIN_FILENO);
	int PipeFlags = fcntl (originalStdIn, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdIn, F_SETFD, PipeFlags);

	int originalStdOut = dup (STDOUT_FILENO);
	PipeFlags = fcntl (originalStdOut, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdOut, F_SETFD, PipeFlags);
	
	g_LockStdFilesPntr->Unlock();
  
	if (pipe (fOutPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.

	if (pipe (fInPipe) != 0) // Returns -1 if failed, 0 if okay.
		return errno; // Pipe creation failed.

	// Write end of the outPipe not used by the child, make it close on exec.
	PipeFlags = fcntl (fOutPipe[WRITE_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fOutPipe[WRITE_END], F_SETFD, PipeFlags);
  
	// Read end of the inPipe not used by the child, make it close on exec.
	PipeFlags = fcntl (fInPipe[READ_END], F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (fInPipe[READ_END], F_SETFD, PipeFlags);
  
	dup2(fInPipe[WRITE_END], STDOUT_FILENO);
	close(fInPipe[WRITE_END]);
	dup2(fOutPipe[READ_END], STDIN_FILENO);
	close(fOutPipe[READ_END]);
  
	status_t stat = B_OK;
	fChildpid = load_image (5, argv, const_cast<const char **>(environ));
	if (fChildpid < 0)
	{
		close(fOutPipe[WRITE_END]);
		close(fInPipe[READ_END]);
		stat = fChildpid;
	}
	else
	{
		setpgid (fChildpid, fChildpid);
		resume_thread (fChildpid); // rock'n'roll!
	}
	
	close(fOutPipe[READ_END]);
	close(fInPipe[WRITE_END]);
  
	g_LockStdFilesPntr->Lock();
	dup2 (originalStdIn, STDIN_FILENO);
	dup2 (originalStdOut, STDOUT_FILENO);
	g_LockStdFilesPntr->Unlock();

	return stat;
}

void	
LSPClient::Close()
{
	close(fOutPipe[WRITE_END]);
	close(fInPipe[READ_END]);
}

LSPClient::~LSPClient()
{
	Close();
}

void
LSPClient::SkipLine()
{
	char xread;
	while (read(fInPipe[READ_END], &xread, 1)) {
		if (xread == '\n') {
			break;
		}
	}
}

int
LSPClient::ReadLength()
{
	char szReadBuffer[255];
	int hasRead = 0;
	int length = 0;
	while ((hasRead = read(fInPipe[READ_END], &szReadBuffer[length], 1)) != -1) {
		if (hasRead == 0 || length >= 254) // pipe eof or protection
			return 0;

		if (szReadBuffer[length] == '\n') {
			break;
		}
		length++;
	}
	return atoi(szReadBuffer + 16);
}

int
LSPClient::Read(int length, std::string &out)
{
	int readSize = 0;
	int hasRead;
	out.resize(length);
	while ((hasRead = read(fInPipe[READ_END], &out[readSize], length)) != -1) {
		if (hasRead == 0) // pipe eof
			return 0;

		readSize += hasRead;
		if (readSize >= length) {
			break;
		}
	}

	return readSize;
}

bool
LSPClient::Write(std::string &in)
{
	int hasWritten = 0;
	int writeSize = 0;
	int totalSize = in.length();

	if (fWriteLock.Lock()) // for production code: WithTimeout(1000000) == B_OK)
	{
		while ((hasWritten =
                write(fOutPipe[WRITE_END], &in[writeSize], totalSize)) != -1) {
			writeSize += hasWritten;
			if (writeSize >= totalSize || hasWritten == 0) {
				break;
			}
		}
		fWriteLock.Unlock();
	}
	return (hasWritten != 0);
}

bool
LSPClient::readJson(json &json)
{
	json.clear();
	int length = ReadLength();
	if (length == 0)
		return false;
	SkipLine();
	std::string read;
	if (Read(length, read) == 0)
		return false;
	try {
		json = json::parse(read);
	} catch (std::exception &e) {
		return false;
	}
	LogTrace("Client - rcv %d:\n%s\n", length, read.c_str());
	return true;
}

bool
LSPClient::writeJson(json &json)
{
	std::string content = json.dump();
	std::string header = "Content-Length: " + std::to_string(content.length()) +
                       "\r\n\r\n" + content;
	LogTrace("Client: - snd \n%s\n", content.c_str());
	return Write(header);
}

LSPClient::LSPClient()
{
}

pid_t
LSPClient::GetChildPid()
{
	return fChildpid;
}
