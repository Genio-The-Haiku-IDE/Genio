/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <unistd.h>

#include "Transport.h" //JsonTransport
#include "json.hpp"

#include <Locker.h>


class LSPClient : public JsonTransport {
	
public:
  explicit LSPClient();

  status_t Init(const char *argv[]);
  virtual ~LSPClient();
  void	Close();

  void SkipLine();
  int ReadLength();
  int Read(int length, std::string &out);

  bool Write(std::string &in);
  bool readJson(json &json) override;
  bool writeJson(json &json) override;

  pid_t GetChildPid();
  
  status_t InterruptExternal();

private:
  BLocker fWriteLock;
  pid_t fChildpid;
  int fOutPipe[2];
  int fInPipe[2];
};

#endif //LSP_CLIENT_H
