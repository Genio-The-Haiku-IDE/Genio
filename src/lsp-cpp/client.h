//
<<<<<<< HEAD
//	Andrea Anzani <andrea.anzani@gmail.com>
// 	Original source code by Alex on 2020/1/28.
=======
// Created by Alex on 2020/1/28.
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
//

#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H
<<<<<<< HEAD

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
=======
#include "transport.h"
#include "protocol.h"



class LanguageClient : public JsonTransport {
public:
    virtual ~LanguageClient() = default;
protected:
		pid_t   childpid;
public:
    RequestID Initialize(option<DocumentUri> rootUri = {}) {
        InitializeParams params;
        params.processId = childpid;
        params.rootUri = rootUri;
        return SendRequest("initialize", params);
    }
    RequestID Shutdown() {
        return SendRequest("shutdown");
    }
    RequestID Sync() {
        return SendRequest("sync");
    }
    void Exit() {
        SendNotify("exit");
    }
    void Initialized() {
        SendNotify("initialized");
    }
    RequestID RegisterCapability() {
        return SendRequest("client/registerCapability");
    }
    void DidOpen(DocumentUri uri, string_ref text, string_ref languageId = "cpp") {
        DidOpenTextDocumentParams params;
        params.textDocument.uri = std::move(uri);
        params.textDocument.text = text;
        params.textDocument.languageId = languageId;
        SendNotify("textDocument/didOpen", params);
    }
    void DidClose(DocumentUri uri) {
        DidCloseTextDocumentParams params;
        params.textDocument.uri = std::move(uri);
        SendNotify("textDocument/didClose", params);
    }
    void DidChange(DocumentUri uri, std::vector<TextDocumentContentChangeEvent> &changes,
                   option<bool> wantDiagnostics = {}) {
        DidChangeTextDocumentParams params;
        params.textDocument.uri = std::move(uri);
        params.contentChanges = std::move(changes);
        params.wantDiagnostics = wantDiagnostics;
        SendNotify("textDocument/didChange", params);
    }
    RequestID RangeFomatting(DocumentUri uri, Range range) {
        DocumentRangeFormattingParams params;
        params.textDocument.uri = std::move(uri);
        params.range = range;
        return SendRequest("textDocument/rangeFormatting", params);
    }
    RequestID FoldingRange(DocumentUri uri) {
        FoldingRangeParams params;
        params.textDocument.uri = std::move(uri);
        return SendRequest("textDocument/foldingRange", params);
    }
    RequestID SelectionRange(DocumentUri uri, std::vector<Position> &positions) {
        SelectionRangeParams params;
        params.textDocument.uri = std::move(uri);
        params.positions = std::move(positions);
        return SendRequest("textDocument/selectionRange", params);
    }
    RequestID OnTypeFormatting(DocumentUri uri, Position position, string_ref ch) {
        DocumentOnTypeFormattingParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        params.ch = std::move(ch);
        return SendRequest("textDocument/onTypeFormatting", std::move(params));
    }
    RequestID Formatting(DocumentUri uri) {
        DocumentFormattingParams params;
        params.textDocument.uri = std::move(uri);
        return SendRequest("textDocument/formatting", std::move(params));
    }
    RequestID CodeAction(DocumentUri uri, Range range, CodeActionContext context) {
        CodeActionParams params;
        params.textDocument.uri = std::move(uri);
        params.range = range;
        params.context = std::move(context);
        return SendRequest("textDocument/codeAction", std::move(params));
    }
    RequestID Completion(DocumentUri uri, Position position, option<CompletionContext> context = {}) {
        CompletionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        params.context = context;
        return SendRequest("textDocument/completion", params);
    }
    RequestID SignatureHelp(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/signatureHelp", std::move(params));
    }
    RequestID GoToDefinition(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/definition", std::move(params));
    }
    
    RequestID GoToImplementation(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/implementation", std::move(params));
	}
    
    RequestID GoToDeclaration(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/declaration", std::move(params));
    }
    RequestID References(DocumentUri uri, Position position) {
        ReferenceParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/references", std::move(params));
    }
    RequestID SwitchSourceHeader(DocumentUri uri) {
        TextDocumentIdentifier params;
        params.uri = std::move(uri);
        return SendRequest("textDocument/switchSourceHeader", std::move(params));
    }
    RequestID Rename(DocumentUri uri, Position position, string_ref newName) {
        RenameParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        params.newName = newName;
        return SendRequest("textDocument/rename", std::move(params));
    }
    RequestID Hover(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/hover", std::move(params));
    }
    RequestID DocumentSymbol(DocumentUri uri) {
        DocumentSymbolParams params;
        params.textDocument.uri = std::move(uri);
        return SendRequest("textDocument/documentSymbol", std::move(params));
    }
    RequestID DocumentColor(DocumentUri uri) {
        DocumentSymbolParams params;
        params.textDocument.uri = std::move(uri);
        return SendRequest("textDocument/documentColor", std::move(params));
    }
    RequestID DocumentHighlight(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/documentHighlight", std::move(params));
    }
    RequestID SymbolInfo(DocumentUri uri, Position position) {
        TextDocumentPositionParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        return SendRequest("textDocument/symbolInfo", std::move(params));
    }
    RequestID TypeHierarchy(DocumentUri uri, Position position, TypeHierarchyDirection direction, int resolve) {
        TypeHierarchyParams params;
        params.textDocument.uri = std::move(uri);
        params.position = position;
        params.direction = direction;
        params.resolve = resolve;
        return SendRequest("textDocument/typeHierarchy", std::move(params));
    }
    RequestID WorkspaceSymbol(string_ref query) {
        WorkspaceSymbolParams params;
        params.query = query;
        return SendRequest("workspace/symbol", std::move(params));
    }
    RequestID ExecuteCommand(string_ref cmd, option<TweakArgs> tweakArgs = {}, option<WorkspaceEdit> workspaceEdit = {}) {
        ExecuteCommandParams params;
        params.tweakArgs = tweakArgs;
        params.workspaceEdit = workspaceEdit;
        params.command = cmd;
        return SendRequest("workspace/executeCommand", std::move(params));
    }
    RequestID DidChangeWatchedFiles(std::vector<FileEvent> &changes) {
        DidChangeWatchedFilesParams params;
        params.changes = std::move(changes);
        return SendRequest("workspace/didChangeWatchedFiles", std::move(params));
    }
    RequestID DidChangeConfiguration(ConfigurationSettings &settings) {
        DidChangeConfigurationParams params;
        params.settings = std::move(settings);
        return SendRequest("workspace/didChangeConfiguration", std::move(params));
    }

public:
    RequestID SendRequest(string_ref method, value params = json()) {
        RequestID id = method.str();
        request(method, params, id);
        return id;
    }
    void SendNotify(string_ref method, value params = json()) {
        notify(method, params);
    }
};

#define READ_END  0
#define WRITE_END 1
#include <unistd.h>

class ProcessLanguageClient : public LanguageClient {
public:
        int     outPipe[2], inPipe[2];
        explicit ProcessLanguageClient(){};
        
<<<<<<< HEAD
    explicit ProcessLanguageClient(const char *program, const char *arguments = "") {
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
        void Init(const char *program, const char *arguments = "") {
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)

		pipe(outPipe);
		pipe(inPipe);
		
		if((childpid = fork()) == -1)
        {
                perror("fork");
                exit(1);
        }

<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
        if (childpid == 0) {
	        
          setpgid(childpid, childpid);
          // close(outPipe[READ_END]);
          close(outPipe[WRITE_END]);
          close(inPipe[READ_END]);
          // close(inPipe[WRITE_END]);
<<<<<<< HEAD

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
=======
        if(childpid == 0)
        {
				//close(outPipe[READ_END]);
				close(outPipe[WRITE_END]);
				close(inPipe[READ_END]);
				//close(inPipe[WRITE_END]);				
				
				dup2(inPipe[WRITE_END], STDOUT_FILENO);
				close(inPipe[WRITE_END]);
				dup2(outPipe[READ_END], STDIN_FILENO);
				close(outPipe[READ_END]);
				execlp(program, program, "--log=verbose","--offset-encoding=utf-8","--pretty", NULL);
<<<<<<<< HEAD:src/lsp-cpp/include/client.h
				fprintf(stderr, "ERROR in exec\n");
				sleep(2);
				//printf("{\"id\": \"initialize\",\"jsonrpc\": \"2.0\", \"result\":\"error\" }");
				//while(true) { sleep(1);}
========
				
				//execlp("clangdx", "clangdx", "--log=error","--offset-encoding=utf-8","--pretty", NULL);
				//attempt to provide a fallback in case clangd is not available.
>>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position):src/lsp-cpp/client.h
				json response;
				response["id"] = "initialize";
				response["jsonrpc"] = "2.0";
				response["result"] = "error";
				
				std::string content = response.dump();
				std::string header = "Content-Length: " + std::to_string(content.length()) + "\r\n\r\n" + content;
				//if (VERBOSE)
				//	fprintf(stderr, "Client: - snd \n%s\n", content.c_str());
				int hasWritten;
				int writeSize = 0;
				int totalSize = header.length();
				while (( hasWritten = write(inPipe[WRITE_END], &header[writeSize],totalSize)) != -1) {
					writeSize += hasWritten;
					if (writeSize >= totalSize) {
						break;
					}
				}
				fprintf(stderr,"FORK - [%s]\n", header.c_str());
				//printf("%s",header.c_str()); 
				while(true) { sleep(1);}
                exit(0);
        }
		else
		{
				close(outPipe[READ_END]);
				//close(outPipe[WRITE_END]);
				//close(inPipe[READ_END]);
				close(inPipe[WRITE_END]);
		}
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)

          dup2(inPipe[WRITE_END], STDOUT_FILENO);
          close(inPipe[WRITE_END]);
          dup2(outPipe[READ_END], STDIN_FILENO);
          close(outPipe[READ_END]);
          execlp(program, program, "--log=verbose", "--offset-encoding=utf-8", "--pretty", NULL);
          exit(1);
        } else {
	        
          close(outPipe[READ_END]);
          // close(outPipe[WRITE_END]);
          // close(inPipe[READ_END]);
          close(inPipe[WRITE_END]);
        }
    }
    ~ProcessLanguageClient() override {
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
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
<<<<<<< HEAD
<<<<<<< HEAD
			if (hasRead == 0 || length >= 254) // pipe eof or protection
				return 0;
				
=======
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
			if (hasRead == 0 || length >= 254) // pipe eof or protection
				return 0;
				
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
			if (szReadBuffer[length] == '\n') {
                break;
            }
            length++;
		}
		return atoi(szReadBuffer + 16);
    }
<<<<<<< HEAD
<<<<<<< HEAD
    int Read(int length, std::string &out) {
=======
    void Read(int length, std::string &out) {
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
    int Read(int length, std::string &out) {
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
		int readSize = 0;
		int hasRead;
		out.resize(length);
		while (( hasRead = read(inPipe[READ_END], &out[readSize],length)) != -1) {
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
			
			if (hasRead == 0) // pipe eof
				return 0;
				
<<<<<<< HEAD
=======
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
            readSize += hasRead;
            if (readSize >= length) {
                break;
            }
        }
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
        
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
<<<<<<< HEAD
=======
    }
    bool Write(std::string &in) {
		int hasWritten;
		int writeSize = 0;
        int totalSize = in.length();
        while (( hasWritten = write(outPipe[WRITE_END], &in[writeSize],totalSize)) != -1) {
            writeSize += hasWritten;
            if (writeSize >= totalSize) {
                break;
            }
        }
        return true;
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
    }
    bool readJson(json &json) override {
        json.clear();
        int length = ReadLength();
<<<<<<< HEAD
<<<<<<< HEAD
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
=======
=======
        if (length == 0)
	        return false;
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
        SkipLine();
        std::string read;
        if (Read(length, read) == 0)
	        return false;
        try {
            json = json::parse(read);
        } catch (std::exception &e) {
<<<<<<< HEAD
            //printf("read error -> %s\nread -> %s\n ", e.what(), read.c_str());
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
           return false;
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
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
<<<<<<< HEAD
<<<<<<< HEAD
    

    
    private:
		BLocker	writeLock;
=======
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
    
    private:
		BLocker	writeLock;
>>>>>>> 677ef97 (improved read and write from  pipe to handle eof)
};

#endif //LSP_CLIENT_H
