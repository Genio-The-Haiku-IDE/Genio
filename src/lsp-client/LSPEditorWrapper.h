/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPEditorWrapper_H
#define LSPEditorWrapper_H

#include <iostream>
#include <SupportDefs.h>
#include <ToolTip.h>
#include <Autolock.h>
#include <vector>
#include "Sci_Position.h"
#include "LSPTextDocument.h"
#include "protocol_objects.h"

class LSPProjectWrapper;
class Editor;

class LSPEditorWrapper : public LSPTextDocument {
	
public:
	enum GoToType {
		GOTO_DEFINITION,
		GOTO_DECLARATION,
		GOTO_IMPLEMENTATION
	};
	
public:
				LSPEditorWrapper(BString fileURI, Editor* fEditor);
		virtual	~LSPEditorWrapper() {};
		void	ApplySettings();
		void	SetLSPClient(LSPProjectWrapper* cW);
		void	UnsetLSPClient();
		bool	HasLSPClient();

private:
		void	didOpen();
public:
		void	didClose();
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	didSave();

		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	GoTo(LSPEditorWrapper::GoToType type);
		void	SwitchSourceHeader();
		void	StartHover(Sci_Position sci_position);
		void	EndHover();
		
		
		void	IndicatorClick(Sci_Position position);
		
		void	CharAdded(const char ch /*utf-8?*/);
		
		/* experimental */	
		void	ContinueCallTip();
		void	UpdateCallTip(int deltaPos);
		
		
private:
		/* experimental section */	
		bool	StartCallTip(bool searchStart);
		
		
		
		int 	braceCount = 0;
		int 	startCalltipWord;
		Sci_Position calltipPosition;
		Sci_Position calltipStartPosition;
		SignatureHelp 	lastCalltip;
		int 	currentCalltip = 0;
		int 	maxCalltip = 0;
		std::string functionDefinition;
		/************************/
public:
		//still experimental
		//std::string		fID;
		void onNotify(std::string method, value &params);
		void onResponse(RequestID ID, value &result);
		void onError(RequestID ID, value &error);
		void onRequest(std::string method, value &params, value &ID);
		
		
private:

	struct InfoRange {
		Sci_Position	from;
		Sci_Position	to;
		std::string		info;
	};

	Editor*				fEditor;
	CompletionList		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	BTextToolTip* 		fToolTip;
	LSPProjectWrapper*	fLSPProjectWrapper;
	BString				fFileStatus;

	std::vector<InfoRange>		fLastDiagnostics;
	std::vector<InfoRange>		fLastDocumentLinks;
	
	void				_ShowToolTip(const char* text);
	void				_RemoveAllDiagnostics();
	void				_RemoveAllDocumentLinks();
	
private:
	//callbacks:
	void	_DoFormat(nlohmann::json& params);
	void 	_DoHover(nlohmann::json& params);
	void	_DoGoTo(nlohmann::json& params);
	void	_DoSignatureHelp(nlohmann::json& params);
	void	_DoSwitchSourceHeader(nlohmann::json& params);
	void	_DoCompletion(nlohmann::json& params);
	void	_DoDiagnostics(nlohmann::json& params);
	void	_DoDocumentLink(nlohmann::json& params);
	void	_DoFileStatus(nlohmann::json& params);
	
private:
	//utils
	void 			FromSciPositionToLSPPosition(const Sci_Position &pos, Position *lsp_position);
	Sci_Position 	FromLSPPositionToSciPosition(const Position* lsp_position);
	void 			GetCurrentLSPPosition(Position *lsp_position);
	void 			FromSciPositionToRange(Sci_Position s_start, Sci_Position s_end, Range *range);
	Sci_Position 	ApplyTextEdit(nlohmann::json &textEdit);
	void			OpenFileURI(std::string uri, int32 line = -1, int32 character = -1);
	std::string 	GetCurrentLine();
	bool			IsStatusValid();
};


#endif // LSPEditorWrapper_H
