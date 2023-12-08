/*
 * Copyright 2023 Nexus6 
 * Parts taken from QuitAlert.cpp
 * Copyright 2016-2018 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GitAlert.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GitAlert"

namespace {
const int kSemTimeOut = 50000;
const uint32 kMaxItems = 4;
}


GitAlert::GitAlert(const char *title, const char *message, const std::vector<BString> &files)
	:
	BWindow(BRect(100, 100, 200, 200), title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS, 0),
	fTitle(title),
	fMessage(message),
	fFiles(files),
	fMessageString(nullptr),
	fScrollView(nullptr),
	fOK(nullptr),
	fFileStringView(files.size(), nullptr),
	fAlertSem(0)

{
	_InitInterface();
	CenterOnScreen();
}


GitAlert::~GitAlert()
{
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


void
GitAlert::_InitInterface()
{
	fMessageString = new BStringView("message", fMessage);
	fOK = new BButton(B_TRANSLATE("OK"), new BMessage(B_QUIT_REQUESTED));
	BGroupView* filesView = new BGroupView(B_VERTICAL, 0);
	filesView->SetViewUIColor(B_CONTROL_BACKGROUND_COLOR);
	fScrollView = new BScrollView("files", filesView, 0, false, true);
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fMessageString)
		.Add(fScrollView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fOK)
		.End()
		.SetInsets(B_USE_SMALL_INSETS);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	float textHeight = fh.ascent + fh.descent + fh.leading + 5;
	fScrollView->SetExplicitSize(BSize(B_SIZE_UNSET,
		textHeight * std::min<uint32>(fFiles.size(), kMaxItems) + 25.0f));
	BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);
	bar->SetSteps(textHeight / 2.0f, textHeight * 3.0f / 2.0f);
	bar->SetRange(0.0f, fFiles.size() > kMaxItems ?
		(textHeight + 3.0f) * (fFiles.size() - kMaxItems) : 0.0f);

	BGroupLayout* files = filesView->GroupLayout();
	files->SetInsets(B_USE_SMALL_INSETS);
	for (size_t i = 0; i < fFiles.size(); ++i) {
		fFileStringView[i] = new BStringView("file", fFiles[i].String());
		files->AddView(fFileStringView[i]);
	}
}


void
GitAlert::Show()
{
	BWindow::Show();
	fScrollView->SetExplicitSize(BSize(Bounds().Width(), B_SIZE_UNSET));
}


// borrowed from BAlert
void
GitAlert::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < 0) {
		Quit();
	}

	// Get the originating window, if it exists
	BWindow* window = dynamic_cast<BWindow*>(
		BLooper::LooperForThread(find_thread(nullptr)));

	Show();

	if (window != nullptr) {
		status_t status;
		for (;;) {
			do {
				status = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
					kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (status == B_INTERRUPTED);

			if (status == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	if (Lock())
		Quit();
}


void
GitAlert::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default: {
			BWindow::MessageReceived(message);
			break;
		}
	}
	// No need to delete the semaphore because we have only the OK button that sends a
	// B_QUIT_REQUESTED and the message would be released anyway
	// TODO: turn this class into a subclass of GAlert and let it manage the button's lifecycle
	// delete_sem(fAlertSem);
	// fAlertSem = -1;
}
