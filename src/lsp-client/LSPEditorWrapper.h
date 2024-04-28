/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPEditorWrapper_H
#define LSPEditorWrapper_H

#include <Autolock.h>
#include <ToolTip.h>

#include <vector>

#include "LSPProjectWrapper.h"
#include "LSPTextDocument.h"
#include "Sci_Position.h"
#include "protocol_objects.h"
#include "LSPCapabilities.h"
#include "CallTipContext.h"

struct InfoRange {
	Sci_Position	from;
	Sci_Position	to;
	std::string		info;
};

struct LSPDiagnostic {
	InfoRange range;
	Diagnostic diagnostic;
	std::string fixTitle;
};

class LSPProjectWrapper;
class Editor;
class LSPEditorWrapper : public LSPTextDocument {
public:
	enum GoToType {
		GOTO_DEFINITION,
		GOTO_DECLARATION,
		GOTO_IMPLEMENTATION
	};

				LSPEditorWrapper(BPath filenamePath, Editor* fEditor);
		virtual	~LSPEditorWrapper() {};
		void	ApplySettings();
		void	SetLSPServer(LSPProjectWrapper* cW);
		void	UnsetLSPServer();
		bool	HasLSPServer();
		bool	HasLSPServerCapability(const LSPCapability cap);
		void	ApplyFix(BMessage* info);
		void 	ApplyEdit(std::string info);
		void	GoTo(LSPEditorWrapper::GoToType type);

private:
		void	didOpen();
public:
		void	didClose();
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	flushChanges();
		void	didSave();

		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	Rename(std::string newName);
		void	SwitchSourceHeader();
		void	StartHover(Sci_Position sci_position);
		void	EndHover();
		void	GetDiagnostics(std::vector<LSPDiagnostic>& diagnostics) { diagnostics = fLastDiagnostics; }
		void	RequestCodeActions(Diagnostic& diagnostic);
		void	CodeActionResolve(value &params);

		void	IndicatorClick(Sci_Position position);

		void	RequestDocumentSymbols();
		void	CharAdded(const char ch /*utf-8?*/);

		void	NextCallTip();
		void	PrevCallTip();

		void	FindReferences();

public:
	//still experimental
	//std::string		fID;
	void onNotify(std::string method, value &params);
	void onResponse(RequestID ID, value &result);
	void onError(RequestID ID, value &error);
	void onRequest(std::string method, value &params, value &ID);
	int32 DiagnosticFromPosition(Sci_Position p, LSPDiagnostic& dia);
	int32 DiagnosticFromRange(Range& range, LSPDiagnostic& dia);

	Editor*				fEditor;
	CompletionList		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	BTextToolTip* 		fToolTip;
	LSPProjectWrapper*	fLSPProjectWrapper;
	BString				fFileStatus;
	CallTipContext		fCallTip;
	bool				fInitialized;

private:
	bool	IsInitialized();
	std::vector<LSPDiagnostic>	fLastDiagnostics;
	std::vector<InfoRange>		fLastDocumentLinks;

	void				_ShowToolTip(const char* text);
	void				_RemoveAllDiagnostics();
	void				_RemoveAllDocumentLinks();



private:
	//callbacks:
	void	_DoFormat(nlohmann::json& params);
	void	_DoRename(nlohmann::json& params);
	void	_DoHover(nlohmann::json& params);
	void	_DoGoTo(nlohmann::json& params);
	void	_DoSignatureHelp(nlohmann::json& params);
	void	_DoSwitchSourceHeader(nlohmann::json& params);
	void	_DoCompletion(nlohmann::json& params);
	void	_DoDiagnostics(nlohmann::json& params);
	void	_DoDocumentLink(nlohmann::json& params);
	void	_DoFileStatus(nlohmann::json& params);
	void	_DoDocumentSymbol(nlohmann::json& params);
	void	_DoInitialize(nlohmann::json& params);
	void	_DoCodeActions(nlohmann::json& params);
	void	_DoCodeActionResolve(nlohmann::json& params);
	void	_DoReferences(nlohmann::json& params);

	void	_DoRecursiveDocumentSymbol(std::vector<DocumentSymbol>& v, BMessage& msg);
	void	_DoLinearSymbolInformation(std::vector<SymbolInformation>& v, BMessage& msg);

private:
	//utils
	void 			FromSciPositionToLSPPosition(const Sci_Position &pos, Position *lsp_position);
	Sci_Position 	FromLSPPositionToSciPosition(const Position* lsp_position);
	void 			GetCurrentLSPPosition(Position *lsp_position);
	void 			FromSciPositionToRange(Sci_Position s_start, Sci_Position s_end, Range *range);
	Sci_Position 	ApplyTextEdit(nlohmann::json &textEdit);
	Sci_Position 	ApplyTextEdit(TextEdit &textEdit);
	void			OpenFileURI(std::string uri, int32 line = -1, int32 character = -1,
						BString edits = "");
	std::string 	GetCurrentLine();
	bool			IsStatusValid();
	std::vector<TextDocumentContentChangeEvent> fChanges;

};

#endif // LSPEditorWrapper_H
