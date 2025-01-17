/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <MessageFilter.h>
#include <View.h>
#include "Editor.h"

typedef filter_result (Editor::*funcPtrHandler)(BMessage*);

// This filter is used to intercept a specific message messages before it reach the Scintilla view.
// This is needed because we can't override the scintilla view as it's 'hidden'
// behind a BScrollView (see BScintillaView)

class EditorMessageFilter : public BMessageFilter {
public:
	EditorMessageFilter(system_message_code what, funcPtrHandler func):
		BMessageFilter(what),
		fFunc(func)
		{};
	filter_result		Filter(BMessage* message, BHandler** _target) {
		if (_target && *_target) {
			BView* scintillaView = dynamic_cast<BView*>(*_target);
			if (scintillaView) {
				Editor* editor = dynamic_cast<Editor*>(scintillaView->Parent());
				if (editor) {
					return (editor->*fFunc)(message);
				}
			}
		}
		return B_DISPATCH_MESSAGE;
	}

	funcPtrHandler	fFunc;
};

