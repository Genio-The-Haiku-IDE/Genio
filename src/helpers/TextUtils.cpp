/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TextUtils.h"

#include <algorithm>


const std::string kWordCharacters ("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
const std::string kWhiteSpaces (" \n\t\r");

bool Contains(std::string const &s, char ch) noexcept {
	return s.find(ch) != std::string::npos;
}

bool IsASpace(int ch) noexcept {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

const BString EscapeQuotesWrap(const BString& path) {
	BString s = "\"";
	s += path;
	s += "\"";
	return s;
}

// trim from start (in place)
void LeftTrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

void RightTrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

void Trim(std::string& s) {
	LeftTrim(s);
	RightTrim(s);
}