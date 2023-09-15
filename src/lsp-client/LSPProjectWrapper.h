/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _H_LSPProjectWrapper
#define _H_LSPProjectWrapper

#include <SupportDefs.h>
#include <Locker.h>
#include "MessageHandler.h"
#include "json_fwd.hpp"
#include <atomic>
#include <thread>
#include <optional>

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

using json = nlohmann::json;

class LSPProjectWrapper : public MessageHandler {
	
public:
			LSPProjectWrapper(const char* uri);
	virtual ~LSPProjectWrapper() = default;
		
	
	bool	Dispose();
	
	bool	RegisterTextDocument(LSPTextDocument* fw);
	void	UnregisterTextDocument(LSPTextDocument* fw);

    void onNotify(std::string method, value &params);
    void onResponse(RequestID ID, value &result);
    void onError(RequestID ID, value &error);
    void onRequest(std::string method, value &params, value &ID);

public:
    RequestID Initialize(option<DocumentUri> rootUri = {});
    RequestID Shutdown();
    RequestID Sync();
    void Exit();
    void Initialized(json& result);
    
    RequestID RegisterCapability();
    void DidOpen(LSPTextDocument* textDocument, string_ref text, string_ref languageId = "cpp");
    void DidClose(LSPTextDocument* textDocument);
    void DidChange(LSPTextDocument* textDocument, std::vector<TextDocumentContentChangeEvent> &changes,
                   option<bool> wantDiagnostics = {});
    void DidSave(LSPTextDocument* textDocument);
    RequestID RangeFomatting(LSPTextDocument* textDocument, Range range);
    RequestID FoldingRange(LSPTextDocument* textDocument);
    RequestID SelectionRange(LSPTextDocument* textDocument, std::vector<Position> &positions);
    RequestID OnTypeFormatting(LSPTextDocument* textDocument, Position position, string_ref ch);
    RequestID Formatting(LSPTextDocument* textDocument);
    RequestID CodeAction(LSPTextDocument* textDocument, Range range, CodeActionContext context);
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
    
    std::string&	allCommitCharacters() { return fAllCommitCharacters; }
    std::string&	triggerCharacters() { return fTriggerCharacters; }

private:
	bool	_Create();
	LSPPipeClient*			fLSPPipeClient;
	LSPTextDocument*	_DocumentByURI(const char* uri);

	typedef std::map<std::string, LSPTextDocument*> MapFile;

	MapFile	fTextDocs;

	std::atomic<bool> fInitialized;

	std::string fAllCommitCharacters;
	std::string fTriggerCharacters;
	
	std::string fRootURI;

};


#endif // _H_LSPProjectWrapper
