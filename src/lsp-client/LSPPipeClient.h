/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <unistd.h>

#include "PipeImage.h"
#include "Transport.h"

#include <Locker.h>

class LSPReaderThread;
class LSPPipeClient : public AsyncJsonTransport {

public:

			 LSPPipeClient(uint32 what, BMessenger& msgr);
	virtual ~LSPPipeClient();

	status_t Start(const char **argv, int32 argc);

	void	Close();

	bool 	readMessage(std::string &json) override;
	bool 	writeMessage(std::string &json) override;

	pid_t	GetChildPid();

	void	ForceQuit(); //quite the looper and the kill the thread
	bool	HasQuitBeenRequested();
	void	KillThread();

private:
  int 	ReadMessageHeader();
  bool	ReadHeaderLine(char* header, size_t maxlen);
  int 	Read(int length, std::string &out);
  bool 	Write(std::string &in);
  void	Quit() override;
  thread_id	Run() override;

  BLocker 			fWriteLock;
  LSPReaderThread*	fReaderThread;
  PipeImage			fPipeImage;
};
