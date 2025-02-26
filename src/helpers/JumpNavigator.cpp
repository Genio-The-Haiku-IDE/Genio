/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "JumpNavigator.h"

#include <Application.h>
#include <Message.h>

#include "Log.h"

void
JumpNavigator::JumpToFile(BMessage* message, JumpPosition* currentPosition)
{
	entry_ref ref;
	if (message->FindRef("refs", 0, &ref) == B_OK) {
		message->AddRef("jumpFrom", currentPosition);
		message->what = B_REFS_RECEIVED;
		be_app->PostMessage(message);
	}
}


void
JumpNavigator::JumpingTo(JumpPosition& newPosition, JumpPosition& fromPosition)
{
	LogDebugF("from %s to %s\n", fromPosition.name, newPosition.name);
	if (newPosition == fromPosition || newPosition == fCurrentPosition)
		return;

	history.push(fromPosition);
	forwardStack = {};
	fCurrentPosition = newPosition;
}


bool
JumpNavigator::HasNext() const
{
	return !forwardStack.empty();
}


bool
JumpNavigator::HasPrev() const
{
	return !history.empty();
}


void
JumpNavigator::JumpToNext()
{
	if (HasNext()) {
		history.push(fCurrentPosition);
		fCurrentPosition = forwardStack.top();
		forwardStack.pop();
		_GoToCurrentPosition();
	}
}


void
JumpNavigator::JumpToPrev()
{
	if (HasPrev()) {
		forwardStack.push(fCurrentPosition);
		fCurrentPosition = history.top();
		history.pop();
		_GoToCurrentPosition();
	}
}


void
JumpNavigator::_GoToCurrentPosition()
{
	LogDebugF("%s\n", fCurrentPosition.name);
	BMessage ref(B_REFS_RECEIVED);
	ref.AddRef("refs", &fCurrentPosition);
	be_app->PostMessage(&ref);
}
