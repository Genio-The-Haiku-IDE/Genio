/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileWrapper_H
#define FileWrapper_H
#include <iostream>

#include <SupportDefs.h>


class FileWrapper {
public:
		FileWrapper(std::string fileURI);
		
		void	didOpen(const char* text, long len);
		void	didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char);

		void	didClose();
		void	Completion(int _line, int _char);
		void	Format();
		void	GoToDefinition();
		void	GoToDeclaration();
		void	SwitchSourceHeader();

	static void Initialize(const char* rootURI = "");
	static void Dispose();

	
private:

	std::string fFilenameURI;

};


#endif // FileWrapper_H
