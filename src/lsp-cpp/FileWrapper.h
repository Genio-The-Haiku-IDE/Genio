/*
<<<<<<< HEAD
 * Copyright 2023, Andrea Anzani 
=======
 * Copyright 2018, Your Name 
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileWrapper_H
#define FileWrapper_H
#include <iostream>
<<<<<<< HEAD
<<<<<<< HEAD
#include "LSPTextDocument.h"
=======
#include "MessageHandler.h"
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
#include <SupportDefs.h>
#include <ToolTip.h>
#include <Autolock.h>
#include "Editor.h"
#include <vector>
#include "protocol.h"
class LSPClientWrapper;

class FileWrapper : public LSPTextDocument {
=======
#include "json.hpp"
#include <SupportDefs.h>
#include "Editor.h"

<<<<<<< HEAD
class FileWrapper {
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
class LSPClientWrapper;

class FileWrapper : public MessageHandler {
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
	
public:
	enum GoToType {
		GOTO_DEFINITION,
		GOTO_DECLARATION,
		GOTO_IMPLEMENTATION
	};
	
public:
<<<<<<< HEAD
<<<<<<< HEAD
				FileWrapper(std::string fileURI, Editor* fEditor);
		void	ApplySettings();
		void	SetLSPClient(LSPClientWrapper* cW);
		void	UnsetLSPClient();
private:
		void	didOpen();
public:
		void	didClose();
				
=======
				FileWrapper(std::string fileURI);
				
		void	SetLSPClient(LSPClientWrapper* cW);
		
		void	didOpen(const char* text, Editor* editor);
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
		void	didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char);
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	didSave();

=======
		FileWrapper(std::string fileURI);
		
		void	didOpen(const char* text, long len);
		void	didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char);

		void	didClose();
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	GoTo(FileWrapper::GoToType type);
		void	SwitchSourceHeader();
<<<<<<< HEAD
=======

>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
		void	StartHover(Sci_Position sci_position);
		void	EndHover();
		
		void	SignatureHelp();
		
		/* experimental */
		void	CharAdded(const char ch /*utf-8?*/);
		bool	StartCallTip();
		void	ContinueCallTip();
		void	UpdateCallTip(int deltaPos);
		int 	braceCount = 0;
		int 	startCalltipWord;
		Sci_Position calltipPosition;
		Sci_Position calltipStartPosition;
		nlohmann::json 	lastCalltip;
		int 	currentCalltip = 0;
		int 	maxCalltip = 0;
		std::string functionDefinition;
		/************************/
<<<<<<< HEAD
<<<<<<< HEAD
public:
		//still experimental
		//std::string		fID;
=======
public:
		//still experimental
		std::string		fID;
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
		void onNotify(std::string method, value &params);
		void onResponse(RequestID ID, value &result);
		void onError(RequestID ID, value &error);
		void onRequest(std::string method, value &params, value &ID);
		
		
<<<<<<< HEAD
private:

	Editor*		fEditor;
	nlohmann::json		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	BTextToolTip* 		fToolTip;
	LSPClientWrapper*	fLSPClientWrapper;
	bool				initialized = false; //to be removed, use LSPClientWrapper pointer..
	

	std::vector<Diagnostic>		fLastDiagnostics;
	
	void				_ShowToolTip(const char* text);
	void				_RemoveAllDiagnostics();
	
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
	
private:
	//utils
	void 			FromSciPositionToLSPPosition(const Sci_Position &pos, Position &lsp_position);
	Sci_Position 	FromLSPPositionToSciPosition(const Position& lsp_position);
	void 			GetCurrentLSPPosition(Position &lsp_position);
	void 			FromSciPositionToRange(Sci_Position s_start, Sci_Position s_end, Range &range);
	Sci_Position 	ApplyTextEdit(json &textEdit);
	void			OpenFile(std::string &uri, int32 line = -1, int32 character = -1);
	std::string 	GetCurrentLine();                            
=======


	static void Initialize(const char* rootURI = "");
	static void Dispose();

	

	
=======
    
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
private:

	std::string fFilenameURI;

	Editor*		fEditor;
	nlohmann::json		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
<<<<<<< HEAD


>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
=======
	BTextToolTip* 		fToolTip;
	LSPClientWrapper*	fLSPClientWrapper;
	bool				initialized = false; //to be removed
private:
	//callbacks:
	void	_DoFormat(nlohmann::json& params);
	void 	_DoHover(nlohmann::json& params);
	void	_DoGoTo(nlohmann::json& params);
	void	_DoSignatureHelp(nlohmann::json& params);
	void	_DoOpenFile(nlohmann::json& params);
	void	_DoSwitchSourceHeader(nlohmann::json& params);
	void	_DoCompletion(nlohmann::json& params);
>>>>>>> 6868452 (major (raw) refactor to split between a lsp textDocument and a lsp project)
};


#endif // FileWrapper_H
