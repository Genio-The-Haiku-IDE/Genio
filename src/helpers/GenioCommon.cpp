/*
 * Copyright 2018 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GenioCommon.h"

#include <Entry.h>

#include <cctype>
#include <ctime>
#include <sstream>
#include <string>
#include "Utils.h"
#include <Path.h>
#include "Log.h"
namespace Genio
{

std::string const
file_type(const std::string& fullpath)
{
	BPath path;
	//should 'support' also the URI format
	if (fullpath.find("file://") == 0) {
		path.SetTo(fullpath.substr(7).c_str());
	} else {
		path.SetTo(fullpath.c_str());
	}
	
	if (path.InitCheck() != B_OK) {
		LogErrorF("Invalid path: %s", fullpath.c_str());
		return "";
	}
	
	std::string filename = path.Leaf(); //ensure InitCheck is checked!
	
 	if (filename.find("Jamfile") == 0) {
		return "jam";
	}
	
	if (filename.find("Makefile") == 0
		|| filename.find("makefile") == 0) {
		return "make";
	}
	
	auto pos = filename.find_last_of('.');
	if (pos != std::string::npos) {
		std::string extension = filename.substr(pos);
		if (IsCppHeaderExtension(extension) || IsCppSourceExtension(extension))
			return "c++";
		else if (extension == ".rs")
			return "rust";
	}
	return "";
}

} // end of namespace
