/*
 * Copyright 2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
// TODO: Very basic, it only reads the "NAME" from haiku makefiles.
// Should implement a real makefile reader

#include "MakeFileHandler.h"

#include <fstream>
#include <iostream>


MakeFileHandler::MakeFileHandler()
{
}


MakeFileHandler::MakeFileHandler(const char* path)
{
	SetTo(path);
}


MakeFileHandler::~MakeFileHandler()
{
}


status_t
MakeFileHandler::SetTo(const char* path)
{
	std::ifstream inFile(path);
	std::string line;
	while (std::getline(inFile, line)) {
		if (line.starts_with("NAME")) {
			size_t pos = line.find("=");
			if (pos != std::string::npos) {
				fTargetName = line.substr(pos + 1).c_str();
				break;
			}
		}
	}
	return B_OK;
}


void
MakeFileHandler::GetTargetName(BString& outName) const
{
	outName = fTargetName;
}


void
MakeFileHandler::SetTargetName(const BString& inName)
{
	fTargetName = inName;
}
