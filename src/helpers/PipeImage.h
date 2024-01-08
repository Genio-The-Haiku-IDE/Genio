/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PipeImage_H
#define PipeImage_H

#include <SupportDefs.h>

#define READ_END 0
#define WRITE_END 1

// to read from the new image created:   read(fInPipe[READ_END],..
// to write from the new image created:  write(fOutPipe[WRITE_END],..

class BLocker;
class PipeImage {

public:

  status_t Init(const char **argv, int32 argc, bool dupStdErr, bool resume);

  virtual ~PipeImage();
  void	Close();

  pid_t GetChildPid();

  ssize_t ReadError(void* buffer, size_t size);
  ssize_t Read(void* buffer, size_t size);
  ssize_t Write(const void* buffer, size_t size);

  static BLocker *sLockStdFilesPntr;

  int GetStdOutFD() { return fInPipe[READ_END]; };
  int GetStdErrFD() { return fErrPipe[READ_END]; };
  int GetStdInFD() { return fOutPipe[WRITE_END]; };

protected:


  pid_t fChildpid;
  int fOutPipe[2];
  int fInPipe[2];
  int fErrPipe[2];
  bool	fDupStdErr;
};


#endif // PipeImage_H
