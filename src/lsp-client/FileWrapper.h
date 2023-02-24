/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileWrapper_H
#define FileWrapper_H

#include <iostream>
#include <SupportDefs.h>
#include <ToolTip.h>
#include <Autolock.h>
#include <vector>
#include "Sci_Position.h"
#include "LSPTextDocument.h"

class Position;
class Range;
class LSPClientWrapper;
class Editor;

class FileWrapper : public LSPTextDocument {
	
public:
	enum GoToType {
		GOTO_DEFINITION,
		GOTO_DECLARATION,
		GOTO_IMPLEMENTATION
	};
	
public:
				FileWrapper(std::string fileURI, Editor* fEditor);
		virtual	~FileWrapper() {};
		void	ApplySettings();
		void	SetLSPClient(LSPClientWrapper* cW);
		void	UnsetLSPClient();
private:
		void	didOpen();
public:
		void	didClose();
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	didSave();

		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	GoTo(FileWrapper::GoToType type);
		void	SwitchSourceHeader();
		void	StartHover(Sci_Position sci_position);
		void	EndHover();
		
		void	SignatureHelp();
		
		void	IndicatorClick(Sci_Position position);
		
		void	CharAdded(const char ch /*utf-8?*/);
		
		/* experimental */	
		void	ContinueCallTip();
		void	UpdateCallTip(int deltaPos);
private:
		/* experimental section */	
		bool	StartCallTip();
		
		
		
		int 	braceCount = 0;
		int 	startCalltipWord;
		Sci_Position calltipPosition;
		Sci_Position calltipStartPosition;
		nlohmann::json 	lastCalltip;
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
	nlohmann::json		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	BTextToolTip* 		fToolTip;
	LSPClientWrapper*	fLSPClientWrapper;
	bool				initialized = false; //to be removed, use LSPClientWrapper pointer..
	

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
	void	_DoOpenFile(nlohmann::json& params);
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
	Sci_Position 	ApplyTextEdit(json &textEdit);
	void			OpenFileURI(std::string uri, int32 line = -1, int32 character = -1);
	std::string 	GetCurrentLine();                            
};


#endif // FileWrapper_H
