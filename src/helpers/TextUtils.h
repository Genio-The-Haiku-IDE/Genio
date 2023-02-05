/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TextUtils_H
#define TextUtils_H


#include <string>

extern std::string wordCharacters;

bool Contains(std::string const &s, char ch) noexcept;

bool IsASpace(int ch) noexcept;

#endif // TextUtils_H
