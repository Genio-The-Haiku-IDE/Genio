/*
 * Copyright 2018 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GenioCommon.h"

#include <cctype>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>

namespace Genio
{

std::string const Copyright()
{
	std::ostringstream header;
	header
		<< "/*\n"
		<< " * Copyright " << get_year() << " Your_Name \n"
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
std::string const HeaderGuard(const std::string& fileName)
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

bool file_exists(const std::string& filename)
{
	// Test existence
	std::ifstream file(filename);
	if (file.is_open()) {
		file.close();
		return true;
	}
	return false;
}

std::string const file_type(const std::string& filename)
{
 	if (filename.find("Jamfile") == 0) {
		return "jam";
	}
	if (filename.find("Makefile") == 0
		|| filename.find("makefile") == 0) {
		return "make";
	}

	std::string extension = filename.substr(filename.find_last_of('.') + 1);
	if (extension == "cpp" || extension == "cxx" || extension == "cc"
			 || extension == "h" || extension == "hpp" || extension == "c")
		return "c++";
	else if (extension == "rs")
		return "rust";

	return "";
}

int get_year()
{
	// Get year
	time_t time = ::time(NULL);
	struct tm* tm = localtime(&time);
	return tm->tm_year + 1900;
}







} // end of namespace
