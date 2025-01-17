/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <SupportDefs.h>
#include <string>
#include <stack>

// Once per project or a single one?

typedef std::string Position;

class JumpNavigator {
public:
	void AddJump(const char* filename);

	bool HasNext();
	bool HasPrev();

	const char* GetNext();
	const char* GetPrev();

private:
	std::stack<Position>	history;
	std::stack<Position>	forwardStack;
	Position				fCurrentPosition;
};


