/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "LSPClientWrapper.h"
#include "Log.h"
#include <map>
#include "LSPTextDocument.h"
#include "protocol.h"


LSPClientWrapper::LSPClientWrapper()
{
	fInitialized.store(false);
}

#define X(A) std::to_string((size_t)A)
void
LSPClientWrapper::RegisterTextDocument(LSPTextDocument* fw)
{
	//protect access? who should call this?
	fTextDocs[X(fw)] = fw;
}

void
LSPClientWrapper::UnregisterTextDocument(LSPTextDocument* fw)
{
	//protect access? who should call this?
	if (fTextDocs.find(X(fw)) != fTextDocs.end())
		fTextDocs.erase(X(fw));
}

bool	
LSPClientWrapper::Create(const char *uri)
{
  fRootURI = uri;
  /** configuration for clangd */
  std::string logLevel("--log=");
  switch (Logger::Level())
  {
  		case LOG_LEVEL_UNSET:
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
  const char* argv[] = { "clangd", 
						 logLevel.c_str(), 
						 "--offset-encoding=utf-8", 
						 "--pretty", 
						 "--header-insertion-decorators=false",
						 NULL
						};
  
  if (Init(argv) != B_OK) {
	  //TODO: show an alert to the user. (but only once per session!)
	  LogInfo("Can't execute clangd tool to provide advanced features! Please install llvm12 package.");
	  return false;
  }
  std::atomic<bool> on_error;
  on_error.store(false);
  fReaderThread = std::thread([&] {
	if (loop(*this) == -1)
	{
		on_error.store(true);
		fInitialized.store(false);
		Close();
	}
  });
  
  Initialize(string_ref(uri));

  while (!fInitialized.load() && !on_error.load()) {
    LogDebug("Waiting for clangd initialization.. \n");
    usleep(500000);
  }
  return on_error.load();
}

bool	
LSPClientWrapper::Dispose()
{
	if (!fInitialized) {
		if (fReaderThread.joinable())
			 fReaderThread.detach();
    	return true;
    }

    for(auto& m: fTextDocs)
		LogError("LSPClientWrapper::Dispose() still textDocument registered! [%s]", m.second->GetFilenameURI().c_str());
    
	Shutdown();
	
	while (fInitialized.load()) {
		LogDebug("Waiting for shutdown...\n");
		usleep(500000);
	}
	
  	fReaderThread.detach();
  	Exit();
	return true;
}
		
LSPTextDocument*	
LSPClientWrapper::_DocumentByURI(const std::string& uri)
{
	LSPTextDocument* doc = nullptr;
	for (auto& x: fTextDocs) {
		if(x.second->GetFilenameURI().compare(uri) == 0)
		{
			doc = x.second;
			break;
		}	
	}
	
	return doc;
}

void 
LSPClientWrapper::onNotify(std::string method, value &params)
{
	if (method.compare("textDocument/publishDiagnostics") == 0 ||
		method.compare("textDocument/clangd.fileStatus")  == 0 )
	{		
		auto uri = params["uri"].get<std::string>();
		LSPTextDocument* doc = _DocumentByURI(uri);		
		if (doc)
			doc->onNotify(method, params);
			
		return;
	}
	
	LogError("LSPClientWrapper::onNotify not implemented! [%s]", method.c_str());
}

void
LSPClientWrapper::onResponse(RequestID id, value &result)
{
	std::size_t found = id.find('_');
	std::string	key;
	if (found != std::string::npos) {
		key = id.substr(0, found);
		id = id.substr(found + 1);
	}
	
	if (id.compare("initialize") == 0)
	{
		fInitialized.store(true); 
		Initialized(result);
		return;
	}
	if (id.compare("shutdown") == 0)
	{
       fprintf(stderr, "Shutdown received\n");
       fInitialized.store(false);
       return; 
	}
	
	auto search = fTextDocs.find(key);
	if (search != fTextDocs.end())
        search->second->onResponse(id, result);
    else    
		LogError("LSPClientWrapper::onResponse not handled! [%s][%s] for [%s]", key.c_str(), id.c_str(), key.c_str());
}

void 
LSPClientWrapper::onError(RequestID id, value &error)
{
	std::size_t found = id.find('_');
	std::string	key;
	if (found != std::string::npos) {
		key = id.substr(0, found);
		id = id.substr(found + 1);
	}
	
	auto search = fTextDocs.find(key);
	if (search != fTextDocs.end())
        search->second->onError(id, error);
    else    
		LogError("LSPClientWrapper::onError not handled! [%s][%s] for [%s]", key.c_str(), id.c_str(), key.c_str());
}

void 
LSPClientWrapper::onRequest(std::string method, value &params, value &ID)
{
	LogError("LSPClientWrapper::onRequest not implemented! [%s] [%s]", method.c_str(), ID.dump().c_str());
}

RequestID
LSPClientWrapper::Initialize(option<DocumentUri> rootUri)
{
	InitializeParams params;
	params.processId = GetChildPid();
	params.rootUri = rootUri;
	return SendRequest("client", "initialize", params);
}

RequestID
LSPClientWrapper::Shutdown()
{
	return SendRequest("client", "shutdown");
}

RequestID
LSPClientWrapper::Sync()
{
	return SendRequest("client", "sync");
}

void
LSPClientWrapper::Exit()
{
	SendNotify("exit");
}

void
LSPClientWrapper::Initialized(json& result)
{
	auto &capas = result["capabilities"];
	if (capas != value::value_t::null) {
		auto& completionProvider = capas["completionProvider"];
		if (completionProvider != value::value_t::null) {
			auto& allCommitCharacters = completionProvider["allCommitCharacters"];
			if (allCommitCharacters != value::value_t::null) {
				fAllCommitCharacters.clear();
				for (auto &c : allCommitCharacters)
				{
					fAllCommitCharacters.append(c.get<std::string>().c_str());
				}
				LogDebug("allCommitCharacters [%s]", this->allCommitCharacters().c_str());
			}
			auto& triggerCharacters = completionProvider["triggerCharacters"];
			if (triggerCharacters != value::value_t::null) {
				fTriggerCharacters.clear();
				for (auto &c : triggerCharacters)
				{
					fTriggerCharacters.append(c.get<std::string>().c_str());
				}
				LogDebug("triggerCharacters [%s]", this->triggerCharacters().c_str());
			}
		}
	}
	
	SendNotify("initialized");
}

RequestID
LSPClientWrapper::RegisterCapability()
{ //?
	return SendRequest("client/registerCapability", "client/registerCapability");
}

void
LSPClientWrapper::DidOpen(MessageHandler* fw, DocumentUri uri, string_ref text, string_ref languageId)
{
	DidOpenTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	params.textDocument.text = text;
	params.textDocument.languageId = languageId;
	SendNotify("textDocument/didOpen", params);
}

void
LSPClientWrapper::DidClose(MessageHandler* fw, DocumentUri uri)
{
	DidCloseTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	SendNotify("textDocument/didClose", params);
}

void
LSPClientWrapper::DidChange(MessageHandler* fw, DocumentUri uri, std::vector<TextDocumentContentChangeEvent> &changes,
			   option<bool> wantDiagnostics)
{
	DidChangeTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	params.contentChanges = std::move(changes);
	//params.wantDiagnostics = wantDiagnostics;
	SendNotify("textDocument/didChange", params);
}

//xed
void
LSPClientWrapper::DidSave(MessageHandler* fw, DocumentUri uri)
{
	DidSaveTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	SendNotify("textDocument/didSave", params);
}

RequestID
LSPClientWrapper::RangeFomatting(MessageHandler* fw, DocumentUri uri, Range range)
{
	DocumentRangeFormattingParams params;
	params.textDocument.uri = std::move(uri);
	params.range = range;
	return SendRequest(X(fw), "textDocument/rangeFormatting", params);
}

RequestID
LSPClientWrapper::FoldingRange(MessageHandler* fw, DocumentUri uri)
{
	FoldingRangeParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/foldingRange", params);
}

RequestID
LSPClientWrapper::SelectionRange(MessageHandler* fw, DocumentUri uri, std::vector<Position> &positions)
{
	SelectionRangeParams params;
	params.textDocument.uri = std::move(uri);
	params.positions = std::move(positions);
	return SendRequest(X(fw), "textDocument/selectionRange", params);
}

RequestID
LSPClientWrapper::OnTypeFormatting(MessageHandler* fw, DocumentUri uri, Position position, string_ref ch)
{
	DocumentOnTypeFormattingParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.ch = std::move(ch);
	return SendRequest(X(fw), "textDocument/onTypeFormatting", std::move(params));
}

RequestID
LSPClientWrapper::Formatting(MessageHandler* fw, DocumentUri uri)
{
	DocumentFormattingParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/formatting", std::move(params));
}

RequestID
LSPClientWrapper::CodeAction(MessageHandler* fw, DocumentUri uri, Range range, CodeActionContext context)
{
	CodeActionParams params;
	params.textDocument.uri = std::move(uri);
	params.range = range;
	params.context = std::move(context);
	return SendRequest(X(fw), "textDocument/codeAction", std::move(params));
}

RequestID
LSPClientWrapper::Completion(MessageHandler* fw, DocumentUri uri, Position position, CompletionContext& context)
{
	CompletionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.context = option<CompletionContext>(context);
	return SendRequest(X(fw), "textDocument/completion", params);
}

RequestID
LSPClientWrapper::SignatureHelp(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/signatureHelp", std::move(params));
}

RequestID
LSPClientWrapper::GoToDefinition(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/definition", std::move(params));
}

RequestID
LSPClientWrapper::GoToImplementation(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/implementation", std::move(params));
}

RequestID
LSPClientWrapper::GoToDeclaration(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/declaration", std::move(params));
}

RequestID
LSPClientWrapper::References(MessageHandler* fw, DocumentUri uri, Position position)
{
	ReferenceParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/references", std::move(params));
}

RequestID
LSPClientWrapper::SwitchSourceHeader(MessageHandler* fw, DocumentUri uri)
{
	TextDocumentIdentifier params;
	params.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/switchSourceHeader", std::move(params));
}

RequestID
LSPClientWrapper::Rename(MessageHandler* fw, DocumentUri uri, Position position, string_ref newName)
{
	RenameParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.newName = newName;
	return SendRequest(X(fw), "textDocument/rename", std::move(params));
}

RequestID
LSPClientWrapper::Hover(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/hover", std::move(params));
}

RequestID
LSPClientWrapper::DocumentSymbol(MessageHandler* fw, DocumentUri uri)
{
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/documentSymbol", std::move(params));
}

RequestID
LSPClientWrapper::DocumentColor(MessageHandler* fw, DocumentUri uri)
{
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/documentColor", std::move(params));
}

RequestID
LSPClientWrapper::DocumentHighlight(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/documentHighlight", std::move(params));
}

RequestID
LSPClientWrapper::SymbolInfo(MessageHandler* fw, DocumentUri uri, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/symbolInfo", std::move(params));
}

RequestID
LSPClientWrapper::TypeHierarchy(MessageHandler* fw, DocumentUri uri, Position position, TypeHierarchyDirection direction, int resolve)
{
	TypeHierarchyParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.direction = direction;
	params.resolve = resolve;
	return SendRequest(X(fw), "textDocument/typeHierarchy", std::move(params));
}

RequestID
LSPClientWrapper::DocumentLink(MessageHandler* fw, DocumentUri uri)
{
	DocumentLinkParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/documentLink", std::move(params));
}

RequestID
LSPClientWrapper::WorkspaceSymbol(MessageHandler* fw, string_ref query)
{
	WorkspaceSymbolParams params;
	params.query = query;
	return SendRequest(X(fw), "workspace/symbol", std::move(params));
}
/*
RequestID
LSPClientWrapper::ExecuteCommand(MessageHandler* fw, string_ref cmd, option<TweakArgs> tweakArgs, option<WorkspaceEdit> workspaceEdit)
{
	ExecuteCommandParams params;
	params.tweakArgs = tweakArgs;
	params.workspaceEdit = workspaceEdit;
	params.command = cmd;
	return SendRequest(X(fw), "workspace/executeCommand", std::move(params));
}*/

RequestID
LSPClientWrapper::DidChangeWatchedFiles(MessageHandler* fw, std::vector<FileEvent> &changes)
{
	DidChangeWatchedFilesParams params;
	params.changes = std::move(changes);
	return SendRequest(X(fw), "workspace/didChangeWatchedFiles", std::move(params));
}

RequestID
LSPClientWrapper::DidChangeConfiguration(MessageHandler* fw, ConfigurationSettings &settings)
{
	DidChangeConfigurationParams params;
	params.settings = std::move(settings);
	return SendRequest(X(fw), "workspace/didChangeConfiguration", std::move(params));
}

RequestID
LSPClientWrapper::SendRequest(RequestID id, string_ref method, value params)
{
	id.append("_").append(method);
	request(method, params, id);
	return id;
}

void
LSPClientWrapper::SendNotify(string_ref method, value params)
{
	notify(method, params);
}
