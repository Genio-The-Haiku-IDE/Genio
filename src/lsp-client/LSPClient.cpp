/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
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
static BLocker *g_LockStdFilesPntr = new BLocker ("Std-In-Out Changed Lock");

status_t 
LSPClient::Init(const char *argv[]) {

	// first we should backup current STDIO/STDOUT
	// then we prepare the pipes for the 'child'
	// we load_image
	// if ok we fix the stdio/stdout to link to the child
	// if ko we revert original state.
	// we run the thread. crossing finger.
  
	int originalStdIn;
	int originalStdOut;
	
	g_LockStdFilesPntr->Lock();
	
    int PipeFlags = 0;

	originalStdIn = dup (STDIN_FILENO);
	PipeFlags = fcntl (originalStdIn, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdIn, F_SETFD, PipeFlags);

	originalStdOut = dup (STDOUT_FILENO);
	PipeFlags = fcntl (originalStdOut, F_GETFD);
	PipeFlags |= FD_CLOEXEC;
	fcntl (originalStdOut, F_SETFD, PipeFlags);
	
	g_LockStdFilesPntr->Unlock();

  
  if (pipe (outPipe) != 0) // Returns -1 if failed, 0 if okay.
    return errno; // Pipe creation failed.

  if (pipe (inPipe) != 0) // Returns -1 if failed, 0 if okay.
    return errno; // Pipe creation failed.

    
  // Write end of the outPipe not used by the child, make it close on exec.
  PipeFlags = fcntl (outPipe[WRITE_END], F_GETFD);
  PipeFlags |= FD_CLOEXEC;
  fcntl (outPipe[WRITE_END], F_SETFD, PipeFlags);
  
  // Read end of the inPipe not used by the child, make it close on exec.
  PipeFlags = fcntl (inPipe[READ_END], F_GETFD);
  PipeFlags |= FD_CLOEXEC;
  fcntl (inPipe[READ_END], F_SETFD, PipeFlags);
  
  dup2(inPipe[WRITE_END], STDOUT_FILENO);
  close(inPipe[WRITE_END]);
  dup2(outPipe[READ_END], STDIN_FILENO);
  close(outPipe[READ_END]);
  
  status_t stat = B_OK;
  childpid = load_image (5, argv, const_cast<const char **>(environ));
  if (childpid < 0)
  {
    close(outPipe[WRITE_END]);
    close(inPipe[READ_END]);
    stat = childpid;
  }
  else
  {
	setpgid (childpid, childpid);
	resume_thread (childpid); // rock'n'roll!
  }

  close(outPipe[READ_END]);
  close(inPipe[WRITE_END]);
  
  g_LockStdFilesPntr->Lock();
  dup2 (originalStdIn, STDIN_FILENO);
  dup2 (originalStdOut, STDOUT_FILENO);
  g_LockStdFilesPntr->Unlock();

  return stat;
}

void	
LSPClient::Close()
{
	close(outPipe[WRITE_END]);
	close(inPipe[READ_END]);
}

LSPClient::~LSPClient() {
	Close();
}
void LSPClient::SkipLine() {
  char xread;
  while (read(inPipe[READ_END], &xread, 1)) {
    if (xread == '\n') {
      break;
    }
  }
}
int LSPClient::ReadLength() {
  char szReadBuffer[255];
  int hasRead = 0;
  int length = 0;
  while ((hasRead = read(inPipe[READ_END], &szReadBuffer[length], 1)) != -1) {
    if (hasRead == 0 || length >= 254) // pipe eof or protection
      return 0;

    if (szReadBuffer[length] == '\n') {
      break;
    }
    length++;
  }
  return atoi(szReadBuffer + 16);
}
int LSPClient::Read(int length, std::string &out) {
  int readSize = 0;
  int hasRead;
  out.resize(length);
  while ((hasRead = read(inPipe[READ_END], &out[readSize], length)) != -1) {

    if (hasRead == 0) // pipe eof
      return 0;

    readSize += hasRead;
    if (readSize >= length) {
      break;
    }
  }

  return readSize;
}
bool LSPClient::Write(std::string &in) {

  int hasWritten = 0;
  int writeSize = 0;
  int totalSize = in.length();

  if (writeLock.Lock()) // for production code: WithTimeout(1000000) == B_OK)
  {
    while ((hasWritten =
                write(outPipe[WRITE_END], &in[writeSize], totalSize)) != -1) {
      writeSize += hasWritten;
      if (writeSize >= totalSize || hasWritten == 0) {
        break;
      }
    }
    writeLock.Unlock();
  }
  return (hasWritten != 0);
}
bool LSPClient::readJson(json &json) {
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
bool LSPClient::writeJson(json &json) {
  std::string content = json.dump();
  std::string header = "Content-Length: " + std::to_string(content.length()) +
                       "\r\n\r\n" + content;
  LogTrace("Client: - snd \n%s\n", content.c_str());
  return Write(header);
}
LSPClient::LSPClient(){};
pid_t LSPClient::GetChildPid() { return childpid; }
