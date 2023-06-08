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

namespace Genio
{

std::string const
file_type(const std::string& fullpath)
{
	BPath path(fullpath.c_str());
	std::string filename = path.Leaf();
	
 	if (filename.find("Jamfile") == 0) {
		return "jam";
	}
	
	if (filename.find("Makefile") == 0
		|| filename.find("makefile") == 0) {
		return "make";
	}
	
	auto pos = filename.find_last_of('.');
	if (pos != std::string::npos) {
		std::string extension = filename.substr(filename.find_last_of('.'));
		if (IsCppHeaderExtension(extension) || IsCppSourceExtension(extension))
			return "c++";
		else if (extension == ".rs")
			return "rust";
	}
	return "";
}

} // end of namespace
