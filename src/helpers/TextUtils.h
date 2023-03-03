/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TextUtils_H
#define TextUtils_H


#include <string>

#include "String.h"

extern std::string wordCharacters;

bool Contains(std::string const &s, char ch) noexcept;

bool IsASpace(int ch) noexcept;

const BString EscapeQuotesWrap(const BString& string);

#endif // TextUtils_H
