#include "LSPClient.h"
#include "Log.h"

#define READ_END 0
#define WRITE_END 1

void LSPClient::Init(char *argv[]) {

  pipe(outPipe);
  pipe(inPipe);

  if ((childpid = fork()) == -1) {
    perror("fork");
    exit(1);
  }

  if (childpid == 0) {

    setpgid(childpid, childpid);
    // close(outPipe[READ_END]);
    close(outPipe[WRITE_END]);
    close(inPipe[READ_END]);
    // close(inPipe[WRITE_END]);

    dup2(inPipe[WRITE_END], STDOUT_FILENO);
    close(inPipe[WRITE_END]);
    dup2(outPipe[READ_END], STDIN_FILENO);
    close(outPipe[READ_END]);

    execvp(argv[0], argv);
    exit(1);
  } else {

    close(outPipe[READ_END]);
    // close(outPipe[WRITE_END]);
    // close(inPipe[READ_END]);
    close(inPipe[WRITE_END]);
  }
}

LSPClient::~LSPClient() {
  close(outPipe[WRITE_END]);
  close(inPipe[READ_END]);
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
