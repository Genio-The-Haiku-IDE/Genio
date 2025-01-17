/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "JumpNavigator.h"


void
JumpNavigator::AddJump(const char* filename)
{
	history.push(filename);
	forwardStack = {};
}

bool
JumpNavigator::HasNext()
{
	return (!forwardStack.empty());
}

bool
JumpNavigator::HasPrev()
{
	return (!history.empty());
}

const char*
JumpNavigator::GetNext()
{
	if(HasNext()) {
		history.push(fCurrentPosition);
		fCurrentPosition = forwardStack.top();
		forwardStack.pop();
		return fCurrentPosition.c_str();
	}
	return nullptr;
}

const char*
JumpNavigator::GetPrev()
{
	if (HasPrev()) {
		forwardStack.push(fCurrentPosition);
		fCurrentPosition = history.top();
		history.pop();
		return fCurrentPosition.c_str();
	}
	return nullptr;
}