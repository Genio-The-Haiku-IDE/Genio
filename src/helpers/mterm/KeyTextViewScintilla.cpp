/*
 * Copyright 2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "KeyTextViewScintilla.h"
#include "GMessage.h"

class ScintillaHaikuAllMessageFilter : public BMessageFilter {
public:
	ScintillaHaikuAllMessageFilter():BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE){ };
	filter_result		Filter(BMessage* message, BHandler** _target) {
		if (_target && *_target) {
			BView* scintillaView = dynamic_cast<BView*>(*_target);
			if (scintillaView) {
				KeyTextViewScintilla* editor = dynamic_cast<KeyTextViewScintilla*>(scintillaView->Parent());
				if (editor) {
					return editor->BeforeMessageReceived(message, scintillaView);
				}
			}
		}
		return B_DISPATCH_MESSAGE;
	}
};

uint32 kRealModifiers = B_COMMAND_KEY | B_OPTION_KEY | B_CONTROL_KEY | B_MENU_KEY;

KeyTextViewScintilla::KeyTextViewScintilla(const char *name, const BMessenger &msgr)
    : BScintillaView(name,0,true,true), fTarget(msgr), fEnableInput(false), fCaretPosition(-1)
{
	ClearBuffer();
	SendMessage(SCI_SETUNDOCOLLECTION, false);

	font_family fontName;
	be_fixed_font->GetFamilyAndStyle(&fontName, nullptr);
	SendMessage(SCI_STYLESETFONT, 32, (sptr_t) fontName);
	SendMessage(SCI_STYLESETSIZE, 32, (sptr_t) be_fixed_font->Size());
	SendMessage(SCI_USEPOPUP, SC_POPUP_NEVER);
	SendMessage(SCI_SETMARGINS, 0, 0);

	EnableInput(false);
}

void
KeyTextViewScintilla::AttachedToWindow()
{
	if (FindView("ScintillaHaikuView"))
		FindView("ScintillaHaikuView")->AddFilter(new ScintillaHaikuAllMessageFilter());
}
void
KeyTextViewScintilla::ClearAll()
{
	bool isRO = SendMessage(SCI_GETREADONLY);
	if (isRO) SendMessage(SCI_SETREADONLY, false);
	SendMessage(SCI_CLEARALL);
	if (isRO) SendMessage(SCI_SETREADONLY, true);
}

void
KeyTextViewScintilla::EnableInput(bool enable)
{
	fEnableInput = enable;
	SendMessage(SCI_SETCARETSTYLE, enable ? CARETSTYLE_BLOCK : CARETSTYLE_INVISIBLE);
	SendMessage(SCI_SETREADONLY, !enable);
	if (enable)
		SendMessage(SCI_GRABFOCUS);
}

void
KeyTextViewScintilla::Append(const BString& text)
{
	bool isRO = SendMessage(SCI_GETREADONLY);
	if (isRO) SendMessage(SCI_SETREADONLY, false);
	SendMessage(SCI_APPENDTEXT, text.Length(), (uptr_t)text.String());
	SendMessage(SCI_GOTOPOS, SendMessage(SCI_GETLENGTH));
	fCaretPosition = SendMessage(SCI_GETCURRENTPOS);
	if (isRO) SendMessage(SCI_SETREADONLY, true);
}

filter_result
KeyTextViewScintilla::BeforeMessageReceived(BMessage* msg, BView* scintillaView)
{
	switch(msg->what) {
		case B_KEY_DOWN:
			return BeforeKeyDown(msg, scintillaView);
		case B_CUT:
		case B_PASTE:
		case B_UNDO:
		case B_REDO:
			return B_SKIP_MESSAGE;
		default:
			return B_DISPATCH_MESSAGE;
	};
}

filter_result
KeyTextViewScintilla::BeforeKeyDown(BMessage* msg, BView* scintillaView)
{
	if(!fEnableInput)
		return B_DISPATCH_MESSAGE;

	const char* bytes = nullptr;
	if (msg->FindString("bytes", &bytes) != B_OK || bytes == nullptr)
		return B_SKIP_MESSAGE;

	int32 numBytes = strlen(bytes);

	  switch (bytes[numBytes - 1]) {

		  case B_LEFT_ARROW:
		  case B_RIGHT_ARROW:
		  case B_UP_ARROW:
		  case B_DOWN_ARROW:
		  case B_TAB:
		  case B_ESCAPE:
		  case B_SUBSTITUTE:
		  case B_INSERT:
		  case B_DELETE:
		  case B_HOME:
		  case B_END:
		  case B_PAGE_UP:
		  case B_PAGE_DOWN:
			break;
		  case B_ENTER:
			return B_DISPATCH_MESSAGE;
		  case B_BACKSPACE:
		    if (kRealModifiers & modifiers())
				return B_SKIP_MESSAGE;

			if (fCaretPosition != -1) {
				SendMessage(SCI_SETCURRENTPOS, fCaretPosition);
				SendMessage(SCI_SETANCHOR, fCaretPosition);
			}
			if (fBuffer.Length() > 0) {
			  fBuffer.Remove(fBuffer.Length() - 1, 1);
			  return B_DISPATCH_MESSAGE;
			}
			return B_SKIP_MESSAGE;

		  default:
			  if (bytes[numBytes - 1] < 32)
				return B_SKIP_MESSAGE;

			  if (fCaretPosition != -1) {
					SendMessage(SCI_SETCURRENTPOS, fCaretPosition);
					SendMessage(SCI_SETANCHOR, fCaretPosition);
				}
				return B_DISPATCH_MESSAGE;

			  };
		  return B_SKIP_MESSAGE;
}


void
KeyTextViewScintilla::NotificationReceived(SCNotification* notification)
{
	Sci_NotifyHeader* pNmhdr = &notification->nmhdr;

	switch (pNmhdr->code) {
		case SCN_CHARADDED: {
			char ch = static_cast<char>(notification->ch);
			fBuffer.Append(ch, 1);
			if (ch == B_ENTER) {
				GMessage msg = {{"what", kKTVInputBuffer}, {"buffer", fBuffer}};
				BMessenger(Parent()).SendMessage(&msg);
				ClearBuffer();
			}
			fCaretPosition = SendMessage(SCI_GETCURRENTPOS);
			break;
		}
	}
}
