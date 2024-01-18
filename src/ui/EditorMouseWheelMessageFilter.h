/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorMouseWheelMessageFilter_H
#define EditorMouseWheelMessageFilter_H


#include <MessageFilter.h>
#include <View.h>

#include "GenioWindowMessages.h"

// This filter is used to intercept wheel messages before they reach the Scintilla view.
// if the COMMAND_KEY is pressed the message is re-router to the Looper (GenioWindow).

class EditorMouseWheelMessageFilter : public BMessageFilter {
public:
	EditorMouseWheelMessageFilter():BMessageFilter(B_MOUSE_WHEEL_CHANGED){};
	filter_result		Filter(BMessage* message, BHandler** _target) {
		if (_target && *_target && modifiers() & B_COMMAND_KEY) {
				message->what = MSG_WHEEL_WITH_COMMAND_KEY;
				(*_target)->Looper()->PostMessage(message);
				return B_SKIP_MESSAGE;
		}
		return B_DISPATCH_MESSAGE;
	}
};


#endif // EditorMouseWheelMessageFilter_H
