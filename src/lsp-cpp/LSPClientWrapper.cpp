/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "LSPClientWrapper.h"
#include "Log.h"
#include <map>
#include "LSPTextDocument.h"




LSPClientWrapper::LSPClientWrapper()
{
	initialized.store(false);
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
  client = new ProcessLanguageClient();
  client->Init("clangd");

  std::atomic<bool> on_error;
  on_error.store(false);
  readerThread = std::thread([&] {
	if (client->loop(*this) == -1)
	{
		on_error.store(true);
		initialized.store(false);
	}
  });
  
  rootURI = uri;
  Initialize(rootURI);

  while (!initialized.load() && !on_error.load()) {
    LogDebug("Waiting for clangd initialization.. \n");
    usleep(500000);
  }
  return on_error.load();
}

bool	
LSPClientWrapper::Dispose()
{
	if (!initialized)
    	return true;

    for(auto& m: fTextDocs)
		LogError("LSPClientWrapper::Dispose() still textDocument registered! [%s]", m.second->GetFilenameURI().c_str());
    
	Shutdown();
	
	while (initialized.load()) {
		LogDebug("Waiting for shutdown...\n");
		usleep(500000);
	}
	
  	readerThread.detach();
  	Exit();
  	delete client;
  	client = NULL;
	return true;
}
		
		

void 
LSPClientWrapper::onNotify(std::string method, value &params)
{
	if (method.compare("textDocument/publishDiagnostics") == 0)
	{
		LSPTextDocument* doc = NULL;
		auto uri = params["uri"].get<std::string>();
		for (auto& x: fTextDocs) {
			//LogDebug("comparing [%s] vs [%s]", uri.c_str(), x.second->GetFilenameURI().c_str());
			if(x.second->GetFilenameURI().compare(uri) == 0)
			{
				doc = x.second;
				break;
			}	
		}
		
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
		initialized.store(true); 
		Initialized();
		return;
	}
	if (id.compare("shutdown") == 0)
	{
       fprintf(stderr, "Shutdown received\n");
       initialized.store(false);
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


RequestID LSPClientWrapper::Initialize(option<DocumentUri> rootUri) {
	InitializeParams params;
	params.processId = client->childpid;
	params.rootUri = rootUri;
	return SendRequest("client", "initialize", params);
}
RequestID LSPClientWrapper::Shutdown() {
	return SendRequest("client", "shutdown");
}
RequestID LSPClientWrapper::Sync() {
	return SendRequest("client", "sync");
}
void LSPClientWrapper::Exit() {
	SendNotify("exit");
}
void LSPClientWrapper::Initialized() {
	SendNotify("initialized");
}
RequestID LSPClientWrapper::RegisterCapability() { //?
	return SendRequest("client/registerCapability", "client/registerCapability");
}
void LSPClientWrapper::DidOpen(MessageHandler* fw, DocumentUri uri, string_ref text, string_ref languageId) {
	DidOpenTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	params.textDocument.text = text;
	params.textDocument.languageId = languageId;
	SendNotify("textDocument/didOpen", params);
}
void LSPClientWrapper::DidClose(MessageHandler* fw, DocumentUri uri) {
	DidCloseTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	SendNotify("textDocument/didClose", params);
}
void LSPClientWrapper::DidChange(MessageHandler* fw, DocumentUri uri, std::vector<TextDocumentContentChangeEvent> &changes,
			   option<bool> wantDiagnostics) {
	DidChangeTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	params.contentChanges = std::move(changes);
	//params.wantDiagnostics = wantDiagnostics;
	SendNotify("textDocument/didChange", params);
}
//xed
void LSPClientWrapper::DidSave(MessageHandler* fw, DocumentUri uri) {
	DidSaveTextDocumentParams params;
	params.textDocument.uri = std::move(uri);
	SendNotify("textDocument/didSave", params);
}

RequestID LSPClientWrapper::RangeFomatting(MessageHandler* fw, DocumentUri uri, Range range) {
	DocumentRangeFormattingParams params;
	params.textDocument.uri = std::move(uri);
	params.range = range;
	return SendRequest(X(fw), "textDocument/rangeFormatting", params);
}
RequestID LSPClientWrapper::FoldingRange(MessageHandler* fw, DocumentUri uri) {
	FoldingRangeParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/foldingRange", params);
}
RequestID LSPClientWrapper::SelectionRange(MessageHandler* fw, DocumentUri uri, std::vector<Position> &positions) {
	SelectionRangeParams params;
	params.textDocument.uri = std::move(uri);
	params.positions = std::move(positions);
	return SendRequest(X(fw), "textDocument/selectionRange", params);
}
RequestID LSPClientWrapper::OnTypeFormatting(MessageHandler* fw, DocumentUri uri, Position position, string_ref ch) {
	DocumentOnTypeFormattingParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.ch = std::move(ch);
	return SendRequest(X(fw), "textDocument/onTypeFormatting", std::move(params));
}
RequestID LSPClientWrapper::Formatting(MessageHandler* fw, DocumentUri uri) {
	DocumentFormattingParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/formatting", std::move(params));
}
RequestID LSPClientWrapper::CodeAction(MessageHandler* fw, DocumentUri uri, Range range, CodeActionContext context) {
	CodeActionParams params;
	params.textDocument.uri = std::move(uri);
	params.range = range;
	params.context = std::move(context);
	return SendRequest(X(fw), "textDocument/codeAction", std::move(params));
}
RequestID LSPClientWrapper::Completion(MessageHandler* fw, DocumentUri uri, Position position, option<CompletionContext> context) {
	CompletionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.context = context;
	return SendRequest(X(fw), "textDocument/completion", params);
}
RequestID LSPClientWrapper::SignatureHelp(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/signatureHelp", std::move(params));
}
RequestID LSPClientWrapper::GoToDefinition(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/definition", std::move(params));
}

RequestID LSPClientWrapper::GoToImplementation(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/implementation", std::move(params));
}

RequestID LSPClientWrapper::GoToDeclaration(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/declaration", std::move(params));
}
RequestID LSPClientWrapper::References(MessageHandler* fw, DocumentUri uri, Position position) {
	ReferenceParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/references", std::move(params));
}
RequestID LSPClientWrapper::SwitchSourceHeader(MessageHandler* fw, DocumentUri uri) {
	TextDocumentIdentifier params;
	params.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/switchSourceHeader", std::move(params));
}
RequestID LSPClientWrapper::Rename(MessageHandler* fw, DocumentUri uri, Position position, string_ref newName) {
	RenameParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.newName = newName;
	return SendRequest(X(fw), "textDocument/rename", std::move(params));
}
RequestID LSPClientWrapper::Hover(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/hover", std::move(params));
}
RequestID LSPClientWrapper::DocumentSymbol(MessageHandler* fw, DocumentUri uri) {
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/documentSymbol", std::move(params));
}
RequestID LSPClientWrapper::DocumentColor(MessageHandler* fw, DocumentUri uri) {
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(uri);
	return SendRequest(X(fw), "textDocument/documentColor", std::move(params));
}
RequestID LSPClientWrapper::DocumentHighlight(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/documentHighlight", std::move(params));
}
RequestID LSPClientWrapper::SymbolInfo(MessageHandler* fw, DocumentUri uri, Position position) {
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	return SendRequest(X(fw), "textDocument/symbolInfo", std::move(params));
}
RequestID LSPClientWrapper::TypeHierarchy(MessageHandler* fw, DocumentUri uri, Position position, TypeHierarchyDirection direction, int resolve) {
	TypeHierarchyParams params;
	params.textDocument.uri = std::move(uri);
	params.position = position;
	params.direction = direction;
	params.resolve = resolve;
	return SendRequest(X(fw), "textDocument/typeHierarchy", std::move(params));
}
RequestID LSPClientWrapper::WorkspaceSymbol(MessageHandler* fw, string_ref query) {
	WorkspaceSymbolParams params;
	params.query = query;
	return SendRequest(X(fw), "workspace/symbol", std::move(params));
}
RequestID LSPClientWrapper::ExecuteCommand(MessageHandler* fw, string_ref cmd, option<TweakArgs> tweakArgs, option<WorkspaceEdit> workspaceEdit) {
	ExecuteCommandParams params;
	params.tweakArgs = tweakArgs;
	params.workspaceEdit = workspaceEdit;
	params.command = cmd;
	return SendRequest(X(fw), "workspace/executeCommand", std::move(params));
}
RequestID LSPClientWrapper::DidChangeWatchedFiles(MessageHandler* fw, std::vector<FileEvent> &changes) {
	DidChangeWatchedFilesParams params;
	params.changes = std::move(changes);
	return SendRequest(X(fw), "workspace/didChangeWatchedFiles", std::move(params));
}
RequestID LSPClientWrapper::DidChangeConfiguration(MessageHandler* fw, ConfigurationSettings &settings) {
	DidChangeConfigurationParams params;
	params.settings = std::move(settings);
	return SendRequest(X(fw), "workspace/didChangeConfiguration", std::move(params));
}

RequestID LSPClientWrapper::SendRequest(RequestID id, string_ref method, value params) {
	id.append("_").append(method);
	client->request(method, params, id);
	return id;
}

void LSPClientWrapper::SendNotify(string_ref method, value params) {
	client->notify(method, params);
}