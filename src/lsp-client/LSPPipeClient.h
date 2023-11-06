/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <unistd.h>

#include "Transport.h"
#include <Locker.h>
#include "PipeImage.h"

class LSPReaderThread;

class LSPPipeClient : public AsyncJsonTransport {

public:

			 LSPPipeClient(uint32 what, BMessenger& msgr);
	virtual ~LSPPipeClient();

	status_t Start(const char *argv[], int32 argc);

	void	Close();

	bool 	readMessage(std::string &json) override;
	bool 	writeMessage(std::string &json) override;

	pid_t	GetChildPid();

	void	ForceQuit(); //quite the looper and the kill the thread
	bool	HasQuitBeenRequested();
	void	KillThread();

private:

  void 	SkipLine();
  int 	ReadLength();
  int 	Read(int length, std::string &out);
  bool 	Write(std::string &in);
  void	Quit() override;
  thread_id	Run() override;

  BLocker 			fWriteLock;
  LSPReaderThread*	fReaderThread;
  PipeImage			fPipeImage;
};

#endif //LSP_CLIENT_H
