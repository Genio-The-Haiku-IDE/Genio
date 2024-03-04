/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _H_LSPProjectWrapper
#define _H_LSPProjectWrapper

#include <Locker.h>
#include <Path.h>
#include <Locker.h>
#include <atomic>
#include <MessageFilter.h>
#include <Messenger.h>

#include "MessageHandler.h"
#include "json_fwd.hpp"
#include "LSPCapabilities.h"
#include "protocol_objects.h"

class  LSPTextDocument;
struct TextDocumentContentChangeEvent;
struct Range;
struct Position;
struct CodeActionContext;
struct CompletionContext;
struct TweakArgs;
struct WorkspaceEdit;
struct FileEvent;
struct WorkspaceEdit;
struct ConfigurationSettings;
enum class TypeHierarchyDirection: int;
class LSPPipeClient;
class LSPServerConfigInterface;

using json = nlohmann::json;


class LSPProjectWrapper : public BHandler {

public:
			LSPProjectWrapper(BPath rootPath,
							  const BMessenger& msgr, const LSPServerConfigInterface& serverConfig);

	virtual ~LSPProjectWrapper();

	const LSPServerConfigInterface&	ServerConfig() { return fServerConfig;}

	virtual	void	MessageReceived(BMessage* message);

	bool	RegisterTextDocument(LSPTextDocument* fw);
	void	UnregisterTextDocument(LSPTextDocument* fw);

    void onNotify(std::string method, value &params);
    void onResponse(RequestID ID, value &result);
    void onError(RequestID ID, value &error);
    void onRequest(std::string method, value &params, value &ID);


	bool HasCapability(const LSPCapability flag);


public:
    RequestID Initialize(option<DocumentUri> rootUri = {});
    RequestID Shutdown();
    RequestID Sync();
    void Exit();
    void Initialized(json& result);

    RequestID RegisterCapability();
    void DidOpen(LSPTextDocument* textDocument, string_ref text, string_ref languageId);
    void DidClose(LSPTextDocument* textDocument);
    void DidChange(LSPTextDocument* textDocument, std::vector<TextDocumentContentChangeEvent> &changes,
                   option<bool> wantDiagnostics = {});
    void DidSave(LSPTextDocument* textDocument);
    RequestID RangeFomatting(LSPTextDocument* textDocument, Range range);
    RequestID FoldingRange(LSPTextDocument* textDocument);
    RequestID SelectionRange(LSPTextDocument* textDocument, std::vector<Position> &positions);
    RequestID OnTypeFormatting(LSPTextDocument* textDocument, Position position, string_ref ch);
    RequestID Formatting(LSPTextDocument* textDocument);
    RequestID CodeAction(LSPTextDocument* textDocument, Range range, CodeActionContext& context);
	RequestID CodeActionResolve(LSPTextDocument* textDocument, struct CodeAction& data);
	RequestID CodeActionResolve(LSPTextDocument* textDocument, nlohmann::json& data);
    RequestID Completion(LSPTextDocument* textDocument, Position position, CompletionContext& context);
    RequestID SignatureHelp(LSPTextDocument* textDocument, Position position);
    RequestID GoToDefinition(LSPTextDocument* textDocument, Position position);
    RequestID GoToImplementation(LSPTextDocument* textDocument, Position position);
    RequestID GoToDeclaration(LSPTextDocument* textDocument, Position position);
    RequestID References(LSPTextDocument* textDocument, Position position);
    RequestID SwitchSourceHeader(LSPTextDocument* textDocument);
    RequestID Rename(LSPTextDocument* textDocument, Position position, string_ref newName);
    RequestID Hover(LSPTextDocument* textDocument, Position position);
    RequestID DocumentSymbol(LSPTextDocument* textDocument);
    RequestID DocumentColor(LSPTextDocument* textDocument);
    RequestID DocumentHighlight(LSPTextDocument* textDocument, Position position);
    RequestID SymbolInfo(LSPTextDocument* textDocument, Position position);
    RequestID TypeHierarchy(LSPTextDocument* textDocument, Position position, TypeHierarchyDirection direction, int resolve);
    RequestID DocumentLink(LSPTextDocument* textDocument);

    RequestID 	SendRequest(RequestID id, string_ref method, value params);
    void 		SendNotify(string_ref method, value params);

    std::string&	allCommitCharacters() { return fAllCommitCharacters; } //not yet used.
    std::string&	triggerCharacters() { return fTriggerCharacters; } //for completion

private:
	bool	_Create();
	LSPPipeClient*			fLSPPipeClient;
	LSPTextDocument*	_DocumentByURI(const char* uri);
	bool _CheckAndSetCapability(json& capas, const char* str, const LSPCapability flag);

	typedef std::map<std::string, LSPTextDocument*> MapFile;

	MapFile	fTextDocs;

	std::atomic<bool> fInitialized;

	std::string fAllCommitCharacters;
	std::string fTriggerCharacters;

	std::string fRootURI;
	BMessenger fMessenger;
	uint32		fWhat;
	const LSPServerConfigInterface& fServerConfig;
	uint32	fServerCapabilities;
};

#endif // _H_LSPProjectWrapper
