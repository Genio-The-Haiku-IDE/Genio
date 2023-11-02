/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioCommon.h"

#include <Path.h>
#include <Url.h>

#include "Log.h"
#include "Utils.h"

namespace Genio
{



//TODO deprecate!
std::string const
DEP_file_type(const std::string& fullpath)
{
	BPath path(fullpath.c_str());

	if (path.InitCheck() != B_OK) {
		BUrl url(fullpath.c_str());
		if (url.IsValid() && url.HasPath()) {
			path = BPath(url.Path().String());
			if (path.InitCheck() != B_OK) {
				LogErrorF("Invalid URI: %s", fullpath.c_str());
				return "";
			}
		} else {
			// Not an url, nor a path
			LogErrorF("Invalid path: %s", fullpath.c_str());
		}
	}

	std::string filename = path.Leaf(); //ensure InitCheck is checked!

	auto pos = filename.find_last_of('.');
	if (pos != std::string::npos) {
		std::string extension = filename.substr(pos);
		if (IsCppHeaderExtension(extension) || IsCppSourceExtension(extension))
			return "c++";
		else if (extension == ".rs")
			return "rust";
	}

	if (filename.find("Jamfile") == 0) {
		return "jam";
	}

	if (filename.find("Makefile") == 0
		|| filename.find("makefile") == 0) {
		return "make";
	}

	return "";
}

} // end of namespace
