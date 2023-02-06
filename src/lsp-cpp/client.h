//
//	Andrea Anzani 
// 	Original source code by Alex on 2020/1/28.
//

#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include <unistd.h>

#include "transport.h"
#include "protocol.h"
#include "Log.h"

#define READ_END  0
#define WRITE_END 1


class ProcessLanguageClient : public JsonTransport {
	
public:
		pid_t   childpid;
        int     outPipe[2], inPipe[2];
        explicit ProcessLanguageClient(){};
        
        void Init(const char *program, const char *arguments = "") {

		pipe(outPipe);
		pipe(inPipe);
		
		if((childpid = fork()) == -1)
        {
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

          std::string logLevel("--log=");
          switch (Logger::Level())
          {
				case LOG_LEVEL_OFF:
				case LOG_LEVEL_ERROR:
					logLevel += "error"; // Error messages only
					break;
				case LOG_LEVEL_INFO:
					logLevel += "info";  // High level execution tracing
					break;
				case LOG_LEVEL_DEBUG:
				case LOG_LEVEL_TRACE:
					logLevel += "verbose"; // Low level details
					break;
	      };
		  execlp(program, program, logLevel.c_str(), "--offset-encoding=utf-8", "--pretty", NULL);
          exit(1);
        } else {
	        
          close(outPipe[READ_END]);
          // close(outPipe[WRITE_END]);
          // close(inPipe[READ_END]);
          close(inPipe[WRITE_END]);
        }
    }
    virtual ~ProcessLanguageClient()  {
		close(outPipe[WRITE_END]);
		close(inPipe[READ_END]);
    }
    void SkipLine() {
		char xread;
		while ( read(inPipe[READ_END], &xread, 1)) {
			if (xread == '\n') {
				break;
			}
		}
    }
    int ReadLength() {
		char szReadBuffer[255];
		int hasRead = 0;
		int length  = 0;
		while ( ( hasRead = read(inPipe[READ_END], &szReadBuffer[length], 1)) != -1)
		{
			if (hasRead == 0 || length >= 254) // pipe eof or protection
				return 0;
				
			if (szReadBuffer[length] == '\n') {
                break;
            }
            length++;
		}
		return atoi(szReadBuffer + 16);
    }
    int Read(int length, std::string &out) {
		int readSize = 0;
		int hasRead;
		out.resize(length);
		while (( hasRead = read(inPipe[READ_END], &out[readSize],length)) != -1) {
			
			if (hasRead == 0) // pipe eof
				return 0;
				
            readSize += hasRead;
            if (readSize >= length) {
                break;
            }
        }
        
        return readSize;
    }
    #include <Locker.h>
    bool Write(std::string &in) {
	    
		int hasWritten = 0;
		int writeSize = 0;
        int totalSize = in.length();        
        
        if (writeLock.Lock()) // for production code: WithTimeout(1000000) == B_OK)
	    {
			while (( hasWritten = write(outPipe[WRITE_END], &in[writeSize],totalSize)) != -1) {
				writeSize += hasWritten;
				if (writeSize >= totalSize || hasWritten == 0) {
					break;
				}
			}
			writeLock.Unlock();
        }
        return (hasWritten != 0);
    }
    bool readJson(json &json) override {
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
        if (VERBOSE)
	        fprintf(stderr, "Client - rcv %d:\n%s\n", length, read.c_str());
        return true;
    }
    bool writeJson(json &json) override {
        std::string content = json.dump();
        std::string header = "Content-Length: " + std::to_string(content.length()) + "\r\n\r\n" + content;
        if (VERBOSE)
	        fprintf(stderr, "Client: - snd \n%s\n", content.c_str());
        return Write(header);
    }
    

    
    private:
		BLocker	writeLock;
};

#endif //LSP_CLIENT_H
