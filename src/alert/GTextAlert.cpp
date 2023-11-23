/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 */

#include "GTextAlert.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include <string>
#include <vector>

#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GTextAlert"

GTextAlert::GTextAlert(const char *title, const char *message, const char *text)
	:
	GAlert<BString>(title, message),
	fTextControl(nullptr),
	fText(text)
{
	_InitInterface();
	CenterOnScreen();
}


GTextAlert::~GTextAlert()
{
}


void
GTextAlert::_InitInterface()
{
	fTextControl = new BTextControl("", fText, new BMessage(kTextControlMessage));
	// BLayoutBuilder::Group<>(GetPlaceholderView(), B_VERTICAL, B_USE_DEFAULT_SPACING)
		// .Add(fTextControl)
		// .End();
	GetPlaceholderView()->AddChild(fTextControl);
}


void
GTextAlert::Show()
{
	GAlert::Show();
}


void
GTextAlert::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 1: {
			break;
		}
		default: {
			break;
		}
	}

	GAlert::MessageReceived(message);
}
