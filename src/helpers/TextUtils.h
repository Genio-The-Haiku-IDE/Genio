/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TextUtils_H
#define TextUtils_H


#include <string>
#include "String.h"

extern std::string wordCharacters;
extern std::string whiteSpaces;

bool Contains(std::string const &s, char ch) noexcept;

bool IsASpace(int ch) noexcept;

const BString EscapeQuotesWrap(const BString& string);

// trim from start (in place)
void LeftTrim(std::string &s);


#endif // TextUtils_H
