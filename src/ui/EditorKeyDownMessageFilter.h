/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorKeyDownMessageFilter_H
#define EditorKeyDownMessageFilter_H


#include <SupportDefs.h>
#include <MessageFilter.h>
#include <View.h>

// This filter is used to intercept key_down messages before they reach the Scintilla view.
// This is needed because we can't override the KeyDown message of the scintilla view as it's 'hidden'
// behind a BScrollView (see BScintillaView)

class EditorKeyDownMessageFilter : public BMessageFilter {
public:
	EditorKeyDownMessageFilter():BMessageFilter(B_KEY_DOWN){};
	filter_result		Filter(BMessage* message, BHandler** _target) {
		if (_target && *_target) {
			BView* scintillaView = dynamic_cast<BView*>(*_target);
			if (scintillaView) {
				Editor* editor = dynamic_cast<Editor*>(scintillaView->Parent());
				if (editor) {
					return editor->BeforeKeyDown(message);
				}
			}
		}
		return B_DISPATCH_MESSAGE;
	}
};


#endif // EditorKeyDownMessageFilter_H
