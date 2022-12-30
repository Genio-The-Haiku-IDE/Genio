/*
 * Copyright 2018, Your Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileWrapper_H
#define FileWrapper_H
#include <iostream>
#include "json.hpp"
#include <SupportDefs.h>
#include "Editor.h"

class FileWrapper {
public:
		FileWrapper(std::string fileURI);
		
		void	didOpen(const char* text, Editor* editor);
		void	didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char);
		void	didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength);
		void	didClose();
		void	StartCompletion();
		void	SelectedCompletion(const char* text);
		void	Format();
		void	GoToDefinition();
		void	GoToDeclaration();
		void	SwitchSourceHeader();
		


	static void Initialize(const char* rootURI = "");
	static void Dispose();
	
private:

	std::string fFilenameURI;
	Editor*		fEditor;
	nlohmann::json		fCurrentCompletion;
	Sci_Position		fCompletionPosition;
	//Range				fLastRangeFormatting;

private:
	//callbacks:
	void	_DoFormat(nlohmann::json& params);
};


#endif // FileWrapper_H
