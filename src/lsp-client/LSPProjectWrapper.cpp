/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "LSPProjectWrapper.h"

#include "GenioCommon.h"
#include "Log.h"
#include "LSPPipeClient.h"
#include "LSPReaderThread.h"
#include "LSPTextDocument.h"
#include "protocol.h"

#include <Url.h>


LSPProjectWrapper::LSPProjectWrapper(BPath rootPath)
{
	BUrl url(rootPath);
	url.SetAuthority("");

	fRootURI = url.UrlString();
	fLSPPipeClient = nullptr;
	fInitialized.store(false);
}


#define X(A) std::to_string((size_t) A)
bool
LSPProjectWrapper::RegisterTextDocument(LSPTextDocument* textDocument)
{
	std::string fileType = Genio::file_type(textDocument->GetFilenameURI().String());
	if (fileType.compare("c++") != 0 && fileType.compare("make") != 0)
		return false;

	if (!fLSPPipeClient)
		_Create();

	// protect access? who should call this?
	fTextDocs[X(textDocument)] = textDocument;
	return true;
}


void
LSPProjectWrapper::UnregisterTextDocument(LSPTextDocument* textDocument)
{
	// protect access? who should call this?
	if (fTextDocs.find(X(textDocument)) != fTextDocs.end())
		fTextDocs.erase(X(textDocument));
}


bool
LSPProjectWrapper::_Create()
{
	fLSPPipeClient = new LSPPipeClient(*((MessageHandler*) this));
	/** configuration for clangd */
	std::string logLevel("--log=");
	switch (Logger::Level()) {
		case LOG_LEVEL_UNSET:
		case LOG_LEVEL_OFF:
		case LOG_LEVEL_ERROR:
			logLevel += "error"; // Error messages only
			break;
		case LOG_LEVEL_INFO:
			logLevel += "info"; // High level execution tracing
			break;
		case LOG_LEVEL_DEBUG:
		case LOG_LEVEL_TRACE:
			logLevel += "verbose"; // Low level details
			break;
	};
	const char* argv[] = {
		"clangd",
		logLevel.c_str(),
		"--offset-encoding=utf-8",
		"--pretty",
		"--header-insertion-decorators=false",
		NULL
	};

	if (fLSPPipeClient->Start(argv, 5) != B_OK) {
		// TODO: show an alert to the user. (but only once per session!)
		LogInfo("Can't execute clangd tool to provide advanced features! Please install llvm12 "
				"package.");
		return false;
	}

	Initialize(string_ref(fRootURI));

	while (!fInitialized.load() && !fLSPPipeClient->HasQuitBeenRequested()) {
		LogDebug("Waiting for clangd initialization.. \n");
		snooze(50000);
	}
	return !fLSPPipeClient->HasQuitBeenRequested();
}


bool
LSPProjectWrapper::Dispose()
{
	if (!fInitialized) {
		if (fLSPPipeClient)
			fLSPPipeClient->ForceQuit();
		return true;
	}

	for (auto& m : fTextDocs)
		LogError("LSPProjectWrapper::Dispose() still textDocument registered! [%s]",
			m.second->GetFilenameURI().String());

	Shutdown();
	Exit();

	int i = 0;
	while (fLSPPipeClient && !fLSPPipeClient->HasQuitBeenRequested() && i++ < 3) {
		snooze(50000);
	}

	// let's force the thread to quit.
	if (fLSPPipeClient) {
		fLSPPipeClient->KillThread();
	}

	return true;
}


LSPTextDocument*
LSPProjectWrapper::_DocumentByURI(const char* uri)
{
	LSPTextDocument* doc = nullptr;
	for (auto& x : fTextDocs) {
		if (x.second->GetFilenameURI().Compare(uri) == 0) {
			doc = x.second;
			break;
		}
	}
	return doc;
}


void
LSPProjectWrapper::onNotify(std::string method, value& params)
{
	if (method.compare("textDocument/publishDiagnostics") == 0
		|| method.compare("textDocument/clangd.fileStatus") == 0) {
		auto uri = params["uri"].get<std::string>();
		LSPTextDocument* doc = _DocumentByURI(uri.c_str());
		if (doc) {
			doc->onNotify(method, params);
		} else {
			LogError(
				"Can't deliver a notify from LSP to %s\n%s\n", uri.c_str(), params.dump().c_str());
		}
		return;
	}
	LogError("LSPProjectWrapper::onNotify not implemented! [%s]", method.c_str());
}


void
LSPProjectWrapper::onResponse(RequestID id, value& result)
{
	std::size_t found = id.find('_');
	std::string key;
	if (found != std::string::npos) {
		key = id.substr(0, found);
		id = id.substr(found + 1);
	}

	if (id.compare("initialize") == 0) {
		fInitialized.store(true);
		Initialized(result);
		return;
	}
	if (id.compare("shutdown") == 0) {
		fprintf(stderr, "Shutdown received\n");
		fInitialized.store(false);
		return;
	}

	auto search = fTextDocs.find(key);
	if (search != fTextDocs.end()) {
		search->second->onResponse(id, result);
	} else {
		LogError("LSPProjectWrapper::onResponse not handled! [%s][%s] for [%s]", key.c_str(),
			id.c_str(), key.c_str());
	}
}


void
LSPProjectWrapper::onError(RequestID id, value& error)
{
	std::size_t found = id.find('_');
	std::string key;
	if (found != std::string::npos) {
		key = id.substr(0, found);
		id = id.substr(found + 1);
	}

	auto search = fTextDocs.find(key);
	if (search != fTextDocs.end())
		search->second->onError(id, error);
	else
		LogError("LSPProjectWrapper::onError not handled! [%s][%s] for [%s]", key.c_str(),
			id.c_str(), key.c_str());
}


void
LSPProjectWrapper::onRequest(std::string method, value& params, value& ID)
{
	LogError("LSPProjectWrapper::onRequest not implemented! [%s] [%s]", method.c_str(),
		ID.dump().c_str());
}


RequestID
LSPProjectWrapper::Initialize(option<DocumentUri> rootUri)
{
	InitializeParams params;
	params.processId = fLSPPipeClient->GetChildPid();
	params.rootUri = rootUri;
	return SendRequest("client", "initialize", params);
}


RequestID
LSPProjectWrapper::Shutdown()
{
	return SendRequest("client", "shutdown", json());
}


RequestID
LSPProjectWrapper::Sync()
{
	return SendRequest("client", "sync", json());
}


void
LSPProjectWrapper::Exit()
{
	SendNotify("exit", json());
}


void
LSPProjectWrapper::Initialized(json& result)
{
	auto& capas = result["capabilities"];
	if (capas != value::value_t::null) {
		auto& completionProvider = capas["completionProvider"];
		if (completionProvider != value::value_t::null) {
			auto& allCommitCharacters = completionProvider["allCommitCharacters"];
			if (allCommitCharacters != value::value_t::null) {
				fAllCommitCharacters.clear();
				for (auto& c : allCommitCharacters) {
					fAllCommitCharacters.append(c.get<std::string>().c_str());
				}
				LogDebug("allCommitCharacters [%s]", this->allCommitCharacters().c_str());
			}
			auto& triggerCharacters = completionProvider["triggerCharacters"];
			if (triggerCharacters != value::value_t::null) {
				fTriggerCharacters.clear();
				for (auto& c : triggerCharacters) {
					fTriggerCharacters.append(c.get<std::string>().c_str());
				}
				LogDebug("triggerCharacters [%s]", this->triggerCharacters().c_str());
			}
		}
	}

	SendNotify("initialized", json());
}


RequestID
LSPProjectWrapper::RegisterCapability()
{
	//?
	return SendRequest("client/registerCapability", "client/registerCapability", json());
}


void
LSPProjectWrapper::DidOpen(LSPTextDocument* textDocument, string_ref text, string_ref languageId)
{
	DidOpenTextDocumentParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.textDocument.text = text;
	params.textDocument.languageId = languageId;
	SendNotify("textDocument/didOpen", params);
}


void
LSPProjectWrapper::DidClose(LSPTextDocument* textDocument)
{
	DidCloseTextDocumentParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	SendNotify("textDocument/didClose", params);
}


void
LSPProjectWrapper::DidChange(LSPTextDocument* textDocument,
	std::vector<TextDocumentContentChangeEvent>& changes, option<bool> wantDiagnostics)
{
	DidChangeTextDocumentParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.contentChanges = std::move(changes);
	// params.wantDiagnostics = wantDiagnostics;
	SendNotify("textDocument/didChange", params);
}


// xed
void
LSPProjectWrapper::DidSave(LSPTextDocument* textDocument)
{
	DidSaveTextDocumentParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	SendNotify("textDocument/didSave", params);
}


RequestID
LSPProjectWrapper::RangeFomatting(LSPTextDocument* textDocument, Range range)
{
	DocumentRangeFormattingParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.range = range;
	return SendRequest(X(textDocument), "textDocument/rangeFormatting", params);
}


RequestID
LSPProjectWrapper::FoldingRange(LSPTextDocument* textDocument)
{
	FoldingRangeParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/foldingRange", params);
}


RequestID
LSPProjectWrapper::SelectionRange(LSPTextDocument* textDocument, std::vector<Position>& positions)
{
	SelectionRangeParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.positions = std::move(positions);
	return SendRequest(X(textDocument), "textDocument/selectionRange", params);
}


RequestID
LSPProjectWrapper::OnTypeFormatting(LSPTextDocument* textDocument, Position position, string_ref ch)
{
	DocumentOnTypeFormattingParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	params.ch = std::move(ch);
	return SendRequest(X(textDocument), "textDocument/onTypeFormatting", std::move(params));
}


RequestID
LSPProjectWrapper::Formatting(LSPTextDocument* textDocument)
{
	DocumentFormattingParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/formatting", std::move(params));
}


RequestID
LSPProjectWrapper::CodeAction(LSPTextDocument* textDocument, Range range, CodeActionContext context)
{
	CodeActionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.range = range;
	params.context = std::move(context);
	return SendRequest(X(textDocument), "textDocument/codeAction", std::move(params));
}


RequestID
LSPProjectWrapper::Completion(
	LSPTextDocument* textDocument, Position position, CompletionContext& context)
{
	CompletionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	params.context = option<CompletionContext>(context);
	return SendRequest(X(textDocument), "textDocument/completion", params);
}


RequestID
LSPProjectWrapper::SignatureHelp(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/signatureHelp", std::move(params));
}


RequestID
LSPProjectWrapper::GoToDefinition(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/definition", std::move(params));
}


RequestID
LSPProjectWrapper::GoToImplementation(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/implementation", std::move(params));
}


RequestID
LSPProjectWrapper::GoToDeclaration(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/declaration", std::move(params));
}


RequestID
LSPProjectWrapper::References(LSPTextDocument* textDocument, Position position)
{
	ReferenceParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/references", std::move(params));
}


RequestID
LSPProjectWrapper::SwitchSourceHeader(LSPTextDocument* textDocument)
{
	TextDocumentIdentifier params;
	params.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/switchSourceHeader", std::move(params));

	return 0;
}


RequestID
LSPProjectWrapper::Rename(LSPTextDocument* textDocument, Position position, string_ref newName)
{
	RenameParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	params.newName = newName;
	return SendRequest(X(textDocument), "textDocument/rename", std::move(params));
}


RequestID
LSPProjectWrapper::Hover(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/hover", std::move(params));
}


RequestID
LSPProjectWrapper::DocumentSymbol(LSPTextDocument* textDocument)
{
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/documentSymbol", std::move(params));
}


RequestID
LSPProjectWrapper::DocumentColor(LSPTextDocument* textDocument)
{
	DocumentSymbolParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/documentColor", std::move(params));
}


RequestID
LSPProjectWrapper::DocumentHighlight(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/documentHighlight", std::move(params));
}


RequestID
LSPProjectWrapper::SymbolInfo(LSPTextDocument* textDocument, Position position)
{
	TextDocumentPositionParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	return SendRequest(X(textDocument), "textDocument/symbolInfo", std::move(params));
}


RequestID
LSPProjectWrapper::TypeHierarchy(
	LSPTextDocument* textDocument, Position position, TypeHierarchyDirection direction, int resolve)
{
	TypeHierarchyParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	params.position = position;
	params.direction = direction;
	params.resolve = resolve;
	return SendRequest(X(textDocument), "textDocument/typeHierarchy", std::move(params));
}


RequestID
LSPProjectWrapper::DocumentLink(LSPTextDocument* textDocument)
{
	DocumentLinkParams params;
	params.textDocument.uri = std::move(textDocument->GetFilenameURI().String());
	return SendRequest(X(textDocument), "textDocument/documentLink", std::move(params));
}


RequestID
LSPProjectWrapper::SendRequest(RequestID id, string_ref method, value params)
{
	id.append("_").append(method);
	fLSPPipeClient->request(method, params, id);
	return id;
}


void
LSPProjectWrapper::SendNotify(string_ref method, value params = json())
{
	fLSPPipeClient->notify(method, params);
}
