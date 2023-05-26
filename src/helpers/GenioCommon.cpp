/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GenioCommon.h"

#include <Entry.h>

#include <cctype>
#include <ctime>
#include <sstream>
#include <string>
#include "Utils.h"

namespace Genio
{

std::string
const Copyright()
{
	std::ostringstream header;
	header
		<< "/*\n"
		<< " * Copyright " << get_year() << " Your_Name <your@email.address>\n"
		<< " * All rights reserved. Distributed under the terms of the MIT license.\n"
		<< " */\n";

	return header.str();
}

/*
 * Returns the string to be used in header guard
 * In: ALLUPPER  = Do not modify
 *     all_lower = ALL_LOWER
 *     CamelCase = CAMEL_CASE
 */
std::string const
HeaderGuard(const std::string& fileName)
{
	std::string upper(fileName);
	std::string lower(fileName);
	std::transform (upper.begin(), upper.end(), upper.begin(), ::toupper);
	std::transform (lower.begin(), lower.end(), lower.begin(), ::toupper);

	// Filename is UPPER case, leave it
	if (fileName == upper)
		return fileName;
	// Filename is lower case, make it UPPER
	else if (fileName == lower)
		return upper;

	std::string in(fileName);
	std::string out("");

	for (auto iter = in.begin(); iter != in.end(); iter++) {
		if (iter != in.begin() && std::isupper(static_cast<unsigned char>(*iter)))
			out += static_cast<unsigned char>('_');
		out += std::toupper(static_cast<unsigned char>(*iter));
	}
	return out;
}

bool
file_exists(const std::string& filename)
{
	// TODO: Test if it's a file or directory ?
	return BEntry(filename.c_str()).Exists();
}

std::string const
file_type(const std::string& filename)
{
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

int
get_year()
{
	// Get year
	time_t time = ::time(NULL);
	struct tm* tm = localtime(&time);
	return tm->tm_year + 1900;
}

} // end of namespace
