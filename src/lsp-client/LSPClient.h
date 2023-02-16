//
//	Andrea Anzani 
//

#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <unistd.h>

#include "Transport.h" //JsonTransport
#include "protocol.h"  // ? json_frw?

#include <Locker.h>


class LSPClient : public JsonTransport {
	
public:
  explicit LSPClient();

  void Init(char *argv[]);
  virtual ~LSPClient();

  void SkipLine();
  int ReadLength();
  int Read(int length, std::string &out);

  bool Write(std::string &in);
  bool readJson(json &json) override;
  bool writeJson(json &json) override;

  pid_t GetChildPid();

private:
  BLocker writeLock;
  pid_t childpid;
  int outPipe[2], inPipe[2];
};

#endif //LSP_CLIENT_H
