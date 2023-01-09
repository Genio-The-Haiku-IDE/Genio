/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileWrapper_H
#define FileWrapper_H
#include <iostream>
#include "json.hpp"
#include <SupportDefs.h>
#include <ToolTip.h>
#include <Autolock.h>
#include "Editor.h"

class FileWrapper {
	
public:
	enum GoToType {
		GOTO_DEFINITION,
		GOTO_DECLARATION,
		GOTO_IMPLEMENTATION
	};
	
public:
		FileWrapper(std::string fileURI);
		
		void	didOpen(const char* text, Editor* editor);
		void	didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char);
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	didSave();
		void	didClose();
		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	GoTo(FileWrapper::GoToType type);
		void	SwitchSourceHeader();
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

	static void Initialize(const char* rootURI = "");
	static void Dispose();
	

	
private:

	std::string fFilenameURI;
	Editor*		fEditor;
	nlohmann::json		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	BTextToolTip* 		fToolTip;

private:
	//callbacks:
	void	_DoFormat(nlohmann::json& params);
};


#endif // FileWrapper_H
