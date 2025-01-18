/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "JumpNavigator.h"
#include <Message.h>
#include <Application.h>
#include <cstdio>


void
JumpNavigator::JumpToFile(BMessage* message, JumpPosition& currentPosition) //TODO: store current position!!!
{
	entry_ref	ref;
	if(message->FindRef("refs", 0, &ref) == B_OK)
	{
		if (currentPosition != fCurrentPosition) {
			history.push(currentPosition);
			forwardStack = {};
			fCurrentPosition = currentPosition;
		}

		message->AddBool("jump", true);
		message->what = B_REFS_RECEIVED;
		be_app->PostMessage(message);
	}
}


void
JumpNavigator::Jumped(JumpPosition& position)
{
	printf("Jumped to %s\n", position.name);
	if (position == fCurrentPosition)
		return;

	/*if (fCurrentPosition.device != -1)
		history.push(fCurrentPosition);*/

	forwardStack = {};
	fCurrentPosition = position;
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


JumpPosition
JumpNavigator::GetNext()
{
	if(HasNext()) {
		history.push(fCurrentPosition);
		fCurrentPosition = forwardStack.top();
		forwardStack.pop();
		_GoToCurrentPosition();
		return fCurrentPosition;
	}
	printf("NO NEXT\n");
	return JumpPosition();
}


JumpPosition
JumpNavigator::GetPrev()
{
	if (HasPrev()) {
		forwardStack.push(fCurrentPosition);
		fCurrentPosition = history.top();
		history.pop();
		_GoToCurrentPosition();
		return fCurrentPosition;
	}
	printf("NO PREV\n");
	return JumpPosition();
}


void
JumpNavigator::_GoToCurrentPosition()
{
	printf("Going to %s\n", fCurrentPosition.name);
	BMessage ref(B_REFS_RECEIVED);
	ref.AddRef("refs", &fCurrentPosition);
	be_app->PostMessage(&ref);
}