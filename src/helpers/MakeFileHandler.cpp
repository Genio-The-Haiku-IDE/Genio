/*
 * Copyright 2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
// TODO: Very basic, it only reads the "NAME" from haiku makefiles.
// Should implement a real makefile reader

#include "MakeFileHandler.h"

#include <fstream>
#include <iostream>

#include "TextUtils.h"

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
	bool foundName = false;
	bool foundDir = false;
	while (std::getline(inFile, line)) {
		if (!foundName) {
			size_t pos = line.find("NAME");
			if (pos != std::string::npos) {
				line = line.substr(pos);
				pos = line.find("=");
				if (pos != std::string::npos) {
					std::string str = line.substr(pos + 1).c_str();
					Trim(str);
					fTargetName = str.c_str();
					foundName = true;
				}
			}
		}
		if (!foundDir) {
			size_t pos = line.find("TARGET_DIR");
			if (pos != std::string::npos) {
				line = line.substr(pos);
				pos = line.find("=");
				if (pos != std::string::npos) {
					std::string str = line.substr(pos + 1).c_str();
					Trim(str);
					fTargetDir = str.c_str();
					foundDir = true;
				}
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


void
MakeFileHandler::GetTargetDirectory(BString& outDir) const
{
	outDir = fTargetDir;
}


void
MakeFileHandler::SetTargetDirectory(const BString& inDir)
{
	fTargetDir = inDir;
}


void
MakeFileHandler::GetFullTargetName(BString &fullName) const
{
	if (!fTargetDir.IsEmpty()) {
		fullName = fTargetDir;
		fullName.Append("/");
	}
	fullName.Append(fTargetName);
}
