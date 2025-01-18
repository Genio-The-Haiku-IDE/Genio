/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <SupportDefs.h>
#include <Entry.h>
#include <stack>

class BMessage;
// Once per project or a single one?

typedef entry_ref JumpPosition;

class JumpNavigator {
public:
	enum {
		kJumpPrev	=	'jmpp',
		kJumpNext	=	'jmpn'
	};



    static JumpNavigator* getInstance() {
		static JumpNavigator	instance;
        return &instance;
    }

	void	JumpToFile(BMessage* message, JumpPosition* currentPosition);
	void	JumpingTo(JumpPosition& newPosition, JumpPosition& fromPosition);

	bool HasNext();
	bool HasPrev();

	JumpPosition GetNext();
	JumpPosition GetPrev();

private:
							JumpNavigator(){ fCurrentPosition.device = -1; }
					void	_GoToCurrentPosition();

	std::stack<JumpPosition>	history;
	std::stack<JumpPosition>	forwardStack;
	JumpPosition				fCurrentPosition;
};


